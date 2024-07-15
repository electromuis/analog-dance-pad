#pragma once

#include <stdint.h>
#include "Modules.hpp"

class ModuleUSB : public Module {
public:
    void Setup() override;
    void Update() override;
    void Reconnect();

protected:
    uint32_t errCount = 0;
};

extern ModuleUSB ModuleUSBInstance;
