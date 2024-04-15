#pragma once

#include "Structs.hpp"

enum HWLeds {
    HWLed_DATA,
    HWLed_POWER
};

void HAL_Lights_Setup();
void HAL_Lights_SetLed(uint8_t index, rgb_color color);
void HAL_Lights_Update();
void HAL_Lights_SetHWLed(HWLeds led, bool state);
