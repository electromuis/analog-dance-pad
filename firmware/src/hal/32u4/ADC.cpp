#include "hal/hal_ADC.hpp"

void HAL_SPI_Setup()
{
    SPCR = (1 << SPE) | (1 << MSTR);
}

void HAL_SPI_Write(uint8_t* buffer, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        SPDR = buffer[i];
        while (!(SPSR & (1 << SPIF)));
    }
}

void HAL_SPI_Select(uint8_t module)
{
    if(module == 1) { // Digipot
        PORTE &= ~(1 << DDE6);
    } else {
        PORTE |= 1 << DDE6;
    }
}

uint16_t HAL_ADC_ReadSensor(uint8_t sensorIndex)
{
    return 0;
}
