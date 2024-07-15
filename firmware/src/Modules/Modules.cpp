#include "adp_config.hpp"
#include "Modules.hpp"

#include "ModuleUSB.hpp"
#include "ModuleConfig.hpp"
#include "ModulePad.hpp"
#include "ModuleLights.hpp"
#include "ModuleDebug.hpp"
#include "hal/hal_Webserver.hpp"

#ifdef FEATURE_RTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef FEATURE_WEBSERVER_ENABLED
class ModuleWebserver : public Module {
public:
    void Setup() {
        HAL_Webserver_Init();
    }

    void Update() {
        HAL_Webserver_Update();
    }
};
ModuleWebserver ModuleWebserverInstance;
#endif

Module* modules[] = {
    &ModuleDebugInstance,
    // &ModuleConfigInstance,
    &ModuleLightsInstance,
    &ModuleUSBInstance,
    &ModulePadInstance,
// #ifdef FEATURE_WEBSERVER_ENABLED
//     &ModuleWebserverInstance
// #endif
    
};

void ModulesSetup()
{
    for (auto module : modules)
        module->Setup();
}

void ModulesUpdate()
{
    for (auto module : modules)
        if(module->ShouldMainUpdate())
            module->Update();
}
