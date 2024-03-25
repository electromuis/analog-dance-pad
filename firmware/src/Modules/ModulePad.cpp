#include "ModulePad.hpp"
#include "ModuleConfig.hpp"
#include "hal/hal_ADC.hpp"
#include "hal/hal_USB.hpp"

ModulePad ModulePadInstance = ModulePad();

void ModulePad::Setup()
{
    HAL_ADC_Init();
}

void ModulePad::Update()
{
    HAL_USB_Update();
}

void ModulePad::UpdateStatus()
{
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorValues[i] = HAL_ADC_ReadSensor(i);
    }

    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool newButtonPressedState = false;

        for (int j = 0; j < SENSOR_COUNT; j++) {
			SensorConfig s = ModuleConfigInstance.GetSensorConfig(j);
            if(s.buttonMapping != i) {
                continue;
            }

            uint16_t sensorVal = sensorValues[j];

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

            sensorStates[j] = newButtonPressedState;
        }

        buttonsPressed[i] = newButtonPressedState;
    }

}
