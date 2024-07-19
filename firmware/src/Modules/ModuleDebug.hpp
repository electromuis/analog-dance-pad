#pragma once

#include "Modules.hpp"
#include "Structs.hpp"

class ModuleDebug : public Module {
public:
    void Setup() override;
    void Write(const char* message, ...);
};

extern ModuleDebug ModuleDebugInstance;


#ifdef SERIAL_DEBUG_ENABLED
#define DEBUG_WRITE(...) ModuleDebugInstance.Write(__VA_ARGS__)
#else
#define DEBUG_WRITE(...)
#endif