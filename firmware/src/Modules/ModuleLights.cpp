#include "hal/hal_Lights.hpp"
#include "ModuleLights.hpp"
#include "ModulePad.hpp"
#include "ModuleConfig.hpp"

ModuleLights ModuleLightsInstance = ModuleLights();

void ModuleLights::Setup()
{
    HAL_Lights_Setup();
    HAL_Lights_SetHWLed(HWLeds::HWLed_POWER, true);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

rgb_color CalcColor(const LightRule& rule, bool sensorState, uint16_t sensorValue, uint16_t sensorThreshold)
{
    rgb_color color;
		
    if(sensorState == true) {
        if(rule.flags & LRF_FADE_ON) {
            uint16_t sensorThreshold2 = sensorThreshold * 2;
            
            if(sensorValue <= sensorThreshold2) {				
                color = (rgb_color) {	
                    map(sensorValue, sensorThreshold, sensorThreshold2, rule.onColor.red,   rule.onFadeColor.red),
                    map(sensorValue, sensorThreshold, sensorThreshold2, rule.onColor.green, rule.onFadeColor.green),
                    map(sensorValue, sensorThreshold, sensorThreshold2, rule.onColor.blue,  rule.onFadeColor.blue)
                };
            }
            else {
                color = rule.onFadeColor;
            }
        }
        else {
            color = rule.onColor;
        }
    }
    else {
        if(rule.flags & LRF_FADE_OFF) {
            color = (rgb_color) {	
                map(sensorValue, 0, sensorThreshold, rule.offColor.red,   rule.offFadeColor.red),
                map(sensorValue, 0, sensorThreshold, rule.offColor.green, rule.offFadeColor.green),
                map(sensorValue, 0, sensorThreshold, rule.offColor.blue,  rule.offFadeColor.blue)
            };
        }
        else {
            color = rule.offColor;
        }
    }

    return color;
}

void ModuleLights::Update()
{
    if(ledTimer > 0)
    {
        ledTimer--;
        return;
    }

    ledTimer = LED_UPDATE_INTERVAL;

    bool update = false;
    
    for(int i=0; i<LED_COUNT; i++)
    {
        HAL_Lights_SetLed(i, {0, 0, 0});
    }

    for (uint8_t m = 0; m < MAX_LED_MAPPINGS; ++m)
	{
        const LedMapping& mapping = configuration.lightConfiguration.ledMappings[m];

        if (!(mapping.flags & LMF_ENABLED) || mapping.lightRuleIndex >= MAX_LIGHT_RULES || mapping.sensorIndex >= SENSOR_COUNT)
            continue;

        const LightRule& rule = configuration.lightConfiguration.lightRules[mapping.lightRuleIndex];

        if (!(rule.flags & LRF_ENABLED))
            continue;

        update = true;

        SensorConfig s = ModuleConfigInstance.GetSensorConfig(mapping.sensorIndex);
		uint16_t sensorValue = ModulePadInstance.sensorValues[mapping.sensorIndex];
		uint16_t sensorThreshold = s.threshold;		
		bool sensorState = ModulePadInstance.sensorStates[mapping.sensorIndex];


        for (uint8_t led = mapping.ledIndexBegin; led < mapping.ledIndexEnd; ++led) {
            HAL_Lights_SetLed(led, CalcColor(rule, sensorState, sensorValue, sensorThreshold));
		}
    }

    if(update)
        HAL_Lights_Update();
}

void ModuleLights::SetManualMode(bool mode)
{

}

void ModuleLights::SetManual(int index, rgb_color color)
{

}

void ModuleLights::DataCycle()
{
    if(dataCycleCount == 0)
    {
        HAL_Lights_SetHWLed(HWLeds::HWLed_DATA, true);
        dataCycleCount = 100;
        return;
    }

    if(dataCycleCount == 50)
        HAL_Lights_SetHWLed(HWLeds::HWLed_DATA, false);

    dataCycleCount--;
}
