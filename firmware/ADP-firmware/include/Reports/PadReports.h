#pragma once

#include "stdint.h"
#include "DancePadConfig.h"

//
// INPUT REPORTS
// ie. from microcontroller to computer
//

#define CEILING(x,y) (((x) + (y) - 1) / (y))
#define MAX_NAME_SIZE 50

#pragma pack(push, 1)

typedef struct {
    uint8_t size;
    char name[MAX_NAME_SIZE];
} NameAndSize; 

typedef struct InputHIDReport {
    #ifdef __cplusplus
    InputHIDReport();
    #endif

    uint8_t buttons[CEILING(BUTTON_COUNT, 8)];
    uint16_t sensorValues[SENSOR_COUNT];
} InputHIDReport;

typedef struct NameFeatureHIDReport {
    NameFeatureHIDReport();
    void Process();

    NameAndSize nameAndSize;
} NameFeatureHIDReport;

typedef struct IdentificationFeatureReport {
    IdentificationFeatureReport();

    uint16_t firmwareVersionMajor;
    uint16_t firmwareVersionMinor;
    uint8_t buttonCount;
    uint8_t sensorCount;
    uint8_t ledCount;
    uint16_t maxSensorValue;
    char boardType[32];
} IdentificationFeatureReport;

typedef struct IdentificationV2FeatureReport {
    IdentificationV2FeatureReport();
    
    struct IdentificationFeatureReport parent;
    uint16_t features;
} IdentificationV2FeatureReport;

// IDS used by SetPropertyHIDReport.
#define SPID_SELECTED_LIGHT_RULE_INDEX  0
#define SPID_SELECTED_LED_MAPPING_INDEX 1
#define SPID_SELECTED_SENSOR_INDEX 2
#define SPID_SENSOR_CAL_PRELOAD 3

typedef struct SetPropertyHIDReport {
    void Process();
    
    uint32_t propertyId;
    uint32_t propertyValue;
} SetPropertyHIDReport;

typedef struct DebugHIDReport {
    DebugHIDReport();

    uint16_t messageSize;
    char messagePacket[32];
} DebugHIDReport;

#pragma pack(pop)

void RegisterPadReports();
