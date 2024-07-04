#include <Arduino.h>
#include "adp_config.hpp"
#include "hal/hal_Lights.hpp"
#include "Modules/ModuleLights.hpp"
#include <FastLED.h>

#define BRIGHTNESS  254
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[LED_COUNT];

void HAL_Lights_Setup()
{
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_LED_POWER, OUTPUT);

    FastLED.addLeds<LED_TYPE, PIN_LED, COLOR_ORDER>(leds, LED_COUNT).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
}

void HAL_Lights_SetLed(uint8_t index, rgb_color color)
{
    leds[index].r = color.red;
    leds[index].g = color.green;
    leds[index].b = color.blue;
}

void HAL_Lights_Update()
{
    FastLED.show();
}

void HAL_Lights_SetHWLed(HWLeds led, bool state)
{
    if(led == HWLed_DATA)
        digitalWrite(PIN_LED_DATA, !state);

    if(led == HWLed_POWER)
        digitalWrite(PIN_LED_POWER, !state);
}