#include "DeviceClient.h"
#include "DeviceServer.h"
#include "Log.h"

#include <websocketpp/config/core.hpp>
#include <websocketpp/server.hpp>

#include <thread>

namespace adp {

typedef websocketpp::server<websocketpp::config::core> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::placeholders::_3;
using websocketpp::lib::bind;
using websocketpp::lib::error_code;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

class SReportPacket : public ReportPacketBase {
public:
    bool Send(server& s, websocketpp::connection_hdl& con_hdl)
    {
        std::string payload;
        payload.append((const char*)&header, sizeof(ReportHeader));
        payload.append(data);

        s.send(con_hdl, payload, websocketpp::frame::opcode::binary);
        return true;
    }
};

class MyDeviceServer : public DeviceServer {
public:
    MyDeviceServer(ReporterBackend& device)
        :DeviceServer(device)
    {
        threadMain = std::thread(&MyDeviceServer::ThreadFunc, this);
    }

    void ThreadFunc()
    {
        s.set_message_handler(bind(&MyDeviceServer::OnMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

        error_code ec;
        server::connection_ptr con = s.get_connection(ec);
        if(ec) {
            Log::Writef("MyDeviceServer :: could not create connection because: %s", ec.message());
            return;
        }

        con->start();
    }

    void OnMessage(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        SReportPacket responsePacket;

        if (msg->get_opcode() != websocketpp::frame::opcode::binary)
        {
            responsePacket.header.length = 2001;
            responsePacket.Send(s, hdl);
            return;
        }

        SReportPacket packet;
        if (!packet.FromString(msg->get_payload()))
        {
            responsePacket.header.length = 2002;
            responsePacket.Send(s, hdl);
            return;
        }

        responsePacket = OnPacket(packet);
        responsePacket.Send(s, hdl);
    }

    SReportPacket OnPacket(SReportPacket& packet)
    {
        SReportPacket responsePacket;

        switch (packet.header.cmd)
        {
        }
                
        return responsePacket;
    }

    ~MyDeviceServer()
    {
        threadMain.join();
    }

protected:
    server s;
    std::thread threadMain;
};

DeviceServer* DeviceServerCreate(ReporterBackend& device)
{
    return new MyDeviceServer(device);
}

};