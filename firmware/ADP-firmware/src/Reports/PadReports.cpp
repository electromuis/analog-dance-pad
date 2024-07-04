#include "Reports/PadReports.h"
#include <string.h>

const char globalBoardType[] = BOARD_TYPE;

InputHIDReport::InputHIDReport()
{
    buttons[0] = 0b001010101;
    buttons[1] = 0b000000000;
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensorValues[i] = 200;
    }
}

NameFeatureHIDReport::NameFeatureHIDReport()
{
    const char name[] = {"Test"};
    nameAndSize.size = strlen(name);
    memcpy(&nameAndSize.name, &name, nameAndSize.size);
}

void NameFeatureHIDReport::Process()
{
    
}

IdentificationFeatureReport::IdentificationFeatureReport()
{
    this->firmwareVersionMajor = FIRMWARE_VERSION_MAJOR;
    this->firmwareVersionMinor = FIRMWARE_VERSION_MINOR;
        
    this->buttonCount = BUTTON_COUNT;
    this->sensorCount = SENSOR_COUNT;
    this->ledCount = LED_COUNT;
    this->maxSensorValue = MAX_SENSOR_VALUE;

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
    
}

DebugHIDReport::DebugHIDReport()
{
    
}

void SaveConfiguration()
{
    
}

void FactoryReset()
{
    
}
