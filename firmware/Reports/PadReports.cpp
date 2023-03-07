#include "ReportsInternal.h"
#include "PadReports.h"
#include "./../Config/DancePadConfig.h"
#include "./../Descriptors.h"

const char globalBoardType[] = BOARD_TYPE;

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

void RegisterPadReports()
{
    REGISTER_REPORT(IDENTIFICATION_REPORT_ID, IdentificationFeatureReport)
    REGISTER_REPORT(IDENTIFICATION_V2_REPORT_ID, IdentificationV2FeatureReport)
}
