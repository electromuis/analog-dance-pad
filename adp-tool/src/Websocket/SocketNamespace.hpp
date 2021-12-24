#pragma once

#include "Websocket.h"
#include "Socket.hpp"
#include "Model/Log.h"

using namespace websocketpp;
typedef websocketpp::server<websocketpp::config::asio> mywsserver;

namespace adp {

class Socket;

class SocketNamespace
{
public:

    SocketNamespace(const string& nsp, mywsserver& server)
    :m_namespace(nsp)
    ,m_wsserver(server)
    {
    }

    void onConnection(function<void (Socket&)> cb)
    {
        //sig_Connection.connect(cb);
    }

    void onDisconnection(function<void (Socket&)> cb)
    {
        //sig_Disconnection.connect(cb);
    }

    void emit(const string& name, const json& data)
    {
        for (const auto& i : m_sockets)
        {
            i.second->emit(name, data);
        }
    }

    string socketNamespace() const
    {
        return m_namespace;
    }

    void onSocketIoConnection(connection_hdl hdl)
    {
        auto socket = make_shared<Socket>(m_wsserver, m_namespace, hdl);
        m_sockets[hdl] = socket;
    }
                    
    void onSocketIoEvent(connection_hdl hdl, string msg)
    {
        json j;

        try {
            j = json::parse(msg);
            if (!j.is_array() || j.empty())
            {
                Log::Write(L"WS :: Invalid json received");
                return;
            }
        }
        catch (json::exception e)
        {
            Log::Write(L"WS :: Invalid json received");
            return;
        }

        string name = j[0];
        json args = json::array();
        for (int i = 1; i < j.size(); i++)
        {
            args[i - 1] = j[i];
        }


        auto iter = m_sockets.find(hdl);
        if (iter != m_sockets.end())
        {
            iter->second->onEvent(name, args);
        }

    }

    void onSocketIoDisconnect(connection_hdl hdl)
    {
        auto iter = m_sockets.find(hdl);
        if (iter != m_sockets.end())
        {
            m_sockets.erase(iter);
        }
    }

    string m_namespace;
    mywsserver& m_wsserver;

    map<connection_hdl, shared_ptr<Socket>, std::owner_less<connection_hdl>> m_sockets;
};

}