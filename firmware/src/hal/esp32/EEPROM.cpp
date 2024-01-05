#include "hal/hal_EEPROM.hpp"
#include <EEPROM.h>
#include "Structs.hpp"

const ssize_t EEPROM_SIZE = sizeof(Configuration) + 128;

void HAL_EEPROM_Init()
{
    EEPROM.begin(EEPROM_SIZE);
}

void HAL_EEPROM_Write(uint16_t address, uint8_t* buffer, uint16_t length)
{
    EEPROM.writeBytes(address, buffer, length);
    EEPROM.commit();
}

void HAL_EEPROM_Read(uint16_t address, uint8_t* buffer, uint16_t length)
{
    EEPROM.readBytes(address, buffer, length);
}
