#include <Model/DeviceClient.h>
#include <Model/Reporter.h>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <emscripten/val.h>
#include <emscripten/promise.h>

#include <memory>
#include <cstdlib>
#include <iostream>

namespace adp {

struct ReportPacket : public ReportPacketBase {
    bool Send(EMSCRIPTEN_WEBSOCKET_T& socket_)
    {
        std::string payload;
        payload.append((const char*)&header, sizeof(ReportHeader));
        payload.append(data);

        EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(socket_, (void*)payload.c_str(), payload.length());
        return result == EMSCRIPTEN_RESULT_SUCCESS;
    }
};

class BackendWs : public ReporterBackend
{
public:
	BackendWs(std::string url)
        :ReporterBackend()
	{
        connectPromise = emscripten_promise_create();

        EmscriptenWebSocketCreateAttributes ws_attrs = {
            (const char*)url.c_str(),
            NULL,
            EM_TRUE
        };

        socket_ = emscripten_websocket_new(&ws_attrs);

        emscripten_websocket_set_onopen_callback(socket_, this, [](int eventType, const EmscriptenWebSocketOpenEvent* websocketEvent, void* userData) {
            return ((BackendWs*)userData)->on_connect(eventType, websocketEvent);
        });

        emscripten_websocket_set_onclose_callback(socket_, this, [](int eventType, const EmscriptenWebSocketCloseEvent* websocketEvent, void* userData) {
            return ((BackendWs*)userData)->on_disconnect(eventType, websocketEvent);
        });

        emscripten_websocket_set_onmessage_callback(socket_, this, [](int eventType, const EmscriptenWebSocketMessageEvent* websocketEvent, void* userData) {
            return ((BackendWs*)userData)->on_message(eventType, websocketEvent);
        });

        emscripten_promise_await(connectPromise);
        emscripten_promise_destroy(connectPromise);
	}

    EM_BOOL on_message(int eventType, const EmscriptenWebSocketMessageEvent* websocketEvent)
    {
        if(websocketEvent->isText)
            return EM_TRUE;

        if(!responsePromise)
            return EM_TRUE;

        lastResponse = std::string((const char*)websocketEvent->data, websocketEvent->numBytes);
        emscripten_promise_resolve(*responsePromise, EM_PROMISE_FULFILL, (void*)&lastResponse);

        return EM_TRUE;
    }

    EM_BOOL on_connect(int eventType, const EmscriptenWebSocketOpenEvent* websocketEvent)
    {
        connected = true;
        emscripten_promise_resolve(connectPromise, EM_PROMISE_FULFILL, (void*)true);

        return EM_TRUE;
    }

    EM_BOOL on_disconnect(int eventType, const EmscriptenWebSocketCloseEvent* websocketEvent)
    {
        connected = false;

        return EM_TRUE;
    }

    ReportPacket send_and_get(ReportPacket packet)
    {
        responsePromise = std::make_unique<em_promise_t>(emscripten_promise_create());

        if (!packet.Send(socket_))
           return ReportPacket();

        em_settled_result_t promiseResult = emscripten_promise_await(*responsePromise);
        std::string response = *(std::string*)promiseResult.value;

        em_promise_t* promisePtr = responsePromise.release();
        emscripten_promise_destroy(*promisePtr);
        delete promisePtr;

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

        ReportPacket response = send_and_get(request);

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

        ReportPacket response = send_and_get(request);
        
        if (response.header.cmd != CMD_DATA_READ_ACK || response.header.reportId != request.header.reportId)
            return 0;

        memcpy(data+1, response.data.c_str(), response.header.length);

        return response.header.length+1;
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
    EMSCRIPTEN_WEBSOCKET_T socket_;
    em_promise_t connectPromise;
    bool connected = false;
    std::unique_ptr<em_promise_t> responsePromise;
    std::string lastResponse = "";
};

ReporterBackend* ReporterBackendWsCreate(std::string url)
{
    return new BackendWs(url);
}


};