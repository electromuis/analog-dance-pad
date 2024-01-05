#pragma once
#include "stdint.h"

void HAL_EEPROM_Init();
void HAL_EEPROM_Write(uint16_t address, uint8_t* buffer, uint16_t length);
void HAL_EEPROM_Read(uint16_t address, uint8_t* buffer, uint16_t length);
