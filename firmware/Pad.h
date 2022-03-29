#ifndef _PAD_H_
#define _PAD_H_

#include <stdint.h>
#include <stdbool.h>
#include "Config/DancePadConfig.h"

enum SensorConfigFlags
{
	ADC_DISABLED     = 1
};

typedef struct {
    uint16_t sensorThresholds[SENSOR_COUNT];
    float releaseMultiplier;
    int8_t sensorToButtonMapping[SENSOR_COUNT];
} __attribute__((packed)) PadConfiguration;

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
} __attribute__((packed)) PadConfigurationV2;

typedef struct {
    uint16_t sensorValues[SENSOR_COUNT];
    bool buttonsPressed[BUTTON_COUNT];
} PadState;

void Pad_Initialize(const PadConfigurationV2* padConfiguration);
void Pad_UpdateState(void);
void Pad_UpdateConfiguration(const PadConfigurationV2* padConfiguration);

extern PadConfigurationV2 PAD_CONF;
extern PadState PAD_STATE;

#endif
