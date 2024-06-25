#include "adp_config.hpp"
#ifdef USB_MODE_KEYBOARD

#include "hal/hal_USB.hpp"
#include "Modules/ModulePad.hpp"

#include "USB.h"
#include "USBHIDKeyboard.h"
USBHIDKeyboard Keyboard;

#include <Arduino.h>

bool lastButtonState[BUTTON_COUNT] = {false};
char keyboardKeys[BUTTON_COUNT] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'};

// Exports

void HAL_USB_Setup()
{  
  Keyboard.begin();
  USB.begin();
}

void HAL_USB_Update()
{
    for(int b=0; b<BUTTON_COUNT; b++)
    {
        if(ModulePadInstance.buttonsPressed[b] != lastButtonState[b])
        {
            lastButtonState[b] = ModulePadInstance.buttonsPressed[b];
            if(ModulePadInstance.buttonsPressed[b])
            {
                Keyboard.press(keyboardKeys[b]);
            }
            else
            {
                Keyboard.release(keyboardKeys[b]);
            }
        }
    }
}

#endif