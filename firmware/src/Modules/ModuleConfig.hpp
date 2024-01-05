#pragma once

#include "Modules.hpp"
#include "Structs.hpp"

class ModuleConfig : public Module {
public:
    void Setup() override;
    
    void LoadDefaults();
    void Load();
    void Save();
    SensorConfig GetSensorConfig(uint8_t index);
};

extern Configuration configuration;
extern ModuleConfig ModuleConfigInstance;
