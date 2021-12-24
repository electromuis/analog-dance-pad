#include "Websocket.h"
#include "Message.hpp"

#include "Model/Log.h"
#include <fstream>
#include <wx/regex.h>
#include "wx/msw/ole/uuid.h"
#include "url.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace adp {

WebSocketServer* instance = nullptr;

void WebSocketServer::Init()
{
	instance = new WebSocketServer();
}

void WebSocketServer::Shutdown()
{
	delete instance;
}

void WebSocketServer::UpdatePadValues()
{
	json j;
	j["deviceId"] = 0;
	j["inputData"]["sensors"] = json::array();
	j["inputData"]["buttons"] = json::array();

	auto pad = Device::Pad();
	if (pad)
	{
		for (int b = 0; b < pad->numButtons; b++)
		{
			j["inputData"]["buttons"][b] = false;
		}

		for (int s = 0; s < pad->numSensors; s++)
		{
			auto sensor = Device::Sensor(s);
			j["inputData"]["sensors"][s] = sensor->value;
			j["inputData"]["buttons"][sensor->button] = sensor->pressed;
		}
	}

	instance->sockets()->emit("inputEvent", j);
}

WebSocketServer::WebSocketServer()
    : wxThread(wxTHREAD_DETACHED)
{
    if (Run() != wxTHREAD_NO_ERROR)
    {
        Log::Write(L"Can't create the thread!");
    }
}

WebSocketServer::~WebSocketServer()
{
    stopRequested = true;
}

shared_ptr<SocketNamespace> WebSocketServer::sockets()
{
	return m_sockets;
}

shared_ptr<SocketNamespace> WebSocketServer::of(const string& nsp)
{
	auto iter = m_socket_namespace.find(nsp);
	if (iter == m_socket_namespace.end())
	{
		auto snsp = make_shared<SocketNamespace>(nsp, m_server);
		m_socket_namespace.insert(std::make_pair(nsp, snsp));
		return snsp;
	}
	else
	{
		return iter->second;
	}
}

wxThread::ExitCode WebSocketServer::Entry()
{
	m_sockets = this->of("");

    m_server.set_open_handler(bind(&WebSocketServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&WebSocketServer::on_close, this, ::_1));
    m_server.set_message_handler(bind(&WebSocketServer::on_message, this, ::_1, ::_2));
    m_server.set_http_handler(bind(&WebSocketServer::on_http, this, _1));

    m_server.init_asio();
    m_server.listen(3333);
    m_server.start_accept();

    Log::Write(L"Websocket started");

    while (!stopRequested)
    {
        m_server.run_one();
    }

    return (wxThread::ExitCode)0;
}

void WebSocketServer::on_open(connection_hdl hdl)
{
	auto connection = m_server.get_con_from_hdl(hdl);

	Url url(connection->get_resource());
	auto q = url.query();
	auto r = find_if(q.begin(), q.end(), [](auto& kv) { return kv.key() == "sid"; });
	string sid = "";

	if (r != q.end())
		sid = r->val();

	if (sid == "")
	{
		Uuid id;
		id.Create();
		wxString uid(id);

		json j;
		j["sid"] = "73pll5gZtJfyO3ChAAAA";
		j["upgrades"] = json::array();
		j["pingInterval"] = 25000;
		j["pingTimeout"] = 5000;

		m_server.send(hdl, "0" + j.dump(), frame::opcode::value::text);
		m_server.send(hdl, "40", frame::opcode::value::text);
	}

	m_sockets->onSocketIoConnection(hdl);
}

void WebSocketServer::on_close(connection_hdl hdl)
{
	for (auto sns : m_socket_namespace)
	{
		sns.second->onSocketIoDisconnect(hdl);
	}
}

void WebSocketServer::on_http(connection_hdl hdl)
{
	wsserver::connection_ptr con = m_server.get_con_from_hdl(hdl);

	std::ifstream file;
	std::string filename = con->get_resource();
	std::string response;

	if (filename == "/") {
		filename = docRoot + "/index.html";
	}
	else {
		filename = docRoot + "/" + filename.substr(1);
	}
	
	wxRegEx re = wxRegEx("^/socket\\.io/");
	if (re.Matches(filename))
	{
		con->replace_header("Access-Control-Allow-Origin", "*");

		Url url(filename);
		auto q = url.query();
		auto r = find_if(q.begin(), q.end(), [](auto& kv) { return kv.key() == "sid"; });
		string sid = "";

		if (r != q.end())
			sid = r->val();

		string method = con->get_request().get_method();

		if (method == "GET" && sid == "")
		{
			Uuid id;
			id.Create();
			wxString uid(id);

			json j;
			j["sid"] = uid;
			j["upgrades"] = json::array();
			j["upgrades"][0] = "websocket";
			j["pingInterval"] = 25000;
			j["pingTimeout"] = 5000;

			m_server.send(hdl, "0" + j.dump(), frame::opcode::value::text);
			
			return;
		}

		if (method == "GET" && sid != "")
		{
			Uuid id;
			id.Create();
			wxString uid(id);

			json j;
			j["sid"] = uid;

			m_server.send(hdl, "40" + j.dump(), frame::opcode::value::text);

			return;
		}

		if (method == "POST" && sid != "")
		{
			m_server.send(hdl, "ok", frame::opcode::value::text);
			return;
		}

		
	}

	file.open(filename.c_str(), std::ios::in);
	if (!file) {
		// 404 error
		std::stringstream ss;

		ss << "<!doctype html><html><head>"
			<< "<title>Error 404 (Resource not found)</title><body>"
			<< "<h1>Error 404</h1>"
			<< "<p>The requested URL " << filename << " was not found on this server.</p>"
			<< "</body></head></html>";

		con->set_body(ss.str());
		con->set_status(websocketpp::http::status_code::not_found);

		Log::Writef(L"WS: File not found: %hs", filename);

		return;
	}

	file.seekg(0, std::ios::end);
	response.reserve(file.tellg());
	file.seekg(0, std::ios::beg);

	response.assign((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	con->set_body(response);
	con->set_status(websocketpp::http::status_code::ok);
}

void WebSocketServer::on_message(connection_hdl hdl, wsserver::message_ptr msg)
{
	string payload = msg->get_payload();

	switch (payload[0])
	{
	// Ping
	case '2':
		if (payload == "2probe")
			m_server.send(hdl, "3probe", frame::opcode::value::text);
		else
			m_server.send(hdl, "3", frame::opcode::value::text);

		return;

	// Upgrade
	case '5':
		return;

	// Message
	case '4':
		if (payload.length() < 2)
			return;

		switch (payload[1])
		{
		// Event
		case '2':
			int start = payload.find_first_of('[');
			string e = payload.substr(start);

			m_sockets->onSocketIoEvent(hdl, e);
			return;
		}

		return;
	
	// Disconnect
	case '1':
		m_sockets->onSocketIoDisconnect(hdl);
		return;
	}
}

}