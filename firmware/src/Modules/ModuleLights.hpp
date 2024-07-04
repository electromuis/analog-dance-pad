#pragma once

#include "adp_config.hpp"
#include "stdint.h"
#include "Modules.hpp"
#include "Structs.hpp"
#include <vector>

class LedMappingWrapper {
public:
    LedMappingWrapper() {};
    LedMappingWrapper(uint8_t i);

    bool IsValid();
    const LightRule& GetRule();
    const LedMapping& GetMapping();
    SensorConfig GetSensorConfig();
    bool Apply();
    bool HasFlag(LightRuleFlags f);
    
protected:
    void Fill(rgb_color color);
    bool lastState = false;
    uint8_t mappingIndex = 0;
    rgb_color CalcColor();
    rgb_color lastColor;
    bool ApplyPulse();

    std::vector<unsigned long> pulses;
};

class ModuleLights : public Module {
public:
    void Setup() override;
    void Update() override;

    void DataCycle();
    void SetManualMode(bool mode);
    void SetManual(int index, rgb_color color);
    bool IsWriting() { return writing; };

	uint8_t selectedLightRuleIndex = 0;
    uint8_t selectedLedMappingIndex = 0;
protected:
    LedMappingWrapper wrappers[MAX_LED_MAPPINGS];
    uint32_t dataCycleCount = 200;
    uint32_t ledTimer = 0;
    bool writing = false;
};

extern ModuleLights ModuleLightsInstance;
