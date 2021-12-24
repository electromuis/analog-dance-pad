#pragma once

#include "Websocket.h"
#include "Message.hpp"
#include "Event.hpp"
#include "url.hpp"

#include "Model/Device.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace websocketpp;
typedef websocketpp::server<websocketpp::config::asio> mywsserver;

namespace adp
{

class Socket
{

public:
    Socket(mywsserver& wsserver, const string& nsp, connection_hdl hdl)
    :m_wsserver(wsserver)
    ,m_namespace(nsp)
    ,m_ws_hdl(hdl)
    {
        json j;
        j["devices"] = json::array();
        auto pad = Device::Pad();
        if (pad)
        {
            auto& d = j["devices"][0];

            d["id"] = 0;

            d["configuration"]["name"] = pad->name;
            d["configuration"]["sensorThresholds"] = json::array();
            d["properties"]["buttonCount"] = pad->numButtons;
            d["properties"]["sensorCount"] = pad->numSensors;
            
            for (int s = 0; s < pad->numSensors; s++)
            {
                auto sensor = Device::Sensor(s);
                d["configuration"]["sensorThresholds"][s] = sensor->threshold;
                d["configuration"]["releaseThreshold"][s] = sensor->releaseThreshold;
                d["configuration"]["sensorToButtonMapping"][s] = sensor->button;
            }

        }

        emit("devicesUpdated", j);

        on("subscribeToDevice", [](const json& data) {

        });
    }

    void on(const std::string& event, function<void (const json& data)> cb)
    {
        m_events[event] = cb;
    }

    void emit(const string& name, const json& data)
    {
        json req = json::array();
        req[0] = name;
        req[1] = data;

        string pl = "42" + req.dump();
        m_wsserver.send(m_ws_hdl, pl, frame::opcode::value::text);
    }

    void onEvent(const string& event, const json& json)
    {
        auto iter = m_events.find(event);
        if (iter != m_events.end())
        {
            iter->second({event, json});
        }
    }

    string uuid() const
    {
        auto connection = m_wsserver.get_con_from_hdl(m_ws_hdl);

        Url url(connection->get_resource());
        auto q = url.query();
        auto r = find_if(q.begin(), q.end(), [](auto& kv) { return kv.key() == "sid"; });
        string sid = "";

        if (r != q.end())
            sid = r->val();

        return sid;
    }

    mywsserver&       m_wsserver;
    const string&     m_namespace;
    connection_hdl    m_ws_hdl;
    map<string, function<void (const json&)>> m_events;
};

}