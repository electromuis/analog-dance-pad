#include "hal/hal_Lights.hpp"
#include <LUFA/Drivers/Board/LEDs.h>

#define LED_STRIP_PORT PORTC
#define LED_STRIP_DDR  DDRC
#define LED_STRIP_PIN  6

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

void HAL_Lights_Setup()
{
  /*
  PORT_LED_POWER &= ~(1<<PIN_LED_POWER);
  REG_LED_POWER |= (1<<PIN_LED_POWER);

  PORT_LED_DATA &= ~(1<<PIN_LED_DATA);
  REG_LED_DATA |= (1<<PIN_LED_DATA);
  */
}

void HAL_Lights_SetLed(uint8_t index, rgb_color color)
{
    if(index >= LED_COUNT) return;

    LED_COLORS[index] = color;
}

void HAL_Lights_Update()
{
    led_strip_write(LED_COLORS, LED_COUNT);
}

void HAL_Lights_SetHWLed(HWLeds led, bool state)
{
  
    /*
    if(led == HWLed_DATA)
    {
      if(state)
        REG_LED_DATA |= (1<<PIN_LED_DATA);
      else
        REG_LED_DATA &= ~(1<<PIN_LED_DATA);
    }

    if(led == HWLed_POWER)
    {
      if(state)
        REG_LED_POWER |= (1<<PIN_LED_POWER);
      else
        REG_LED_POWER &= ~(1<<PIN_LED_POWER);
    }
    */
}