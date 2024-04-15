#pragma once

#include "stdint.h"
#include "adp_config.hpp"

// Backwards compatibility

typedef struct {
    uint16_t sensorThresholds[SENSOR_COUNT_V1];
    float releaseMultiplier;
    int8_t sensorToButtonMapping[SENSOR_COUNT_V1];
} __attribute__((packed)) PadConfigurationV1;

// USB

enum REPORT_ID
{
    INPUT_REPORT_ID                  = 0x1,
    PAD_CONFIGURATION_REPORT_ID      = 0x2,
    RESET_REPORT_ID                  = 0x3,
    SAVE_CONFIGURATION_REPORT_ID     = 0x4,
    NAME_REPORT_ID                   = 0x5,
    UNUSED_ANALOG_JOYSTICK_REPORT_ID = 0x6,
    LIGHT_RULE_REPORT_ID             = 0x7,
    FACTORY_RESET_REPORT_ID          = 0x8,
    IDENTIFICATION_REPORT_ID         = 0x9,
    LED_MAPPING_REPORT_ID            = 0xA,
    SET_PROPERTY_REPORT_ID           = 0xB,
    SENSOR_REPORT_ID      			 = 0xC,
    DEBUG_REPORT_ID      	     	 = 0xD,
    IDENTIFICATION_V2_REPORT_ID      = 0xE,
    LIGHTS_REPORT_ID                 = 0xF,
};

// PAD

typedef struct {
	uint16_t threshold;
	uint16_t releaseThreshold;
	int8_t buttonMapping;
	uint8_t resistorValue;
	uint16_t flags;
} __attribute__((packed)) SensorConfigV13;

typedef struct {
	uint16_t threshold;
	uint16_t releaseThreshold;
	int8_t buttonMapping;
	uint8_t resistorValue;
	uint16_t flags;
	uint16_t preload;
} __attribute__((packed)) SensorConfig;

typedef struct {
    SensorConfig sensors[SENSOR_COUNT];
	uint8_t selectedSensorIndex;
} __attribute__((packed)) PadConfiguration;

typedef struct {
    uint8_t size;
    char name[MAX_NAME_SIZE];
} __attribute__((packed)) NameAndSize;





// LIGHTS

typedef struct
{
    uint8_t red, green, blue;
} __attribute__((packed)) rgb_color;

typedef struct
{
    uint8_t flags;
    uint8_t lightRuleIndex;
    uint8_t sensorIndex;
    uint8_t ledIndexBegin;
    uint8_t ledIndexEnd;
} __attribute__((packed)) LedMapping;

typedef struct
{
    uint8_t flags;
	rgb_color onColor;
	rgb_color offColor;
	rgb_color onFadeColor;
	rgb_color offFadeColor;
} __attribute__((packed)) LightRule;

typedef struct
{
    LightRule lightRules[MAX_LIGHT_RULES];
    LedMapping ledMappings[MAX_LED_MAPPINGS];
} __attribute__((packed)) LightConfiguration;

enum LedMappingFlags
{
    LMF_ENABLED = 0x1,
};

enum LightRuleFlags
{
    LRF_ENABLED  = 0x1,
    LRF_FADE_ON  = 0x2,
    LRF_FADE_OFF = 0x4,
};




// Configuration

typedef struct {
    PadConfiguration padConfiguration;
    NameAndSize nameAndSize;
    LightConfiguration lightConfiguration;
} __attribute__((packed)) Configuration;
