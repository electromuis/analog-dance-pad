#include "ModuleDebug.hpp"
#include "hal/hal_Debug.hpp"

ModuleDebug ModuleDebugInstance;

void ModuleDebug::Setup()
{
    HAL_Debug_Init();
}

void ModuleDebug::Write(const char* message, ...)
{
    // char buffer[256];
    // va_list args;
    // va_start(args, message);
    // vsnprintf(buffer, 256, message, args);
    // va_end(args);

    // HAL_Debug_Write(buffer);
}