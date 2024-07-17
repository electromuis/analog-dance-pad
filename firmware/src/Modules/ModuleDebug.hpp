#pragma once

#include "Modules.hpp"
#include "Structs.hpp"

class ModuleDebug : public Module {
public:
    void Setup() override;
    void Write(const char* message, ...);
};

extern ModuleDebug ModuleDebugInstance;


#define DEBUG_WRITE(...) ModuleDebugInstance.Write(__VA_ARGS__)