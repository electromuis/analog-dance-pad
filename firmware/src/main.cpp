#include "Modules/Modules.hpp"
#include "Reports/Reports.hpp"

void setup() {
  RegisterReports();
  
  ModulesSetup();
}

void loop() {
  ModulesUpdate();
}

#ifndef ARDUINO_ARCH_ESP32

int main(void)
{
  setup();
  for(;;)
    loop();
  return 0;
}

#endif