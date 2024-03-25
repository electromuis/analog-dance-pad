#include "ReportsInternal.hpp"
#include "PadReports.hpp"
#include "Modules/ModuleLights.hpp"
#include "Modules/ModulePad.hpp"
#include "Modules/ModuleConfig.hpp"
#include "Modules/ModuleUSB.hpp"
#include "hal/hal_Bootloader.hpp"

#include <string.h>

const char globalBoardType[] = BOARD_TYPE;

InputHIDReport::InputHIDReport()
{
    // write buttons to the report
    for (int i = 0; i < BUTTON_COUNT; i++) {
        // trol https://stackoverflow.com/a/47990
        buttons[i / 8] ^= (-ModulePadInstance.buttonsPressed[i] ^ buttons[i / 8]) & (1UL << i % 8);
    }
   
    // write sensor values to the report
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorValues[i] = ModulePadInstance.sensorValues[i];
    }

    //ModuleLightsInstance.DataCycle();
}

NameFeatureHIDReport::NameFeatureHIDReport()
{
    memcpy(&nameAndSize, &configuration.nameAndSize, sizeof (nameAndSize));
}

void NameFeatureHIDReport::Process()
{
    memcpy(&configuration.nameAndSize, &nameAndSize, sizeof (configuration.nameAndSize));
}

IdentificationFeatureReport::IdentificationFeatureReport()
{
    this->firmwareVersionMajor = FIRMWARE_VERSION_MAJOR;
    this->firmwareVersionMinor = FIRMWARE_VERSION_MINOR;
        
    this->buttonCount = BUTTON_COUNT;
    this->sensorCount = SENSOR_COUNT;
    this->ledCount = LED_COUNT;
    this->maxSensorValue = MAX_SENSOR_VALUE;

    memset(this->boardType, 0, sizeof(this->boardType));
    strcpy(this->boardType, globalBoardType);
}

IdentificationV2FeatureReport::IdentificationV2FeatureReport(): features(0)
{
    #if defined(FEATURE_DEBUG_ENABLED)
        features |= FEATURE_DEBUG;
    #endif

    #if defined(FEATURE_DIGIPOT_ENABLED)
        features |= FEATURE_DIGIPOT;
    #endif

    #if defined(FEATURE_LIGHTS_ENABLED)
        features |= FEATURE_LIGHTS;
    #endif
}

void SetPropertyHIDReport::Process()
{
    switch (propertyId)
    {
    case SPID_SELECTED_LIGHT_RULE_INDEX:
        if(propertyValue < MAX_LIGHT_RULES)
            ModuleLightsInstance.selectedLightRuleIndex = (uint8_t)propertyValue;
        break;

    case SPID_SELECTED_LED_MAPPING_INDEX:
        if(propertyValue < MAX_LED_MAPPINGS)
            ModuleLightsInstance.selectedLedMappingIndex = (uint8_t)propertyValue;
        break;

    case SPID_SELECTED_SENSOR_INDEX:
        if(propertyValue < SENSOR_COUNT)
            ModulePadInstance.selectedSensorIndex = (uint8_t)propertyValue;
        break;
    // case SPID_SENSOR_CAL_PRELOAD:
    //     PAD_CONF.selectedSensorIndex = (uint8_t)propertyValue;
    //     if (PAD_CONF.selectedSensorIndex < SENSOR_COUNT_V1)
    //     {
    //         configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = 0;
            
    //         uint16_t value = ADC_Read(PAD_CONF.selectedSensorIndex);
            
    //         configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = value;
    //         Pad_UpdateConfiguration(&configuration.padConfiguration);
    //     }
    //     break;
    }
}

DebugHIDReport::DebugHIDReport()
{
    
}

void SaveConfiguration()
{
    ModuleConfigInstance.Save();
}

void FactoryReset()
{
    // LEDs_SetAllLEDs(0);
    ModuleConfigInstance.LoadDefaults();
    ModuleUSBInstance.Reconnect();
    
    // LEDs_SetAllLEDs(LED_BOOT);
}

void RegisterPadReports()
{
    REGISTER_REPORT_WRITE(INPUT_REPORT_ID, InputHIDReport)

    REGISTER_REPORT(NAME_REPORT_ID, NameFeatureHIDReport)
    REGISTER_REPORT_WRITE(IDENTIFICATION_REPORT_ID, IdentificationFeatureReport)
    REGISTER_REPORT_WRITE(IDENTIFICATION_V2_REPORT_ID, IdentificationV2FeatureReport)
    REGISTER_REPORT_WRITE(DEBUG_REPORT_ID, DebugHIDReport)

    REGISTER_REPORT_PROCESS(SET_PROPERTY_REPORT_ID, SetPropertyHIDReport)

    REGISTER_REPORT_PROCESS_FUNC(RESET_REPORT_ID, HAL_Bootloader)
    REGISTER_REPORT_PROCESS_FUNC(SAVE_CONFIGURATION_REPORT_ID, SaveConfiguration)
    REGISTER_REPORT_PROCESS_FUNC(FACTORY_RESET_REPORT_ID, FactoryReset)
}

