#ifdef FEATURE_WEBSERVER_ENABLED

#include <Arduino.h>

#include "WiFi.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebSrv.h"
#include "SPIFFS.h"
#include "Structs.hpp"
#include <cstring>
#include "Reports/Reports.hpp"

#define STACK_SIZE 1024 * 8
StaticTask_t xTaskBuffer;
StackType_t xStack[ STACK_SIZE ];

const char* mySsid     = "VRWifi";
const char* myPassword = "12345678";
AsyncWebServer myServer(80);
AsyncWebSocket myWs("/ws");

enum WS_CMD {
    CMD_FAILED = 0x00,

    CMD_REPORT_SEND = 0x01,
    CMD_REPORT_GET  = 0x02,
    CMD_DATA_READ   = 0x03,

    CMD_REPORT_SEND_ACK = 0x04,
    CMD_REPORT_GET_ACK  = 0x05,
    CMD_DATA_READ_ACK   = 0x06
};

struct ReportHeader {
    uint8_t cmd;
    uint8_t reportId;
    uint16_t length;
};

#define DATA_BUFFER_SIZE 512
uint8_t dataBuffer[DATA_BUFFER_SIZE];

ReportHeader* responseHeader = (ReportHeader*)dataBuffer;
uint8_t* dataBufferPayload = dataBuffer + sizeof(ReportHeader);
String wsDataBuffer = "";

void handleWsPacket(AsyncWebSocketClient * client, uint8_t *data, size_t len)
{
    ReportHeader* header = (ReportHeader*)data;
    responseHeader->cmd = CMD_FAILED;
    responseHeader->reportId = header->reportId;
    responseHeader->length = 0;

    const uint8_t * dataPayload = data + sizeof(ReportHeader);
    size_t payloadSize = header->length;
    uint16_t resultLen = 0;

    if(len != sizeof(ReportHeader) + payloadSize || payloadSize > DATA_BUFFER_SIZE) {
        responseHeader->length = 1001;
        return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));
    }

    switch (header->cmd)
    {
    case CMD_REPORT_SEND:
        if(!ProcessReport(header->reportId, dataPayload, payloadSize)) {
            responseHeader->length = 1002;
            return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));
        }

        responseHeader->cmd = CMD_REPORT_SEND_ACK;
        return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));

    case CMD_DATA_READ:
        header->reportId = INPUT_REPORT_ID;
        if(!WriteReport(header->reportId, dataBufferPayload, &resultLen) ) {
            responseHeader->length = 1002;
            return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));
        }

        responseHeader->cmd = CMD_DATA_READ_ACK;
        responseHeader->length = resultLen;
        return client->binary((uint8_t*)dataBuffer, sizeof(ReportHeader) + resultLen);

    case CMD_REPORT_GET:
        if(!WriteReport(header->reportId, dataBufferPayload, &resultLen) ) {
            responseHeader->length = 1003;
            return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));
        }

        responseHeader->cmd = CMD_REPORT_GET_ACK;
        responseHeader->length = resultLen;
        return client->binary((uint8_t*)dataBuffer, sizeof(ReportHeader) + resultLen);
    }

    responseHeader->reportId = header->cmd;
    responseHeader->length = 1004;
    return client->binary((uint8_t*)responseHeader, sizeof(ReportHeader));
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
    if(type == WS_EVT_CONNECT){
        // client->printf("Hello Client %u :)", client->id());
        // client->ping();
    }
    else if(type == WS_EVT_DATA) {
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        if(info->opcode == WS_TEXT) {
            return client->text("NOTEXT");
        }
       
        if(info->final && info->index == 0 && info->len == len){
            //the whole message is in a single frame and we got all of it's data
            return handleWsPacket(client, data, len);
        }

        return client->text("PACKETED");
    }
}

void wifiTask(void * parameter)
{
    vTaskDelay(300);

    WiFi.mode(WIFI_STA);
    WiFi.begin(mySsid, myPassword);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        return;
    }

    if(!SPIFFS.begin()){
       return;
    }

    auto staticHandler = myServer.serveStatic("/", SPIFFS, "/").setDefaultFile("adp-tool.html");
    staticHandler.setCacheControl("max-age=2592000");

    myWs.onEvent(onWsEvent);
    myServer.addHandler(&myWs);

    myServer.begin();

    while (true)
    {
        vTaskDelay(500);
        myWs.cleanupClients();
    }
}

void HAL_Webserver_Init()
{
    xTaskCreateStaticPinnedToCore(
        wifiTask,
        "wifiTask",
        STACK_SIZE,
        ( void * ) 1,
        6,
        xStack,
        &xTaskBuffer,
        0
    );
}

#else

void HAL_Webserver_Init()
{
}

#endif

