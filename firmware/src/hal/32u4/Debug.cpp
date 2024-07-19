#include "hal_Debug.hpp"

#ifdef SERIAL_DEBUG_ENABLED

#include <HardwareSerial.h>

HardwareSerial mySerial(0);
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

void HAL_Debug_Init()
{
    esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
    esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client


    mySerial.begin(115200, SERIAL_8N1, 33, 34);
    mySerial.println("Hello World");
}

void HAL_Debug_Write(const char* buffer)
{
    mySerial.println(buffer);
}

#else
void HAL_Debug_Init()
{

}

void HAL_Debug_Write(const char* buffer)
{

}
#endif
