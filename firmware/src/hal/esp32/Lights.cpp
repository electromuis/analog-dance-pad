#include <Arduino.h>
#include "adp_config.hpp"
#include "hal/hal_Lights.hpp"
#include <FastLED.h>

#define BRIGHTNESS  254
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[LED_COUNT];

void HAL_Lights_Setup()
{
    pinMode(PIN_LED, OUTPUT);

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