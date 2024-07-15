#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void HAL_USB_Setup();
bool HAL_USB_Update();
void HAL_USB_Reconnect();

bool USB_CreateReport(uint8_t report_id, uint8_t* buffer, uint16_t* len);
bool USB_ProcessReport(uint8_t report_id, const uint8_t* buffer, uint16_t len);

uint16_t USB_WriteDescriptor(uint8_t* buffer);
uint8_t* USB_GetDescriptor();
uint16_t USB_DescriptorSize();

#ifdef __cplusplus
}
#endif
