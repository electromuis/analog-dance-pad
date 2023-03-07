#include "ReportsInternal.h"
#include "LightReports.h"

LightRuleHIDReport::LightRuleHIDReport()
{
    index = LIGHT_CONF.selectedLightRuleIndex;
    if (report->index < MAX_LIGHT_RULES)
        memcpy(&rule, &LIGHT_CONF.lightRules[report->index], sizeof(LightRule));
    else
        memset(&rule, 0, sizeof(LightRule));
}

void LightRuleHIDReport::Process()
{

}

LedMappingHIDReport::LedMappingHIDReport()
{
    index = LIGHT_CONF.selectedLedMappingIndex;
    if (index < MAX_LED_MAPPINGS)
        memcpy(&mapping, &LIGHT_CONF.ledMappings[report->index], sizeof(LedMapping));
    else
        memset(&mapping, 0, sizeof(LedMapping));
}

void LedMappingHIDReport::Process()
{
    
}

void RegisterLightReports()
{
    REGISTER_REPORT(LIGHT_RULE_REPORT_ID, LightRuleHIDReport)
    REGISTER_REPORT(LED_MAPPING_REPORT_ID, LedMappingHIDReport)
}
