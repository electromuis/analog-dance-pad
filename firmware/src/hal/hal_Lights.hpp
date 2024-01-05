#pragma once

#include "Structs.hpp"

void HAL_Lights_Setup();
void HAL_Lights_SetLed(uint8_t index, rgb_color color);
void HAL_Lights_Update();
