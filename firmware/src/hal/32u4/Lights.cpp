#include "adp_config.hpp"
#include "hal/hal_Lights.hpp"
#include <LUFA/Drivers/Board/LEDs.h>

extern "C" {
#include <avr/interrupt.h>
#include <avr/io.h>
}

static rgb_color LED_COLORS[LED_COUNT];

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

        //"nop\n" "nop\n"
        //"brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.
        //"brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

        "nop\n" "nop\n"
        "brcs .+2\n" "cbi %2, %3\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "brcc .+2\n" "cbi %2, %3\n"

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

#define LED_BOOT LEDS_LED2
#define LED_DATA LEDS_LED1

void HAL_Lights_Setup()
{
  LEDs_Init();
	LEDs_SetAllLEDs(LEDS_ALL_LEDS);
	Delay_MS(200);
	LEDs_SetAllLEDs(LED_BOOT);
}

bool hasUpdate = false;

void HAL_Lights_SetLed(uint8_t index, rgb_color color)
{
#ifdef FEATURE_LIGHTS_CHANNELS

    uint8_t colorIndex = index / 3;
    if(colorIndex >= LED_COUNT) return;

    uint16_t val = ((uint16_t)color.red + (uint16_t)color.green + (uint16_t)color.blue) / 3;
    uint8_t* colorPtr = (uint8_t*)LED_COLORS + index;
    *colorPtr = (uint8_t)val;
    
#else

    if(index >= LED_COUNT) return;
    if(LED_COLORS[index].red == color.red && LED_COLORS[index].green == color.green && LED_COLORS[index].blue == color.blue) return;
    hasUpdate = true;
    LED_COLORS[index] = color;

#endif
}

void HAL_Lights_Update()
{
    if(!hasUpdate) return;

    led_strip_write(LED_COLORS, LED_COUNT);
    
    hasUpdate = false;
}

uint8_t ledState = 0;

void HAL_Lights_SetHWLed(HWLeds led, bool state)
{
    if(led == HWLed_DATA)
    {
      LEDs_TurnOnLEDs(LED_DATA);
     
      
      if(state == true)
        ledState |= LED_DATA;
      else
        ledState &= ~LED_DATA;
      
     
    }

    if(led == HWLed_POWER)
    {
      if(state)
        ledState |= LED_BOOT;
      else
        ledState &= ~LED_BOOT;
    }

    LEDs_SetAllLEDs(ledState);
}