#include "../hal_ADC.hpp"
#include <Arduino.h>
#include "adp_config.hpp"
#include "Modules/ModuleConfig.hpp"
#include "SPI.h"
#include <driver/adc.h>
#include "Modules/ModuleLights.hpp"

SPIClass* spiPot = nullptr;
static const int spiClk = 80000000; // 80 MHz
static bool spiReady = false;
adc1_channel_t channel = ADC1_CHANNEL_9;

#ifdef SENSOR_MAP_MINIPAD
uint8_t sensorLookup[16] = {
    5, // FSRioV3 input 3
    7, // FSRioV3 input 1
    8, // FSRioV3 input 16
    10, // FSRioV3 input 14
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};
#else
uint8_t sensorLookup[16] = {
    7, // FSRioV3 input 1
    6, // FSRioV3 input 2
    5, // FSRioV3 input 3
    4, // FSRioV3 input 4
    3, // FSRioV3 input 5
    2, // FSRioV3 input 6
    1, // FSRioV3 input 7
    0, // FSRioV3 input 8
    15, // FSRioV3 input 9
    14, // FSRioV3 input 10
    13, // FSRioV3 input 11
    12, // FSRioV3 input 12
    11, // FSRioV3 input 13
    10, // FSRioV3 input 14
    9, // FSRioV3 input 15
    8 // FSRioV3 input 16
};
#endif


void HAL_ADC_Init()
{
    adc1_config_channel_atten(channel, ADC_ATTEN_DB_12);

    spiPot = new SPIClass(HSPI);

    spiPot->begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_POT_CS);
    spiPot->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));

    pinMode(PIN_MUX_1, OUTPUT);
    pinMode(PIN_MUX_2, OUTPUT);
    pinMode(PIN_MUX_3, OUTPUT);
    pinMode(PIN_MUX_4, OUTPUT);
    pinMode(PIN_SPI_POT_CS, OUTPUT);

    digitalWrite(PIN_SPI_POT_CS, HIGH);

    spiReady = true;
}

void setMuxer(uint8_t sensorIndex)
{
    sensorIndex = sensorLookup[sensorIndex];

    digitalWrite(PIN_MUX_1, (bool)(sensorIndex & 0b0001));
    digitalWrite(PIN_MUX_2, (bool)(sensorIndex & 0b0010));
    digitalWrite(PIN_MUX_3, (bool)(sensorIndex & 0b0100));
    digitalWrite(PIN_MUX_4, (bool)(sensorIndex & 0b1000));
}

void setDigipot(uint8_t sensorIndex)
{
    const SensorConfig& sensorConfig = ModuleConfigInstance.GetSensorConfig(sensorIndex);

    digitalWrite(PIN_SPI_POT_CS, LOW);
    spiPot->transfer(0b00010001);
    spiPot->transfer(sensorConfig.resistorValue);
    digitalWrite(PIN_SPI_POT_CS, HIGH);
}

uint16_t HAL_ADC_ReadSensor(uint8_t sensorIndex)
{
    if(!spiReady)
        return 0;

    setMuxer(sensorIndex);
    setDigipot(sensorIndex);
    adc1_config_channel_atten(channel, ADC_ATTEN_DB_12);
    uint16_t value = adc1_get_raw(channel);

    return value;
}