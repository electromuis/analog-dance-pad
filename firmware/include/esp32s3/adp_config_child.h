#pragma once

// #define FEATURE_WEBSERVER_ENABLED 1
#define FEATURE_RTOS_ENABLED 1

#define ADC_LAYOUT_LEONARDO 1
#define ADC_LAYOUT_MINIPAD_1 2
#define ADC_LAYOUT_MINIPAD_2 3
#define ADC_LAYOUT_FSRIO_2 4

#define CONFIG_LAYOUT_PAD4 1
#define CONFIG_LAYOUT_BOARD8 2

// Set the board type if not provided to the make command
// #define BOARD_TYPE_

#define MAX_SENSOR_VALUE 1024

// this value doesn't mean we're actively using all these buttons.
// it's just what we report and is technically possible to use.
// for now, should be divisible by 8.
#define BUTTON_COUNT 16

#define SENSOR_COUNT 16
#define MAX_LIGHT_RULES 16
#define MAX_LED_MAPPINGS 16

#define BOARD_TYPE "FSRio V3"
#define BOOTLOADER_ADDRESS "0x7000"

#define FEATURE_LIGHTS_ENABLED
#define FEATURE_DIGIPOT_ENABLED

#define LED_PANELS 8
#define PANEL_LEDS 8

#define ADC_LAYOUT ADC_LAYOUT_FSRIO_2
#define CONFIG_LAYOUT CONFIG_LAYOUT_BOARD8


#ifndef DEFAULT_NAME
    #define DEFAULT_NAME BOARD_TYPE
#endif

#if defined(FEATURE_LIGHTS_ENABLED)
    #define LED_COUNT (LED_PANELS * PANEL_LEDS)
#else
    #define LED_COUNT 0
#endif
