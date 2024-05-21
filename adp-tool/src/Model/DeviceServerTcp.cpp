#include "DeviceServer.h"

#include <thread>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/redirect_error.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;

namespace adp {

//----------------------------------------------------------------------

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

class chat_session
  :public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, hid_device* device)
    : socket_(std::move(socket)),
      device(device)
  {
    
  }

  void start()
  {
    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->reader(); },
        detached);
  }

private:
  awaitable<void> reader()
  {
    while(socket_.is_open())
    {
        try
        {
            ReportHeader header;
            std::size_t n = co_await socket_.async_receive(asio::buffer((char*)&header, sizeof(header)), use_awaitable);
            ReportHeader responseHeader;
            unsigned char buffer[1024];

            if(n == sizeof(header))
            {
                switch (header.cmd)
                {
                    case CMD_REPORT_SEND:
                    {
                        buffer[0] = header.reportId;
                        socket_.receive(asio::buffer(buffer, header.length));
                        int result = hid_send_feature_report(device, (const unsigned char*)buffer, header.length);
                        
                        responseHeader.cmd = CMD_REPORT_SEND_ACK;
                        responseHeader.reportId = header.reportId;
                        responseHeader.length = result;

                        co_await asio::async_write(socket_, asio::buffer((char*)&responseHeader, sizeof(responseHeader)), use_awaitable);
                    }
                    break;
                    case CMD_REPORT_GET:
                    {
                        buffer[0] = header.reportId;
                        int result = hid_get_feature_report(device, buffer, header.length);
                        
                        responseHeader.cmd = CMD_REPORT_GET_ACK;
                        responseHeader.reportId = header.reportId;
                        responseHeader.length = result;

                        socket_.send(asio::buffer((char*)&responseHeader, sizeof(responseHeader)));
                        socket_.send(asio::buffer((char*)&buffer, result));
                    }
                    break;
                    case CMD_DATA_READ:
                    {
                        buffer[0] = header.reportId;
                        int result = hid_read(device, buffer, header.length);
                        
                        responseHeader.cmd = CMD_DATA_READ_ACK;
                        responseHeader.reportId = header.reportId;
                        responseHeader.length = result;

                        socket_.send(asio::buffer((char*)&responseHeader, sizeof(responseHeader)));
                        socket_.send(asio::buffer((char*)&buffer, result));
                    }
                    break;
                
                default:
                    {
                        responseHeader.cmd = CMD_FAILED;
                        responseHeader.reportId = 0;
                        responseHeader.length = 0;

                        socket_.send(asio::buffer((char*)&responseHeader, sizeof(responseHeader)));
                    }
                }
            }
        }
        catch (std::exception& e)
        {
            std::string err = e.what();
            stop();
        }
    }
  }

  void stop()
  {
    socket_.close();
  }

  hid_device* device;
  tcp::socket socket_;
};

//----------------------------------------------------------------------

awaitable<void> listener(tcp::acceptor acceptor, hid_device* device)
{
  for (;;)
  {
    std::make_shared<chat_session>(
        co_await acceptor.async_accept(use_awaitable),
        device
      )->start();
  }
}

class MyDeviceServer : public DeviceServer {
public:
    MyDeviceServer(hid_device* device)
        :DeviceServer(device), io_context(1)
    {
        threadMain = std::thread(&MyDeviceServer::ThreadFunc, this);
    }

    void ThreadFunc()
    {
        co_spawn(
            io_context,
            listener(tcp::acceptor(io_context, {tcp::v4(), 1234}), device),
            detached
        );

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ io_context.stop(); });

        io_context.run();
    }

    ~MyDeviceServer()
    {
        io_context.stop();
        threadMain.join();
    }

protected:
    asio::io_context io_context;
    std::thread threadMain;
};

DeviceServer* DeviceServerCreate(hid_device* device)
{
    return new MyDeviceServer(device);
}

};