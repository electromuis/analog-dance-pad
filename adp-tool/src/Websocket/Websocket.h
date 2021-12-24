#pragma once

#include <wx/thread.h>

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

#include "SocketNamespace.hpp"

using namespace std;
using namespace websocketpp;

namespace adp {

typedef websocketpp::server<websocketpp::config::asio> wsserver;

class WebSocketServer : public wxThread
{
public:
	WebSocketServer();
	~WebSocketServer();

	shared_ptr<SocketNamespace> of(const string& nsp);
	shared_ptr<SocketNamespace> sockets();

	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_http(connection_hdl hdl);
	void on_message(connection_hdl hdl, wsserver::message_ptr msg);

	static void Init();
	static void Shutdown();
	static void UpdatePadValues();

protected:
	virtual ExitCode Entry();

	bool stopRequested = false;
	string docRoot = "";

	wsserver m_server;

	shared_ptr<SocketNamespace> m_sockets;
	map<string, shared_ptr<SocketNamespace>> m_socket_namespace;
};

}