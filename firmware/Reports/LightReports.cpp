#include "ReportsInternal.h"
#include "LightReports.h"

extern "C" {
#include "../AnalogDancePad.h"
}

LightRuleHIDReport::LightRuleHIDReport()
{
    index = LIGHT_CONF.selectedLightRuleIndex;
    if (index < MAX_LIGHT_RULES)
        memcpy(&rule, &LIGHT_CONF.lightRules[index], sizeof(LightRule));
    else
        memset(&rule, 0, sizeof(LightRule));
}

void LightRuleHIDReport::Process()
{
    if (index < MAX_LIGHT_RULES)
    {
        memcpy(&configuration.lightConfiguration.lightRules[index], &rule, sizeof(LightRule));
        Lights_UpdateConfiguration(&configuration.lightConfiguration);
    }
}

LedMappingHIDReport::LedMappingHIDReport()
{
    index = LIGHT_CONF.selectedLedMappingIndex;
    if (index < MAX_LED_MAPPINGS)
        memcpy(&mapping, &LIGHT_CONF.ledMappings[index], sizeof(LedMapping));
    else
        memset(&mapping, 0, sizeof(LedMapping));
}

void LedMappingHIDReport::Process()
{
    if (index < MAX_LED_MAPPINGS)
    {
        memcpy(&configuration.lightConfiguration.ledMappings[index], &mapping, sizeof(LedMapping));
        Lights_UpdateConfiguration(&configuration.lightConfiguration);
    }
}

LightsHIDReport::LightsHIDReport()
{

}

void LightsHIDReport::Process()
{
    Lights_SetManualMode(true);
    
    for(int i=0; i<MAX_LED_MAPPINGS; i++)
    {
        Lights_SetManual(i, lights[i]);
    }

    Lights_Update(true);
}

void RegisterLightReports()
{
    REGISTER_REPORT(LIGHT_RULE_REPORT_ID, LightRuleHIDReport)
    REGISTER_REPORT(LED_MAPPING_REPORT_ID, LedMappingHIDReport)
    REGISTER_REPORT_PROCESS(LIGHTS_REPORT_ID, LightsHIDReport)
}
