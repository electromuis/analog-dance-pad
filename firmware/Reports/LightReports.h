#pragma once

#include "Reports.h"
#include "../Lights.h"

typedef struct LightRuleHIDReport {
    #ifdef __cplusplus
    LightRuleHIDReport();
    void Process();
    #endif

    uint8_t index;
    LightRule rule;
} __attribute__((packed)) LightRuleHIDReport;

typedef struct LedMappingHIDReport {
    #ifdef __cplusplus
    LedMappingHIDReport();
    void Process();
    #endif

    uint8_t index;
    LedMapping mapping;
} __attribute__((packed)) LedMappingHIDReport;

void RegisterLightReports();
