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

int updateWait = 0;

typedef struct
{
    uint16_t decay;
	rgb_color color;
} PulseStatus;

PulseStatus pulseStatus[MAX_LED_MAPPINGS];

bool softwareTriggered[MAX_LED_MAPPINGS];

PadState LAST_PAD_STATE;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

rgb_color map_color(long x, long in_min, long in_max, rgb_color min_color, rgb_color max_color)
{
	return (rgb_color) {	
		map(x, in_min, in_max, min_color.red,   max_color.red),
		map(x, in_min, in_max, min_color.green, max_color.green),
		map(x, in_min, in_max, min_color.blue,  max_color.blue)
	};
}

void Lights_UpdateConfiguration(const LightConfiguration* lightConfiguration) {
    memcpy(&LIGHT_CONF, lightConfiguration, sizeof (LightConfiguration));
	Lights_Update(true);
}

void Lights_Timer_Tick()
{
	if(updateWait > 0) {
		updateWait --;
	}
	
	for(int i = 0; i < MAX_LED_MAPPINGS; i ++) {
		if(pulseStatus[i].decay > 0) {
			pulseStatus[i].decay --;
		}
	}
}

void Lights_Trigger_Mapping(int lightMappingIndex)
{
	if(LIGHT_CONF.lightMode != LM_SOFTWARE) {
		return;
	}
	
	if(lightMappingIndex < 0 || lightMappingIndex > MAX_LED_MAPPINGS)
		return;
	
	softwareTriggered[lightMappingIndex] = true;
	
	Lights_Update(true);
}

void Lights_Update(bool forceUpdate)
{
	if(!forceUpdate && updateWait > 0) {
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
			
		if(mapping->sensorIndex < 0 || mapping->sensorIndex > SENSOR_COUNT)
			continue;
		
		uint16_t sensorValue = PAD_STATE.sensorValues[mapping->sensorIndex];
		uint16_t sensorThreshold = PAD_CONF.sensorThresholds[mapping->sensorIndex];
		
		bool newColor = false;
		rgb_color color;
		
		if(LIGHT_CONF.lightMode == LM_HARDWARE) {
			if(PAD_STATE.sensorPressed[mapping->sensorIndex]) {
				if(rule->flags & LRF_FADE_ON) {
					uint16_t sensorThreshold2 = sensorThreshold * 2;
					
					if(sensorValue <= sensorThreshold2) {
						color = map_color(sensorValue, sensorThreshold, sensorThreshold2, rule->onColor, rule->onFadeColor);
					}
					else {
						color = rule->onFadeColor;
					}
					
					newColor = true;
				}
				else if(rule->flags & LRF_PULSE_ON) {
					if(!LAST_PAD_STATE.sensorPressed[mapping->sensorIndex]) {
						pulseStatus[m].decay = LIGHT_PULSE_LENGTH;
						pulseStatus[m].color = rule->onColor;
					}
				}
				else {
					color = rule->onColor;
					
					newColor = true;
				}
			}
			else {
				if(rule->flags & LRF_FADE_OFF) {
					color = map_color(sensorValue, 0, sensorThreshold, rule->offColor, rule->offFadeColor);
					
					newColor = true;
				}
				else if(rule->flags & LRF_PULSE_OFF) {
					if(LAST_PAD_STATE.sensorPressed[mapping->sensorIndex]) {
						//pulseStatus[m].decay = LIGHT_PULSE_LENGTH;
						//pulseStatus[m].color = rule->offColor;
					}
				}
				else {
					color = rule->offColor;
					
					newColor = true;
				}
			}
		}
		else if(LIGHT_CONF.lightMode == LM_SOFTWARE && softwareTriggered[m]) {
			if((rule->flags & (LRF_PULSE_ON))) {
				pulseStatus[m].decay = LIGHT_PULSE_LENGTH;
				pulseStatus[m].color = rule->onColor;
			}
			
			softwareTriggered[m] = false;
		}
		
		if((rule->flags & (LRF_PULSE_ON | LRF_PULSE_OFF)) && pulseStatus[m].decay > 0) {
			color = map_color(pulseStatus[m].decay, 0, LIGHT_PULSE_LENGTH, (rgb_color){0,0,0}, pulseStatus[m].color);
			newColor = true;
			
			/*
			int mappingPixels = mapping->ledIndexEnd - mapping->ledIndexBegin;
			int pixelCountHalf = mappingPixels / 2;
			
			int pixelDecay = 4;
			int pixelPos = map(pulseStatus[m].decay, LIGHT_PULSE_LENGTH, 0, 0, pixelCountHalf + pixelDecay);
			
			for(int d = 0; d < pixelDecay; d ++) {
				int currentPos = pixelPos - d;
				if(currentPos < 0) {
					break;
				}
				
				if(currentPos > pixelCountHalf - 1) {
					continue;
				}
				
				color = map_color(d, 0, pixelDecay, pulseStatus[m].color, (rgb_color){0,0,0});
				LED_COLORS[mapping->ledIndexBegin + currentPos + pixelCountHalf] = color;
				LED_COLORS[mapping->ledIndexBegin - currentPos + pixelCountHalf - 1] = color;
			}
			
			// We already set the color manually
			newColor = false;
			*/
		}
		
		if(newColor) {
			for (uint8_t led = mapping->ledIndexBegin; led < mapping->ledIndexEnd; ++led) {
				LED_COLORS[led] = color;
			}
		}
	}
	
	led_strip_write(LED_COLORS, LED_COUNT);
	
	memcpy(&LAST_PAD_STATE, &PAD_STATE, sizeof(PadState));
}
