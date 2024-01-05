#include <Model/DeviceClient.h>
#include <Model/Reporter.h>

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include "asio.hpp"

namespace adp {
using asio::ip::tcp;

#define CMD_FAILED 0x00

#define CMD_REPORT_SEND 0x01
#define CMD_REPORT_GET  0x02
#define CMD_DATA_READ   0x03

#define CMD_REPORT_SEND_ACK 0x04
#define CMD_REPORT_GET_ACK  0x05
#define CMD_DATA_READ_ACK   0x06

struct ReportHeader {
    uint8_t cmd;
    uint8_t reportId;
    uint16_t length;
};

class BackendTcp : public ReporterBackend
{
public:
	BackendTcp(std::string host, std::string port)
        :ReporterBackend(), socket_(io_context)
	{
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(host, port);

        for(auto& ep : endpoints)
        {
            try {
                socket_.connect(ep);
                connected = true;
                break;
            }
            catch(...) {
                continue;
            }
        }
	}

	~BackendTcp()
	{
		
	}

	int get_feature_report(unsigned char *data, size_t length)
	{
        if(!connected)
            return 0;

        ReportHeader header;
        header.cmd = CMD_REPORT_GET;
        header.reportId = data[0];
        header.length = length;
        auto sizeSent = socket_.send(asio::buffer(&header, sizeof(header)));
        if(sizeSent == 0)
            return 0;

        ReportHeader responseHeader;

        int size = socket_.receive(asio::buffer(&responseHeader, sizeof(responseHeader)));
        if(size != sizeof(responseHeader) || responseHeader.cmd != CMD_REPORT_GET_ACK || responseHeader.reportId != header.reportId)
            return 0;

        return socket_.receive(asio::buffer(data, responseHeader.length));
	}

	int send_feature_report(const unsigned char *data, size_t length)
	{
        if(!connected)
            return 0;

        ReportHeader header;
        header.cmd = CMD_REPORT_SEND;
        header.reportId = data[0];
        header.length = length;
        auto sizeSent = socket_.send(asio::buffer(&header, sizeof(header)));
        if(sizeSent == 0)
            return 0;

        sizeSent = socket_.send(asio::buffer(data, length));
        if(sizeSent == 0)
            return 0;

        ReportHeader responseHeader;
        
        int size = socket_.receive(asio::buffer(&responseHeader, sizeof(responseHeader)));
        if(size != sizeof(responseHeader) || responseHeader.cmd != CMD_REPORT_SEND_ACK || responseHeader.reportId != header.reportId)
            return 0;

		return responseHeader.length;
	}

	int read(unsigned char *data, size_t length)
	{
		if(!connected)
            return 0;

        ReportHeader header;
        header.cmd = CMD_DATA_READ;
        header.reportId = data[0];
        header.length = length;
        auto sizeSent = socket_.send(asio::buffer(&header, sizeof(header)));
        if(sizeSent == 0)
            return 0;

        ReportHeader responseHeader;

        int size = socket_.receive(asio::buffer(&responseHeader, sizeof(responseHeader)));
        if(size != sizeof(responseHeader) || responseHeader.cmd != CMD_DATA_READ_ACK || responseHeader.reportId != header.reportId)
            return 0;

        return socket_.receive(asio::buffer(data, responseHeader.length));
	}

	int write(unsigned char *data, size_t length)
	{
		return 0;
	}

	const wchar_t* error()
	{
		return 0;
	}

protected:
    bool connected = false;
	asio::io_context io_context;
    tcp::socket socket_;
};

ReporterBackend* ReporterBackendTcpCreate(std::string host, std::string port)
{
    return new BackendTcp(host, port);
}


};