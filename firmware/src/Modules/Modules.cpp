#include "Modules.hpp"

#include "ModuleUSB.hpp"
#include "ModuleConfig.hpp"
#include "ModulePad.hpp"
#include "ModuleLights.hpp"

Module* modules[] = {
    &ModuleUSBInstance,
    &ModuleConfigInstance,
    &ModulePadInstance,
    // &ModuleLightsInstance
};

void ModulesSetup()
{
    for (auto module : modules)
        module->Setup();
}

void ModulesUpdate()
{
    for (auto module : modules)
        module->Update();
}
