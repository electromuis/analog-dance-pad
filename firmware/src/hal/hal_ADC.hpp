#pragma once
#include "stdint.h"

void HAL_ADC_Init();
uint16_t HAL_ADC_ReadSensor(uint8_t sensorIndex);
