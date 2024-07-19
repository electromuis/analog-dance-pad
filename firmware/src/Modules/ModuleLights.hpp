#pragma once

#include "adp_config.hpp"
#include "stdint.h"
#include "Modules.hpp"
#include "Structs.hpp"

#define MAX_PULSE_COUNT 8

class LedMappingWrapper {
public:
    // LedMappingWrapper(): mappingIndex(99) {};
    LedMappingWrapper(uint8_t i);

    const LightRule& GetRule();
    const SensorConfig& GetSensorConfig();
    void Apply();
    bool HasFlag(LightRuleFlags f);
    
protected:
    uint8_t mappingIndex = 0;
    LedMapping* mapping = nullptr;
    bool lastState = false;

    bool IsValid();
    void Fill(rgb_color color);
    
    rgb_color CalcColor();
    bool ApplyPulse();

    

    // unsigned long pulses[8] = {0};
    // Vector<unsigned long> pulses(pulsesStorage, sizeof(pulsesStorage));
};

class ModuleLights : public Module {
public:
    void Setup() override;
    void Update() override;
    void Clear();

    void DataCycle();
    void SetManualMode(bool mode);
    void SetManual(int index, rgb_color color);
    bool IsWriting() { return writing; };
    void SetPowerLed(bool state);

	uint8_t selectedLightRuleIndex = 0;
    uint8_t selectedLedMappingIndex = 0;
protected:
    LedMappingWrapper wrappers[MAX_LED_MAPPINGS] = {
        LedMappingWrapper(0),
        LedMappingWrapper(1),
        LedMappingWrapper(2),
        LedMappingWrapper(3),
        LedMappingWrapper(4),
        LedMappingWrapper(5),
        LedMappingWrapper(6),
        LedMappingWrapper(7),
        LedMappingWrapper(8),
        LedMappingWrapper(9),
        LedMappingWrapper(10),
        LedMappingWrapper(11),
        LedMappingWrapper(12),
        LedMappingWrapper(13),
        LedMappingWrapper(14),
        LedMappingWrapper(15)
    };
    uint8_t dataCycleCount = 200;
    uint32_t ledTimer = 0;
    bool ledState = false;
    bool writing = false;
};

extern ModuleLights ModuleLightsInstance;
