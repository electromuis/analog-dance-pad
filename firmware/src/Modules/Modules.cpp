#include "adp_config.hpp"
#include "Modules.hpp"

#include "ModuleUSB.hpp"
#include "ModuleConfig.hpp"
#include "ModulePad.hpp"
#include "ModuleLights.hpp"
#include "hal/hal_Webserver.hpp"

#ifdef FEATURE_WEBSERVER_ENABLED
class ModuleWebserver : public Module {
public:
    void Setup() {
        HAL_Webserver_Init();
    }
};
ModuleWebserver ModuleWebserverInstance;
#endif

Module* modules[] = {
    &ModuleUSBInstance,
    &ModuleConfigInstance,
    &ModulePadInstance,
    &ModuleLightsInstance
#ifdef FEATURE_WEBSERVER_ENABLED
    ,&ModuleWebserverInstance
#endif
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
