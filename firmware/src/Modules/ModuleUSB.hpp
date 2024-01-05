#pragma once

#include "Modules.hpp"

class ModuleUSB : public Module {
public:
    void Setup() override;
    void Reconnect();
};

extern ModuleUSB ModuleUSBInstance;
