#pragma once

#include "Reports.hpp"
#include "Structs.hpp"

typedef struct LightRuleHIDReport {
    LightRuleHIDReport();
    void Process();

    uint8_t index;
    LightRule rule;
} __attribute__((packed)) LightRuleHIDReport;

typedef struct LedMappingHIDReport {
    LedMappingHIDReport();
    void Process();

    uint8_t index;
    LedMapping mapping;
} __attribute__((packed)) LedMappingHIDReport;

typedef struct LightsHIDReport {
    LightsHIDReport();
    void Process();

    rgb_color lights[MAX_LED_MAPPINGS];
} __attribute__((packed)) LightsHIDReport;

void RegisterLightReports();
