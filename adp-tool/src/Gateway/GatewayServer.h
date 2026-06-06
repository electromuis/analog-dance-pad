#pragma once

#include <string>
#include <memory>

namespace adp {

class GatewayServerImpl;

class GatewayServer {
public:
    GatewayServer(int port);
    ~GatewayServer();

    void BroadcastDevicesUpdated();
    void BroadcastInputEvent(int deviceIndex, const std::string& deviceId);
    void BroadcastEventRate(const std::string& deviceId, int rate);
    void ProcessCommands();

private:
    std::unique_ptr<GatewayServerImpl> impl;
};

} // namespace adp
