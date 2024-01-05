#include "../hal_Bootloader.hpp"

#include "esp32-hal-tinyusb.h"

void HAL_Bootloader()
{
    usb_persist_restart(RESTART_BOOTLOADER);
}