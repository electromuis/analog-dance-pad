
#pragma once

#include "Reports.hpp"
#include "Structs.hpp"

typedef struct PadConfigurationFeatureHIDReport {
    PadConfigurationFeatureHIDReport();
    void Process();

    PadConfigurationV1 reportConfiguration;
} __attribute__((packed)) PadConfigurationFeatureHIDReport;

typedef struct SensorHIDReport {
    SensorHIDReport();
    void Process();

    uint8_t index;
    SensorConfigV13 sensor;
} __attribute__((packed)) SensorHIDReport;

void RegisterSensorReports();
