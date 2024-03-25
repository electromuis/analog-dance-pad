#pragma once

#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 0

#define FEATURE_DEBUG 1 << 0
#define FEATURE_DIGIPOT 1 << 1
#define FEATURE_LIGHTS 1 << 2
#define FEATURE_WEBSERVER 1 << 3
#define FEATURE_RTOS 1 << 4


#define SENSOR_COUNT_V1 12

#include "adp_pins.h"
#include "adp_config_child.h"