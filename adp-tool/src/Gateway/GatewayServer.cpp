#include "Gateway/GatewayServer.h"
#include "Model/Device.h"
#include "Model/Log.h"

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

typedef websocketpp::server<websocketpp::config::asio> ws_server;
typedef websocketpp::connection_hdl connection_hdl;

namespace adp {

struct GatewayClient {
    std::set<std::string> subscribedDevices;
};

class GatewayServerImpl {
public:
    GatewayServerImpl(int port) : port(port)
    {
        server.init_asio();
        server.set_access_channels(websocketpp::log::alevel::none);
        server.set_error_channels(websocketpp::log::elevel::none);

        server.set_open_handler([this](connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[hdl] = GatewayClient{};
            auto payload = BuildDevicesUpdated().dump();
            try { server.send(hdl, payload, websocketpp::frame::opcode::text); } catch (...) {}
        });

        server.set_close_handler([this](connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(hdl);
        });

        server.set_fail_handler([this](connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(hdl);
        });

        server.set_message_handler([this](connection_hdl hdl, ws_server::message_ptr msg) {
            OnMessage(hdl, msg->get_payload());
        });

        server.listen(port);
        server.start_accept();

        Log::Writef("GatewayServer :: listening on port %d", port);

        serverThread = std::thread([this]() {
            try { server.run(); }
            catch (const std::exception& e) { Log::Writef("GatewayServer :: %s", e.what()); }
        });
    }

    ~GatewayServerImpl()
    {
        server.stop_listening();
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& [hdl, _] : clients) {
                try { server.close(hdl, websocketpp::close::status::going_away, ""); } catch (...) {}
            }
        }
        server.stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    void OnMessage(connection_hdl hdl, const std::string& payload)
    {
        try {
            auto msg = json::parse(payload);
            std::string action = msg.value("action", "");
            json data = msg.value("data", json::object());

            if (action == "subscribeToDevice") {
                std::string deviceId = data.value("deviceId", "");
                std::lock_guard<std::mutex> lock(clientsMutex);
                if (clients.count(hdl)) clients[hdl].subscribedDevices.insert(deviceId);
            }
            else if (action == "unsubscribeFromDevice") {
                std::string deviceId = data.value("deviceId", "");
                std::lock_guard<std::mutex> lock(clientsMutex);
                if (clients.count(hdl)) clients[hdl].subscribedDevices.erase(deviceId);
            }
            else if (action == "updateConfiguration") {
                std::lock_guard<std::mutex> lock(commandsMutex);
                commands.push_back([data]() { HandleUpdateConfiguration(data); });
            }
            else if (action == "updateSensorThreshold") {
                std::lock_guard<std::mutex> lock(commandsMutex);
                commands.push_back([data]() { HandleUpdateSensorThreshold(data); });
            }
            else if (action == "calibrate") {
                std::lock_guard<std::mutex> lock(commandsMutex);
                commands.push_back([data]() { HandleCalibrate(data); });
            }
        }
        catch (const std::exception& e) {
            Log::Writef("GatewayServer :: OnMessage error: %s", e.what());
        }
    }

    static void HandleUpdateConfiguration(const json& data)
    {
        auto pad = Device::Pad();
        if (!pad) return;

        auto config = data.value("configuration", json::object());

        if (config.contains("name") && config["name"].is_string()) {
            std::string name = config["name"];
            Device::SetDeviceName(name.c_str());
        }

        if (config.contains("releaseThreshold") && config["releaseThreshold"].is_number()) {
            Device::SetReleaseThreshold(config["releaseThreshold"].get<double>());
        }

        if (config.contains("sensorThresholds") && config["sensorThresholds"].is_array()) {
            for (int i = 0; i < (int)config["sensorThresholds"].size() && i < pad->numSensors; ++i) {
                double threshold = config["sensorThresholds"][i].get<double>();
                auto sensor = Device::Sensor(i);
                if (sensor) Device::SetThreshold(i, threshold, sensor->releaseThreshold);
            }
        }

        if (config.contains("sensorToButtonMapping") && config["sensorToButtonMapping"].is_array()) {
            for (int i = 0; i < (int)config["sensorToButtonMapping"].size() && i < pad->numSensors; ++i) {
                Device::SetButtonMapping(i, config["sensorToButtonMapping"][i].get<int>());
            }
        }

        if (data.value("store", false)) Device::SaveChanges();
    }

    static void HandleUpdateSensorThreshold(const json& data)
    {
        auto pad = Device::Pad();
        if (!pad) return;

        int sensorIndex = data.value("sensorIndex", 0);
        double newThreshold = data.value("newThreshold", 0.0);

        auto sensor = Device::Sensor(sensorIndex);
        if (sensor) Device::SetThreshold(sensorIndex, newThreshold, sensor->releaseThreshold);

        if (data.value("store", false)) Device::SaveChanges();
    }

    static void HandleCalibrate(const json& data)
    {
        auto pad = Device::Pad();
        if (!pad) return;

        // calibrationBuffer is a margin added above the current sensor reading (0.1 = 10%)
        double calibrationBuffer = data.value("calibrationBuffer", 0.1);

        for (int i = 0; i < pad->numSensors; ++i) {
            auto sensor = Device::Sensor(i);
            if (sensor && sensor->value > 0.0) {
                double threshold = sensor->value * (1.0 + calibrationBuffer);
                Device::SetThreshold(i, threshold, threshold);
            }
        }

        if (data.value("store", false)) Device::SaveChanges();
    }

    json BuildDevicesUpdated()
    {
        json devicesJson = json::object();
        auto pad = Device::Pad();
        if (pad) {
            const std::string deviceId = "device_0";

            json config;
            config["name"] = pad->name;
            config["releaseThreshold"] = pad->releaseThreshold;
            config["sensorThresholds"] = json::array();
            config["sensorToButtonMapping"] = json::array();

            for (int i = 0; i < pad->numSensors; ++i) {
                auto sensor = Device::Sensor(i);
                if (sensor) {
                    config["sensorThresholds"].push_back(sensor->threshold);
                    config["sensorToButtonMapping"].push_back(sensor->button);
                }
            }

            json props;
            props["buttonCount"] = pad->numButtons;
            props["sensorCount"] = pad->numSensors;

            json device;
            device["id"] = deviceId;
            device["configuration"] = config;
            device["properties"] = props;

            devicesJson[deviceId] = device;
        }

        json msg;
        msg["action"] = "devicesUpdated";
        msg["data"]["devices"] = devicesJson;
        return msg;
    }

    void BroadcastDevicesUpdated()
    {
        auto payload = BuildDevicesUpdated().dump();
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& [hdl, _] : clients) {
            try { server.send(hdl, payload, websocketpp::frame::opcode::text); } catch (...) {}
        }
    }

    void BroadcastInputEvent(const std::string& deviceId)
    {
        auto pad = Device::Pad();
        if (!pad) return;

        json sensors = json::array();
        json buttons = json::array();
        for (int i = 0; i < pad->numSensors; ++i) {
            auto sensor = Device::Sensor(i);
            if (sensor) {
                sensors.push_back(sensor->value);
                buttons.push_back(sensor->pressed);
            }
        }

        json msg;
        msg["action"] = "inputEvent";
        msg["data"]["deviceId"] = deviceId;
        msg["data"]["inputData"]["sensors"] = sensors;
        msg["data"]["inputData"]["buttons"] = buttons;
        auto payload = msg.dump();

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& [hdl, client] : clients) {
            if (client.subscribedDevices.count(deviceId)) {
                try { server.send(hdl, payload, websocketpp::frame::opcode::text); } catch (...) {}
            }
        }
    }

    void BroadcastEventRate(const std::string& deviceId, int rate)
    {
        json msg;
        msg["action"] = "eventRate";
        msg["data"]["deviceId"] = deviceId;
        msg["data"]["eventRate"] = rate;
        auto payload = msg.dump();

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& [hdl, client] : clients) {
            if (client.subscribedDevices.count(deviceId)) {
                try { server.send(hdl, payload, websocketpp::frame::opcode::text); } catch (...) {}
            }
        }
    }

    void ProcessCommands()
    {
        std::vector<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(commandsMutex);
            batch = std::move(commands);
        }
        for (auto& cmd : batch) cmd();
    }

private:
    int port;
    ws_server server;
    std::thread serverThread;

    std::mutex clientsMutex;
    std::map<connection_hdl, GatewayClient, std::owner_less<connection_hdl>> clients;

    std::mutex commandsMutex;
    std::vector<std::function<void()>> commands;
};

GatewayServer::GatewayServer(int port) : impl(std::make_unique<GatewayServerImpl>(port)) {}
GatewayServer::~GatewayServer() = default;

void GatewayServer::BroadcastDevicesUpdated() { impl->BroadcastDevicesUpdated(); }
void GatewayServer::BroadcastInputEvent(const std::string& deviceId) { impl->BroadcastInputEvent(deviceId); }
void GatewayServer::BroadcastEventRate(const std::string& deviceId, int rate) { impl->BroadcastEventRate(deviceId, rate); }
void GatewayServer::ProcessCommands() { impl->ProcessCommands(); }

} // namespace adp
