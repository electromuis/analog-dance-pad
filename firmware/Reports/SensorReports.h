
#pragma once

#include "Reports.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../ConfigStore.h"

#ifdef __cplusplus
}
#endif

typedef struct PadConfigurationFeatureHIDReport {
    #ifdef __cplusplus
    PadConfigurationFeatureHIDReport();
    void Process();
    #endif

    PadConfiguration reportConfiguration;
} __attribute__((packed)) PadConfigurationFeatureHIDReport;

typedef struct SensorHIDReport {
    #ifdef __cplusplus
    SensorHIDReport();
    void Process();
    #endif

    uint8_t index;
    SensorConfigV13 sensor;
} __attribute__((packed)) SensorHIDReport;

void RegisterSensorReports();