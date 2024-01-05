#include "Modules/Modules.hpp"
#include "Reports/Reports.hpp"
#include "hal/hal_Webserver.hpp"

void setup() {
  ModulesSetup();
  HAL_Webserver_Init();
}

void loop() {
  ModulesUpdate();
  HAL_Webserver_Update();
}
