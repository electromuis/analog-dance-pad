#pragma once

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 5

#define FEATURE_DEBUG 1 << 0
#define FEATURE_DIGIPOT 1 << 1
#define FEATURE_LIGHTS 1 << 2
#define FEATURE_WEBSERVER 1 << 3
#define FEATURE_RTOS 1 << 4

#ifdef ARDUINO_ARCH_ESP32
    #include "adp_config_esp32.hpp"
#else
    #include "adp_config_m32u4.hpp"
#endif