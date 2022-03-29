#include <stdbool.h>
#include <string.h>

#include "Config/DancePadConfig.h"
#include "ConfigStore.h"
#include "Pad.h"
#include "ADC.h"
#include "Lights.h"

#define MIN(a,b) ((a) < (b) ? a : b)

PadConfigurationV2 PAD_CONF;

PadState PAD_STATE = { 
    .sensorValues = { [0 ... SENSOR_COUNT - 1] = 0 },
    .buttonsPressed = { [0 ... BUTTON_COUNT - 1] = false }
};

typedef struct {
    uint16_t sensorReleaseThresholds[SENSOR_COUNT];
    int8_t buttonToSensorMap[BUTTON_COUNT][SENSOR_COUNT + 1];
} InternalPadConfiguration;

InternalPadConfiguration INTERNAL_PAD_CONF;

void Pad_UpdateInternalConfiguration(void) {
	/*
    for (int i = 0; i < SENSOR_COUNT; i++) {
        INTERNAL_PAD_CONF.sensorReleaseThresholds[i] = PAD_CONF.sensorThresholds[i] * PAD_CONF.releaseMultiplier;
    }
	*/

    // Precalculate array for mapping buttons to sensors.
    // For every button, there is an array of sensor indices. when there are no more buttons assigned to that sensor,
    // the value is -1.
    for (int buttonIndex = 0; buttonIndex < BUTTON_COUNT; buttonIndex++) {
        int mapIndex = 0;

        for (int sensorIndex = 0; sensorIndex < SENSOR_COUNT; sensorIndex++) {
			SensorConfig s = PAD_CONF.sensors[sensorIndex];
			
            if (s.buttonMapping == buttonIndex) {
                INTERNAL_PAD_CONF.buttonToSensorMap[buttonIndex][mapIndex++] = sensorIndex;
            }
        }

        // mark -1 to end
        INTERNAL_PAD_CONF.buttonToSensorMap[buttonIndex][mapIndex] = -1;
    }
}

void Pad_Initialize(const PadConfigurationV2* padConfiguration) {
    Pad_UpdateConfiguration(padConfiguration);
	ADC_Init();
}

void Pad_UpdateConfiguration(const PadConfigurationV2* padConfiguration) {
    memcpy(&PAD_CONF, padConfiguration, sizeof (PadConfigurationV2));
    Pad_UpdateInternalConfiguration();
}

void Pad_UpdateState(void) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        PAD_STATE.sensorValues[i] = ADC_Read(i);
    }

    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool newButtonPressedState = false;

        for (int j = 0; j < SENSOR_COUNT; j++) {
            int8_t sensor = INTERNAL_PAD_CONF.buttonToSensorMap[i][j];
            
            if (sensor == -1) {
                break;
            }

            if (sensor < 0 || sensor > SENSOR_COUNT) {
                break;
            }

			SensorConfig s = PAD_CONF.sensors[sensor];
            uint16_t sensorVal = PAD_STATE.sensorValues[sensor];

            if (PAD_STATE.buttonsPressed[i]) {
                if (sensorVal > s.releaseThreshold) {
                    newButtonPressedState = true;
                    break;
                }
            } else {
                if (sensorVal > s.threshold) {
                    newButtonPressedState = true;
                    break;
                }
            }
        }

		if(newButtonPressedState)
			LEDs_TurnOnLEDs(LED_DATA);
		
        PAD_STATE.buttonsPressed[i] = newButtonPressedState;
    }
	
	Lights_Update(false);
}
