#pragma once

#include "stdint.h"
#include "Modules.hpp"
#include "Structs.hpp"

class ModuleLights : public Module {
public:
    void Setup() override;
    void Update() override;

    void DataCycle();
    void SetManualMode(bool mode);
    void SetManual(int index, rgb_color color);


	uint8_t selectedLightRuleIndex = 0;
    uint8_t selectedLedMappingIndex = 0;

protected:
    uint32_t dataCycleCount = 200;
    uint32_t ledTimer = 0;
};

extern ModuleLights ModuleLightsInstance;
