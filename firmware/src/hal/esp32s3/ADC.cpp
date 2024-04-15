#include "../hal_ADC.hpp"
#include <Arduino.h>
#include "adp_config.hpp"
#include "Modules/ModuleConfig.hpp"
#include "SPI.h"

SPIClass spiPot = SPIClass(HSPI);
static const int spiClk = 80000000; // 80 MHz
static bool spiReady = false;

void HAL_ADC_Init()
{
    spiPot.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_POT_CS);
    spiPot.beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));

    pinMode(PIN_MUX_1, OUTPUT);
    pinMode(PIN_MUX_2, OUTPUT);
    pinMode(PIN_MUX_3, OUTPUT);
    pinMode(PIN_MUX_4, OUTPUT);
    pinMode(spiPot.pinSS(), OUTPUT);

    digitalWrite(spiPot.pinSS(), HIGH);

    spiReady = true;
}

void setMuxer(uint8_t sensorIndex)
{
    digitalWrite(PIN_MUX_1, (bool)(sensorIndex & 0b0001));
    digitalWrite(PIN_MUX_2, (bool)(sensorIndex & 0b0010));
    digitalWrite(PIN_MUX_3, (bool)(sensorIndex & 0b0100));
    digitalWrite(PIN_MUX_4, (bool)(sensorIndex & 0b1000));
}

void setDigipot(uint8_t sensorIndex)
{
    if(!spiReady)
        return;

    SensorConfig sensorConfig = ModuleConfigInstance.GetSensorConfig(sensorIndex);

    // spiPot->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(spiPot.pinSS(), LOW);
    SPI.transfer(0b00010001);
    SPI.transfer(sensorConfig.resistorValue);
    digitalWrite(spiPot.pinSS(), HIGH);
    // spiPot->endTransaction();
}

uint16_t HAL_ADC_ReadSensor(uint8_t sensorIndex)
{
    setMuxer(sensorIndex);
    // setDigipot(sensorIndex);
    return analogRead(PIN_ANALOG_IN);
}