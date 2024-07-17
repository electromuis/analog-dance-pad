#include "adp_config.hpp"
#include "ModuleUSB.hpp"
#include "ModuleLights.hpp"
#include "hal/hal_USB.hpp"
#include "Reports/Reports.hpp"
#include "ModulePad.hpp"
#include "ModuleDebug.hpp"
#include <string.h>

// #ifdef FEATURE_RTOS_ENABLED
#include "FreeRTOS.h"
#include "task.h"
// #endif

ModuleUSB ModuleUSBInstance = ModuleUSB();
uint8_t USBDescriptor[512];
uint16_t USBDescriptorSize = 0;

// #ifdef FEATURE_RTOS_ENABLED
void inputLoop(void *pvParameter)
{
    while (true) {
        ModuleUSBInstance.Update();
    }
}
// #endif

void ModuleUSB::Setup()
{
    HAL_USB_Setup();

    // #ifdef FEATURE_RTOS_ENABLED
    taskPrority = 18;
    // xTaskCreatePinnedToCore(
    //     inputLoop,
    //     "inputLoop",
    //     1024 * 4,
    //     ( void * ) 1,
    //     taskPrority,
    //     nullptr,
    //     1
    // );
    // #endif

    xTaskCreate(inputLoop, "inputLoop", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
}

void ModuleUSB::Update()
{
    ModulePadInstance.UpdateStatus();
    if(HAL_USB_Update())
    {
        ModuleLightsInstance.DataCycle();
        if(errCount > 0)
            ModuleDebugInstance.Write("HAL_USB_Update Recovered");
        errCount = 0;
    }
    else
    {
        if(errCount == 0)
            ModuleDebugInstance.Write("HAL_USB_Update Error");
            
        errCount ++;

        // ESP usb has probably crashed, try to reconnect
        if(errCount == 2000)
        {
            ModuleDebugInstance.Write("HAL_USB_Update Error limit. Restarting");

            // Reconnect();
            // errCount = 0;
        }
    }
}

void ModuleUSB::Reconnect()
{
    ModuleLightsInstance.SetPowerLed(false);
    HAL_USB_Reconnect();
    ModuleLightsInstance.SetPowerLed(true);
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
        USBDescriptorSize = WriteDescriptor(USBDescriptor);
    }

    return USBDescriptorSize;
}