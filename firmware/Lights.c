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

rgb_color colors[LED_COUNT];

LightConfiguration LIGHT_CONF;

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
	
	updateWait = UPDATE_WAIT_CYCLES;
	
	for (uint8_t s = 0; s < MAX_LIGHT_RULES; s++)
	{
		LightRule light = LIGHT_CONF.lightRules[s];
		
		if(light.sensorNumber == 0 || (light.flags & LRF_DISABLED)) {
			continue;
		}
		
		int8_t sensor = light.sensorNumber - 1;
		
		uint16_t sensorValue = PAD_STATE.sensorValues[sensor];
		uint16_t sensorThreshold = PAD_CONF.sensorThresholds[sensor];
		
		bool sensorState = sensorValue > sensorThreshold;
		
		rgb_color color;
		
		if(sensorState == true) {
			if(light.flags & LRF_FADE_ON) {
				uint16_t sensorThreshold2 = sensorThreshold * 2;
				
				if(sensorValue <= sensorThreshold2) {				
					color = (rgb_color) {	
						map(sensorValue, sensorThreshold, sensorThreshold2, light.onColor.red, 		light.onFadeColor.red),
						map(sensorValue, sensorThreshold, sensorThreshold2, light.onColor.green,	light.onFadeColor.green),
						map(sensorValue, sensorThreshold, sensorThreshold2, light.onColor.blue, 	light.onFadeColor.blue)
					};
				}
				else {
					color = light.onFadeColor;
				}
			}
			else {
				color = light.onColor;
			}
		}
		else {
			if(light.flags & LRF_FADE_OFF) {
				color = (rgb_color) {	
					map(sensorValue, 0, sensorThreshold, light.offColor.red, 	light.offFadeColor.red),
					map(sensorValue, 0, sensorThreshold, light.offColor.green,	light.offFadeColor.green),
					map(sensorValue, 0, sensorThreshold, light.offColor.blue, 	light.offFadeColor.blue)
				};
			}
			else {
				color = light.offColor;
			}
		}
		
		for (uint8_t led = light.fromLight; led < light.toLight; led ++) {
			colors[led] = color;
		}
	}
	
	led_strip_write(colors, LED_COUNT);
}
