#pragma once

#include <stdint.h>

void HAL_USB_Setup();
void HAL_USB_Update();

bool USB_CreateReport(uint8_t report_id, uint8_t* buffer, uint16_t* len);
bool USB_ProcessReport(uint8_t report_id, const uint8_t* buffer, uint16_t len);
uint16_t USB_WriteDescriptor(uint8_t* buffer);
uint16_t USB_DescriptorSize();