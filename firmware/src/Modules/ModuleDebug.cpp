#include "ModuleDebug.hpp"
#include <HardwareSerial.h>

ModuleDebug ModuleDebugInstance;

HardwareSerial mySerial(0);

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

void ModuleDebug::Setup()
{
    esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
    esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client


    mySerial.begin(115200, SERIAL_8N1, 33, 34);
    mySerial.println("Hello World");
}

void ModuleDebug::Write(const char* message, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, 256, message, args);
    va_end(args);
    mySerial.println(buffer);
}