// Skip every x amount of light updates to improve polling rate
#define UPDATE_WAIT_CYCLES 10

#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  6

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "Pad.h"
#include "Lights.h"

// led_strip_write sends a series of colors to the LED strip, updating the LEDs.
// The colors parameter should point to an array of rgb_color structs that hold
// the colors to send.

void __attribute__((noinline)) led_strip_write(rgb_color * colors, uint16_t count)
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
  LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  
  while (count--)
  {
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile (
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0\n"
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "ld __tmp_reg__, -%a0\n"
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "ld __tmp_reg__, %a0+\n"
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
        "ret\n"

        // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line high for some time.
        "send_led_strip_bit%=:\n"

        "sbi %2, %3\n"                           // Drive the line high.
        "rol __tmp_reg__\n"                      // Rotate left through carry

        "nop\n" "nop\n"

        "brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.
        "brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors)
        : "0" (colors),         // %a0 points to the next color to display
          "I" (_SFR_IO_ADDR(LED_STRIP_PORT)),   // %2 is the port register (e.g. PORTC)
          "I" (LED_STRIP_PIN)     // %3 is the pin number (0-8)
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
}

LightConfiguration LIGHT_CONF;

static rgb_color LED_COLORS[LED_COUNT];

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Lights_UpdateConfiguration(const LightConfiguration* lightConfiguration) {
    memcpy(&LIGHT_CONF, lightConfiguration, sizeof (LightConfiguration));
	Lights_Update();
}

int updateWait = 0;

void Lights_Update()
{
	if(updateWait > 0) {
		updateWait --;
		return;
	}
	
	for (uint8_t led = 0; led < LED_COUNT; ++led) {
		LED_COLORS[led] = (rgb_color) {0, 0, 0};
	}
	
	updateWait = UPDATE_WAIT_CYCLES;
	
	for (uint8_t m = 0; m < MAX_LED_MAPPINGS; ++m)
	{
        const LedMapping* mapping = &LIGHT_CONF.ledMappings[m];

        if (!(mapping->flags & LMF_ENABLED))
            continue;

        const LightRule* rule = &LIGHT_CONF.lightRules[mapping->lightRuleIndex];

        if (!(rule->flags & LRF_ENABLED))
            continue;
		
		uint16_t sensorValue = PAD_STATE.sensorValues[mapping->sensorIndex];
		uint16_t sensorThreshold = PAD_CONF.sensorThresholds[mapping->sensorIndex];
		
		bool sensorState = sensorValue > sensorThreshold;
		
		rgb_color color;
		
		if(sensorState == true) {
			if(rule->flags & LRF_FADE_ON) {
				uint16_t sensorThreshold2 = sensorThreshold * 2;
				
				if(sensorValue <= sensorThreshold2) {				
					color = (rgb_color) {	
						map(sensorValue, sensorThreshold, sensorThreshold2, rule->onColor.red,   rule->onFadeColor.red),
						map(sensorValue, sensorThreshold, sensorThreshold2, rule->onColor.green, rule->onFadeColor.green),
						map(sensorValue, sensorThreshold, sensorThreshold2, rule->onColor.blue,  rule->onFadeColor.blue)
					};
				}
				else {
					color = rule->onFadeColor;
				}
			}
			else {
				color = rule->onColor;
			}
		}
		else {
			if(rule->flags & LRF_FADE_OFF) {
				color = (rgb_color) {	
					map(sensorValue, 0, sensorThreshold, rule->offColor.red,   rule->offFadeColor.red),
					map(sensorValue, 0, sensorThreshold, rule->offColor.green, rule->offFadeColor.green),
					map(sensorValue, 0, sensorThreshold, rule->offColor.blue,  rule->offFadeColor.blue)
				};
			}
			else {
				color = rule->offColor;
			}
		}
		
		for (uint8_t led = mapping->ledIndexBegin; led < mapping->ledIndexEnd; ++led) {
            LED_COLORS[led] = color;
		}
	}
	
	led_strip_write(LED_COLORS, LED_COUNT);
}
