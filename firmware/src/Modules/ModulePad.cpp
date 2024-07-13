#include "ModulePad.hpp"
#include "ModuleConfig.hpp"
#include "ModuleLights.hpp"
#include "hal/hal_ADC.hpp"
#include "hal/hal_USB.hpp"

ModulePad ModulePadInstance = ModulePad();

void ModulePad::Setup()
{
    HAL_ADC_Init();
}

void ModulePad::Update()
{
    // UpdateStatus();
    // HAL_USB_Update();
}

void ModulePad::UpdateStatus()
{
    for (int i = 0; i < SENSOR_COUNT; i++) {
        // ADC is noise while lights are updating
        // if(ModuleLightsInstance.IsWriting())
        //     continue;
        
        sensorValues[i] = (sensorValues[i] + HAL_ADC_ReadSensor(i)) / 2;
        // sensorValues[i] = HAL_ADC_ReadSensor(i);
    }

    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool newButtonPressedState = false;

        for (int j = 0; j < SENSOR_COUNT; j++) {
			const SensorConfig& s = ModuleConfigInstance.GetSensorConfig(j);
            if(s.buttonMapping != i) {
                continue;
            }

            uint16_t sensorVal = sensorValues[j];
            sensorStates[j] = sensorVal > s.threshold;

            if (buttonsPressed[i]) {
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

        buttonsPressed[i] = newButtonPressedState;
    }


    ModuleLightsInstance.DataCycle();
}
