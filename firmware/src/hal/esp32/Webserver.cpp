#include <Arduino.h>

#include "WiFi.h"
#include "ESPAsyncWebSrv.h"
#include "SPIFFS.h"

const char* ssid     = "VRWifi";
const char* password = "12345678";

AsyncWebServer server(80);

void HAL_Webserver_Init()
{
    Serial.begin(9600);
    Serial.println("Booting");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Route for root / web page
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.begin();
}

void HAL_Webserver_Update()
{

}