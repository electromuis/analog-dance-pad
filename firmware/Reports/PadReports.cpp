#include "ReportsInternal.h"
#include "PadReports.h"

extern "C" {
#include "../ADC.h"
#include "../Reset.h"
#include "../Lights.h"
#include "../Debug.h"
}

const char globalBoardType[] = BOARD_TYPE;

InputHIDReport::InputHIDReport()
{
    Pad_UpdateState();

    bool hasPressed = false;

    // write buttons to the report
    for (int i = 0; i < BUTTON_COUNT; i++) {
        // trol https://stackoverflow.com/a/47990
        buttons[i / 8] ^= (-PAD_STATE.buttonsPressed[i] ^ buttons[i / 8]) & (1UL << i % 8);

        if(PAD_STATE.buttonsPressed[i]) {
            hasPressed = true;
        }
    }
   
    // write sensor values to the report
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorValues[i] = PAD_STATE.sensorValues[i];
    }

    Lights_DataLedCycle(hasPressed);
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
        LIGHT_CONF.selectedLightRuleIndex = (uint8_t)propertyValue;
        break;

    case SPID_SELECTED_LED_MAPPING_INDEX:
        LIGHT_CONF.selectedLedMappingIndex = (uint8_t)propertyValue;
        break;

    case SPID_SELECTED_SENSOR_INDEX:
        PAD_CONF.selectedSensorIndex = (uint8_t)propertyValue;
        break;
    case SPID_SENSOR_CAL_PRELOAD:
        PAD_CONF.selectedSensorIndex = (uint8_t)propertyValue;
        if (PAD_CONF.selectedSensorIndex < SENSOR_COUNT)
        {
            configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = 0;
            Pad_UpdateConfiguration(&configuration.padConfiguration);
            
            uint16_t value = ADC_Read(PAD_CONF.selectedSensorIndex);
            
            configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = value;
            Pad_UpdateConfiguration(&configuration.padConfiguration);
        }
        break;
    }
}

DebugHIDReport::DebugHIDReport()
{
    uint16_t readSize = Debug_Available();
    uint16_t maxReadSize = sizeof(messagePacket);
    
    if(readSize > 0) {
        if(readSize > maxReadSize) {
            readSize = maxReadSize;
        }
        
        messageSize = readSize;
        Debug_ReadBuffer(messagePacket, readSize);
    }
}

void SaveConfiguration()
{
    ConfigStore_StoreConfiguration(&configuration);
}

void FactoryReset()
{
    LEDs_SetAllLEDs(0);
    ConfigStore_FactoryDefaults(&configuration);
    ConfigStore_StoreConfiguration(&configuration);
    SetupConfiguration();
    Reconnect_Usb();
    LEDs_SetAllLEDs(LED_BOOT);
}

void RegisterPadReports()
{
    REGISTER_REPORT_WRITE(INPUT_REPORT_ID, InputHIDReport)

    REGISTER_REPORT(NAME_REPORT_ID, NameFeatureHIDReport)
    REGISTER_REPORT_WRITE(IDENTIFICATION_REPORT_ID, IdentificationFeatureReport)
    REGISTER_REPORT_WRITE(IDENTIFICATION_V2_REPORT_ID, IdentificationV2FeatureReport)
    REGISTER_REPORT_WRITE(DEBUG_REPORT_ID, DebugHIDReport)

    REGISTER_REPORT_PROCESS(SET_PROPERTY_REPORT_ID, SetPropertyHIDReport)

    REGISTER_REPORT_PROCESS_FUNC(RESET_REPORT_ID, Reset_JumpToBootloader)
    REGISTER_REPORT_PROCESS_FUNC(SAVE_CONFIGURATION_REPORT_ID, SaveConfiguration)
    REGISTER_REPORT_PROCESS_FUNC(FACTORY_RESET_REPORT_ID, FactoryReset)
}

