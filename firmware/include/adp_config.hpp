#pragma once

#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 0

#define FEATURE_DEBUG 1 << 0
#define FEATURE_DIGIPOT 1 << 1
#define FEATURE_LIGHTS 1 << 2
#define FEATURE_WEBSERVER 1 << 3
#define FEATURE_RTOS 1 << 4

// API spec values. Changing these will break compatibility with adp-tool
#define MAX_LIGHT_RULES 16
#define MAX_LED_MAPPINGS 16
#define MAX_NAME_SIZE 50

#define MAX_SENSOR_VALUE 1024
#define SENSOR_COUNT_V1 12
#define BUTTON_COUNT 16
#define LED_UPDATE_INTERVAL 100

#include "arch.h"
#include "adp_pins.h"
#include "adp_config_child.h"

#if defined(FEATURE_LIGHTS_ENABLED)
    #define LED_COUNT (LED_PANELS * PANEL_LEDS)
#else
    #define LED_COUNT 0
#endif
