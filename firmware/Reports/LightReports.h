#pragma once

#include "Reports.h"

#ifdef __cplusplus
    extern "C" {
#endif

#include "../Lights.h"

#ifdef __cplusplus
    }
#endif

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

typedef struct LightsHIDReport {
    #ifdef __cplusplus
    LightsHIDReport();
    void Process();
    #endif

    rgb_color lights[MAX_LED_MAPPINGS];
} __attribute__((packed)) LightsHIDReport;

void RegisterLightReports();
