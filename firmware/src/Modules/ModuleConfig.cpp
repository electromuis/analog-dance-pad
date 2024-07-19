#include "adp_config.hpp"
#include "ModuleConfig.hpp"
#include "hal/hal_EEPROM.hpp"
#include <string.h>

ModuleConfig ModuleConfigInstance = ModuleConfig();

extern "C" {
extern const Configuration DEFAULT_CONFIGURATION;
};

Configuration configuration;

static const uint8_t magicBytes[5] = {10, 58, SENSOR_COUNT, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR};
#define MAGIC_BYTES_ADDRESS ((uint16_t) 0x00)
#define CONFIGURATION_ADDRESS ((uint16_t) (MAGIC_BYTES_ADDRESS + (uint16_t)sizeof (magicBytes)))

void ModuleConfig::Setup()
{
    HAL_EEPROM_Init();
    Load();
}

void ModuleConfig::LoadDefaults()
{
    memcpy(&configuration, &DEFAULT_CONFIGURATION, sizeof (Configuration));
    Save();
}

void ModuleConfig::Load()
{
    uint8_t magicByteBuffer[sizeof (magicBytes)];
    HAL_EEPROM_Read(MAGIC_BYTES_ADDRESS, magicByteBuffer, sizeof (magicBytes));

    if (memcmp(magicByteBuffer, magicBytes, sizeof (magicBytes)) != 0)
    {
        LoadDefaults();
        HAL_EEPROM_Write(MAGIC_BYTES_ADDRESS, (uint8_t*)&magicBytes, sizeof (magicBytes));
    }
    else
    {
        HAL_EEPROM_Read(CONFIGURATION_ADDRESS, (uint8_t*)&configuration, sizeof(configuration));
    }
}

void ModuleConfig::Save()
{
    HAL_EEPROM_Write(CONFIGURATION_ADDRESS, (uint8_t*)&configuration, sizeof(configuration));
}

const SensorConfig& ModuleConfig::GetSensorConfig(uint8_t index)
{
    // if (index < SENSOR_COUNT)
        return configuration.padConfiguration.sensors[index];
    // else
    //     return SensorConfig();
}