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

uint8_t sensorLookup[16] = {
    7,
    6,
    5,
    4,
    3,
    2,
    1,
    0,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15
};

void HAL_ADC_Init()
{
    adc_set_clk_div(8);
    adc1_config_width(ADC_WIDTH_BIT_12);
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
    SensorConfig sensorConfig = ModuleConfigInstance.GetSensorConfig(sensorIndex);

    // spiPot->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_SPI_POT_CS, LOW);
    spiPot->transfer(0b00010001);
    spiPot->transfer(sensorConfig.resistorValue);
    digitalWrite(PIN_SPI_POT_CS, HIGH);
    // spiPot->endTransaction();
}

uint16_t HAL_ADC_ReadSensor(uint8_t sensorIndex)
{
    if(!spiReady)
        return 0;

    setMuxer(sensorIndex);
    setDigipot(sensorIndex);
    uint16_t value = adc1_get_raw(channel);

    return value;
}