#pragma once

#include "Reports.h"

#ifdef __cplusplus
    extern "C" {
#endif

#include "../Config/DancePadConfig.h"
#include "../ConfigStore.h"

#ifdef __cplusplus
    }
#endif

//
// INPUT REPORTS
// ie. from microcontroller to computer
//

#define CEILING(x,y) (((x) + (y) - 1) / (y))

typedef struct InputHIDReport {
    #ifdef __cplusplus
    InputHIDReport();
    #endif

    uint8_t buttons[CEILING(BUTTON_COUNT, 8)];
    uint16_t sensorValues[SENSOR_COUNT];
} __attribute__((packed)) InputHIDReport;

typedef struct NameFeatureHIDReport {
    #ifdef __cplusplus
    NameFeatureHIDReport();
    void Process();
    #endif

    NameAndSize nameAndSize;
} __attribute__((packed)) NameFeatureHIDReport;

typedef struct IdentificationFeatureReport {
    #ifdef __cplusplus
    IdentificationFeatureReport();
    #endif

    uint16_t firmwareVersionMajor;
    uint16_t firmwareVersionMinor;
    uint8_t buttonCount;
    uint8_t sensorCount;
    uint8_t ledCount;
    uint16_t maxSensorValue;
    char boardType[32];
} __attribute__((packed)) IdentificationFeatureReport;

typedef struct IdentificationV2FeatureReport {
    #ifdef __cplusplus
    IdentificationV2FeatureReport();
    #endif
    
    struct IdentificationFeatureReport parent;
    uint16_t features;
}  __attribute__((packed)) IdentificationV2FeatureReport;

// IDS used by SetPropertyHIDReport.
#define SPID_SELECTED_LIGHT_RULE_INDEX  0
#define SPID_SELECTED_LED_MAPPING_INDEX 1
#define SPID_SELECTED_SENSOR_INDEX 2
#define SPID_SENSOR_CAL_PRELOAD 3

typedef struct SetPropertyHIDReport {
    #ifdef __cplusplus
    void Process();
    #endif
    
    uint32_t propertyId;
    uint32_t propertyValue;
} __attribute__((packed)) SetPropertyHIDReport;

typedef struct DebugHIDReport {
    #ifdef __cplusplus
    DebugHIDReport();
    #endif

    uint16_t messageSize;
    char messagePacket[32];
} DebugHIDReport;

void RegisterPadReports();
