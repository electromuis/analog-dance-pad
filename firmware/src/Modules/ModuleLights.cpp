#include "hal/hal_Lights.hpp"
#include "ModuleLights.hpp"
#include "ModulePad.hpp"
#include "ModuleConfig.hpp"
#include <Arduino.h>

#ifdef FEATURE_RTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
#endif

ModuleLights ModuleLightsInstance = ModuleLights();


void lightLoop(void *pvParameter)
{
    while (true) {
        ModuleLightsInstance.Update();
        vTaskDelay(5);
    }
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

rgb_color mapColor(long x, long in_min, long in_max, rgb_color col1, rgb_color col2) {
    return (rgb_color) {	
        map(x, in_min, in_max, col1.red,   col2.red),
        map(x, in_min, in_max, col1.green, col2.green),
        map(x, in_min, in_max, col1.blue,  col2.blue)
    };
}

bool operator==(const rgb_color& a, const rgb_color& b)
{
    return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

bool operator!=(const rgb_color& a, const rgb_color& b)
{
    return !(a == b);
}

// LedMappingWrapper

LedMappingWrapper::LedMappingWrapper(uint8_t i)
    :mappingIndex(i)
{

}

bool LedMappingWrapper::HasFlag(LightRuleFlags f)
{
    return GetRule().flags & f;
}

const LedMapping& LedMappingWrapper::GetMapping()
{
    return configuration.lightConfiguration.ledMappings[mappingIndex];
}

const LightRule& LedMappingWrapper::GetRule()
{
    return configuration.lightConfiguration.lightRules[GetMapping().lightRuleIndex];
}

SensorConfig LedMappingWrapper::GetSensorConfig()
{
    return ModuleConfigInstance.GetSensorConfig(GetMapping().sensorIndex);
}

bool LedMappingWrapper::IsValid()
{
    auto& mapping = GetMapping();

    if (
            !(mapping.flags & LMF_ENABLED) ||
            mapping.lightRuleIndex >= MAX_LIGHT_RULES ||
            mapping.sensorIndex >= SENSOR_COUNT ||
            mapping.ledIndexBegin > LED_COUNT ||
            mapping.ledIndexEnd > LED_COUNT
    ) return false;

    if (!HasFlag(LRF_ENABLED))
        return false;

    return true;
}


rgb_color LedMappingWrapper::CalcColor()
{
    auto& mapping = GetMapping();
    const LightRule& rule = GetRule();

    SensorConfig s = ModuleConfigInstance.GetSensorConfig(mapping.sensorIndex);
    uint16_t sensorValue = ModulePadInstance.sensorValues[mapping.sensorIndex];
    uint16_t sensorThreshold = s.threshold;		
    bool sensorState = ModulePadInstance.sensorStates[mapping.sensorIndex];
    
    rgb_color color;
		
    if(sensorState == true) {
        if(HasFlag(LRF_FADE_ON)) {
            uint16_t sensorThreshold2 = sensorThreshold * 2;
            
            if(sensorValue <= sensorThreshold2) {
                color = mapColor(sensorValue, sensorThreshold, sensorThreshold2, rule.onColor, rule.onFadeColor);
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
        if(HasFlag(LRF_FADE_OFF)) {
            color = mapColor(sensorValue, 0, sensorThreshold, rule.offColor,   rule.offFadeColor);
        }
        else {
            color = rule.offColor;
        }
    }

    return color;
}

#define PULSE_TICK_RATE 20

bool LedMappingWrapper::ApplyPulse()
{
    auto& mapping = GetMapping();
    auto& rule = GetRule();

    bool sensorState = ModulePadInstance.sensorStates[mapping.sensorIndex];
    unsigned long now = millis();

    if(sensorState != lastState) {
        lastState = sensorState;

        if(lastState)
            pulses.push_back(now);
    }

    Fill(rule.offColor);

    for(unsigned long pulseTime : pulses) {
        unsigned long pulseDuration = now - pulseTime;
        float pulsePosition = (float)pulseDuration / (float)PULSE_TICK_RATE;
        if(pulsePosition >= 20)
            continue;

        uint16_t panelSize = (mapping.ledIndexEnd - mapping.ledIndexBegin) / 2;
        uint16_t ledCenter = mapping.ledIndexBegin + panelSize;

        uint16_t pulseLed = map(pulsePosition, 0, 16, 0, panelSize);
        pulseLed = (pulseLed/2)*2;

        if(pulsePosition < 15) {
            HAL_Lights_SetLed(ledCenter + pulseLed, rule.onColor);
            HAL_Lights_SetLed(ledCenter + pulseLed+1, rule.onColor);

            HAL_Lights_SetLed(ledCenter + ((pulseLed)*-1)-1, rule.onColor);
            HAL_Lights_SetLed(ledCenter + ((pulseLed+1)*-1)-1, rule.onColor);
        }


        if(pulseLed > 0) {
            rgb_color color_mid = mapColor(1, 0, 2, rule.onColor, rule.offColor);
            
            HAL_Lights_SetLed(ledCenter + pulseLed - 1, color_mid);
            HAL_Lights_SetLed(ledCenter + pulseLed - 1 - 1, color_mid);

            HAL_Lights_SetLed(ledCenter + ((pulseLed - 1)*-1)-1, color_mid);
            HAL_Lights_SetLed(ledCenter + ((pulseLed - 1 - 1)*-1)-1, color_mid);
        }
        
    }

    std::vector<std::reference_wrapper<unsigned long>> fv;
    std::copy_if(pulses.begin(), pulses.end(), std::back_inserter(fv), [now](unsigned long x){
        return ((now - x) / PULSE_TICK_RATE) < 30;
    });

    return true;
}

void LedMappingWrapper::Fill(rgb_color c)
{
    auto& mapping = GetMapping();

    for (uint8_t led = mapping.ledIndexBegin; led < mapping.ledIndexEnd; ++led) {
        HAL_Lights_SetLed(led, c);
    }
}

bool LedMappingWrapper::Apply()
{
    if(HasFlag(LRF_PULSE)) {
        return ApplyPulse();
    }
    
    auto& mapping = GetMapping();
    rgb_color newColor;

    if(newColor != lastColor) {
        lastColor = newColor;

        Fill(newColor);

        return true;
    }

    return false;
}

// ModuleLights


void ModuleLights::Setup()
{
    for(uint8_t i=0; i<MAX_LED_MAPPINGS; i++) {
        wrappers[i] = LedMappingWrapper(i);
    }

    HAL_Lights_Setup();
    HAL_Lights_SetHWLed(HWLeds::HWLed_POWER, true);
    taskPrority = 5;

#ifdef FEATURE_RTOS_ENABLED
    xTaskCreatePinnedToCore(
        lightLoop,
        "lightLoop",
        1024 * 2,
        ( void * ) 1,
        taskPrority,
        nullptr,
        0
    );
#endif
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
        LedMappingWrapper& w = wrappers[m];
        if(!w.IsValid())
            continue;

        if(w.Apply())
            update = true;
    }

    if(update)
    {
        writing = true;
        HAL_Lights_Update();
        writing = false;
    }
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
