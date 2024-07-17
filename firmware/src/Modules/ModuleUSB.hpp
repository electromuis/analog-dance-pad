#pragma once

#include <stdint.h>
#include "Modules.hpp"

class ModuleUSB : public Module {
public:
    void Setup() override;
    void Update() override;
    void Reconnect();
    virtual bool ShouldMainUpdate() override { return false; }

protected:
    uint32_t errCount = 0;
};

extern ModuleUSB ModuleUSBInstance;
