#pragma once

#include "Modules.hpp"
#include "adp_config.hpp"
#include <stdint.h>

class ModulePad : public Module {
public:
    void Setup() override;
    void Update() override;
    void UpdateStatus();

    uint16_t sensorValues[SENSOR_COUNT];
    bool sensorStates[SENSOR_COUNT];
    bool buttonsPressed[BUTTON_COUNT];

    uint8_t selectedSensorIndex = 0;
};

extern ModulePad ModulePadInstance;
