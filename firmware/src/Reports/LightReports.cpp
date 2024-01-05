#include "ReportsInternal.hpp"
#include "LightReports.hpp"
#include "Modules/ModuleLights.hpp"
#include <string.h>


LightRuleHIDReport::LightRuleHIDReport()
{
    index = ModuleLightsInstance.selectedLightRuleIndex;
    
    if (index < MAX_LIGHT_RULES)
        memcpy(&rule, &configuration.lightConfiguration.lightRules[index], sizeof(LightRule));
    else
        memset(&rule, 0, sizeof(LightRule));
}

void LightRuleHIDReport::Process()
{
    if (index < MAX_LIGHT_RULES)
    {
        memcpy(&configuration.lightConfiguration.lightRules[index], &rule, sizeof(LightRule));
    }
}

LedMappingHIDReport::LedMappingHIDReport()
{
    index = ModuleLightsInstance.selectedLedMappingIndex;
    if (index < MAX_LED_MAPPINGS)
        memcpy(&mapping, &configuration.lightConfiguration.ledMappings[index], sizeof(LedMapping));
    else
        memset(&mapping, 0, sizeof(LedMapping));
}

void LedMappingHIDReport::Process()
{
    if (index < MAX_LED_MAPPINGS)
    {
        memcpy(&configuration.lightConfiguration.ledMappings[index], &mapping, sizeof(LedMapping));
    }
}

LightsHIDReport::LightsHIDReport()
{

}

void LightsHIDReport::Process()
{
    ModuleLightsInstance.SetManualMode(true);
    
    for(int i=0; i<MAX_LED_MAPPINGS; i++)
    {
        ModuleLightsInstance.SetManual(i, lights[i]);
    }
}

void RegisterLightReports()
{
    REGISTER_REPORT(LIGHT_RULE_REPORT_ID, LightRuleHIDReport)
    REGISTER_REPORT(LED_MAPPING_REPORT_ID, LedMappingHIDReport)
    REGISTER_REPORT_PROCESS(LIGHTS_REPORT_ID, LightsHIDReport)
}
