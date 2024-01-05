#include "ModuleUSB.hpp"
#include "hal/hal_USB.hpp"
#include "Reports/Reports.hpp"
#include <cstring>
#include <Arduino.h>

ModuleUSB ModuleUSBInstance = ModuleUSB();
uint8_t USBDescriptor[512];
uint16_t USBDescriptorSize = 0;

void ModuleUSB::Setup()
{
    // Serial.begin(9600);

    // while(!Serial) { ; }

    // delay(1000);

    // Serial.println("Start");
    // for (size_t i = 0; i < USB_DescriptorSize(); i++)
    // {
    //     Serial.print(USBDescriptor[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println("End");
    

    HAL_USB_Setup();
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

uint16_t USB_DescriptorSize()
{
    if(USBDescriptorSize == 0) {
        RegisterReports();
        USBDescriptorSize = WriteDescriptor(USBDescriptor);
    }

    return USBDescriptorSize;
}