#pragma once

#include "Reports.h"

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

void RegisterPadReports();
