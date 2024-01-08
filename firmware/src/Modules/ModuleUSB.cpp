#include "adp_config.hpp"
#include "ModuleUSB.hpp"
#include "hal/hal_USB.hpp"
#include "Reports/Reports.hpp"
#include "Modules/ModulePad.hpp"
#include <string.h>

#ifdef FEATURE_RTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
// TaskHandle_t usbTask;
#endif

ModuleUSB ModuleUSBInstance = ModuleUSB();
uint8_t USBDescriptor[512];
uint16_t USBDescriptorSize = 0;

void inputLoop(void *pvParameter)
{
    while (true) {
        ModulePadInstance.UpdateStatus();
        HAL_USB_Update();
    }
}

void ModuleUSB::Setup()
{
    HAL_USB_Setup();

    #ifdef FEATURE_RTOS_ENABLED
    xTaskCreatePinnedToCore(
        inputLoop,
        "inputLoop",
        1024 * 2,
        ( void * ) 1,
        5,
        nullptr,
        1
    );
    #endif
}

void ModuleUSB::Update()
{
    #ifndef FEATURE_RTOS_ENABLED
    HAL_USB_Update();
    #endif
}

void ModuleUSB::Reconnect()
{

}

bool USB_CreateReport(uint8_t report_id, uint8_t* buffer, uint16_t* len)
{
    return WriteReport(report_id, buffer, len);
}

bool USB_ProcessReport(uint8_t report_id, const uint8_t* buffer, uint16_t len)
{
    return ProcessReport(report_id, buffer, len);
}

uint16_t USB_WriteDescriptor(uint8_t* buffer)
{
    memcpy(buffer, USBDescriptor, USBDescriptorSize);
    return USBDescriptorSize;
}

uint8_t* USB_GetDescriptor()
{
    USB_DescriptorSize();
    return USBDescriptor;
}


uint16_t USB_DescriptorSize()
{
    if(USBDescriptorSize == 0) {
        RegisterReports();
        USBDescriptorSize = WriteDescriptor(USBDescriptor);
    }

    return USBDescriptorSize;
}