#include <Model/DeviceClient.h>
#include <Model/Reporter.h>
#include <Model/Log.h>

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <functional>
#include <future>
#include <deque>

#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

namespace adp {
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;
typedef std::promise<std::string> ws_message;

struct ReportPacket : public ReportPacketBase {
    bool Send(ws_client& socket_, websocketpp::connection_hdl& con_hdl)
    {
        std::string payload;
        payload.append((const char*)&header, sizeof(ReportHeader));
        payload.append(data);

        websocketpp::lib::error_code ec;
        socket_.send(con_hdl, payload, websocketpp::frame::opcode::binary, ec);
        return !ec;
    }
};

class BackendWs : public ReporterBackend
{
public:
	BackendWs(std::string url)
        :ReporterBackend()
	{
        socket_.init_asio();

        websocketpp::lib::error_code ec;
        ws_client::connection_ptr con = socket_.get_connection(url, ec);
        if (ec)
        {
            connectedPromise.set_value(false);
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return;
        }

        socket_.set_access_channels(websocketpp::log::alevel::none);
        con->set_open_handler(std::bind(&BackendWs::on_connect, this, std::placeholders::_1));
        con->set_close_handler(std::bind(&BackendWs::on_disconnect, this, std::placeholders::_1));
        con->set_message_handler(std::bind(&BackendWs::on_message, this, std::placeholders::_1, std::placeholders::_2));
        con->set_fail_handler(std::bind(&BackendWs::on_disconnect, this, std::placeholders::_1));

        socket_.connect(con);
        
        thread = std::thread(std::bind(&BackendWs::Thread, this));
        connectedPromise.get_future().wait();
	}

    void Thread()
    {
        socket_.run();
    }

	~BackendWs()
	{
        thread.join();
	}

    void on_message(websocketpp::connection_hdl hdl, ws_client::message_ptr message)
    {
        if (message->get_opcode() != websocketpp::frame::opcode::binary)
            return;

        if (responseQueue.empty())
            return;

        responseQueue.back().set_value(message->get_payload());
        responseQueue.pop_back();
    }

    void on_connect(websocketpp::connection_hdl con)
    {
        connected = true;
        connectedPromise.set_value(true);
        con_hdl = con;
    }

    void on_disconnect(websocketpp::connection_hdl con)
    {
        if (connected) {
            connected = false;
        }
        else {
            connectedPromise.set_value(false);
        }
        
            
        while (!responseQueue.empty()) {
            responseQueue.back().set_exception(std::make_exception_ptr("Disconnected"));
            responseQueue.pop_back();
        }

        con_hdl.reset();
    }

    ReportPacket send_and_get(ReportPacket packet)
    {
        auto responsePacketData = this->responseQueue.emplace_front().get_future();

        if (!packet.Send(socket_, con_hdl))
            return ReportPacket();

        std::string response = responsePacketData.get();
        if(response.length() == 0)
            return ReportPacket();

        ReportPacket responsePacket;
        if(!responsePacket.FromString(response))
            return ReportPacket();

        return responsePacket;
    }

	int get_feature_report(unsigned char *data, size_t length)
	{
        if(!connected)
            return 0;

        ReportPacket request;
        request.header.cmd = CMD_REPORT_GET;
        request.header.reportId = data[0];

        ReportPacket response = send_and_get(request);

        if (response.header.cmd != CMD_REPORT_GET_ACK || response.header.reportId != request.header.reportId)
            return 0;

        memcpy(data+1, response.data.c_str(), response.header.length);

        return response.header.length+1;
	}

	int send_feature_report(const unsigned char *data, size_t length)
	{
        if(!connected)
            return 0;

        ReportPacket request;
        request.header.cmd = CMD_REPORT_SEND;
        request.header.reportId = data[0];
        request.header.length = length-1;
        request.data = std::string((const char*)data+1, length-1);

        ReportPacket response;

        try {
            response = send_and_get(request);
        }
        catch (...) {
            return -1;
        }


        if(response.header.cmd != CMD_REPORT_SEND_ACK || response.header.reportId != request.header.reportId)
            return 0;

		return length;
	}

	int read(unsigned char *data, size_t length)
	{
		if(!connected)
            return 0;

        ReportPacket request;
        request.header.cmd = CMD_DATA_READ;
        request.header.reportId = data[0];

        ReportPacket response;
        try {
            response = send_and_get(request);
        }
        catch (...) {
            return -1;
        }
        
        if (response.header.cmd != CMD_DATA_READ_ACK || response.header.reportId != request.header.reportId)
            return 0;

        memcpy(data+1, response.data.c_str(), response.header.length);

        return response.header.length+1;
	}

	int write(unsigned char *data, size_t length)
	{
		return send_feature_report(data, length);
	}

	const wchar_t* error()
	{
		return 0;
	}

protected:
    std::thread thread;
    std::promise<bool> connectedPromise;
    bool connected = false;
    std::deque<ws_message> responseQueue;
    ws_client socket_;
    websocketpp::connection_hdl con_hdl;
    ws_client::connection_ptr con;
};

ReporterBackend* ReporterBackendWsCreate(std::string url)
{
    return new BackendWs(url);
}


};