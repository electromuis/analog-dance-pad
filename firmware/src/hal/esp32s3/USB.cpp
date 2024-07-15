#include "adp_config.hpp"
#ifdef USB_MODE_HID

#include <Arduino.h>
#include <mutex>

#include <USB.h>
#include <USBHID.h>

#include "hal/hal_USB.hpp"
#include "esp32-hal-tinyusb.h"
#include "Reports/Reports.hpp"
#include "Reports/PadReports.hpp"
#include "Modules/ModuleDebug.hpp"
#include "esp32-hal-tinyusb.h"

#include "driver/periph_ctrl.h"

class USBPad: public USBHIDDevice {
public:
    USBPad();
    void begin();
    void end();

    bool sendInputReport();

    // internal use
    uint16_t _onGetDescriptor(uint8_t* buffer);
    virtual uint16_t _onGetFeature(uint8_t report_id, uint8_t* buffer, uint16_t len) override;
    virtual void _onSetFeature(uint8_t report_id, const uint8_t* buffer, uint16_t len) override;
    virtual void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) override;

private:
    bool sendInputReportInternal();
    bool connected = false;
    USBHID hid;
    std::mutex usbMutex;
};

static bool suspended = false;
static int startCount = 0;

USBPad::USBPad(): hid()
{
    
}

void USBPad::begin()
{
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        hid.addDevice(this, USB_DescriptorSize());

        ModuleDebugInstance.Write("USB HID initialized");
    }

    hid.begin();
    ModuleDebugInstance.Write("USB HID started");
    USB.onEvent([](void* event_handler_arg,
                                        esp_event_base_t event_base,
                                        int32_t event_id,
                                        void* event_data){
        arduino_usb_hid_event_data_t * data = (arduino_usb_hid_event_data_t *)event_data;
        switch (event_id)
        {
            case ARDUINO_USB_STARTED_EVENT:
                ModuleDebugInstance.Write("USB Start event");
                startCount++;
                if(startCount == 2)
                {
                    ModuleDebugInstance.Write("Double start???");   
                    // usb_persist_restart(RESTART_NO_PERSIST);
                    // periph_module_disable(PERIPH_USB_MODULE);
                    // periph_module_reset(PERIPH_USB_MODULE);
                    // periph_module_enable(PERIPH_USB_MODULE);

                    // startCount = 0;
                }
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                suspended = true;
                ModuleDebugInstance.Write("USB Suspended");
                break;
            case ARDUINO_USB_RESUME_EVENT:
                suspended = false;
                ModuleDebugInstance.Write("USB Resumed");
                break;
            default:
                ModuleDebugInstance.Write("USB Event %d", event_id);
                break;
        }
    });
    
    USB.begin();
    ModuleDebugInstance.Write("USB really started");
}

void USBPad::end()
{
    
}

uint16_t USBPad::_onGetDescriptor(uint8_t* buffer)
{
    return USB_WriteDescriptor(buffer);
}

USBPad usbPad;

bool USBPad::sendInputReportInternal()
{
    if(!tud_hid_n_ready(0))
        return false;
    InputHIDReport report;
    return this->hid.SendReport(INPUT_REPORT_ID, &report, sizeof(report), 1U);
}

bool USBPad::sendInputReport()
{
    bool result = sendInputReportInternal();
    if(result)
    {
        if(!connected)
            ModuleDebugInstance.Write("USB Connected");
        connected = true;
        return true;
    }

    delay(1);

    // Not connected yet, so not raising an error
    if(!connected)
        return true;

    return false;
}

uint16_t USBPad::_onGetFeature(uint8_t report_id, uint8_t* buffer, uint16_t len)
{
    ModuleDebugInstance.Write("USB Get Feature");
    bool result = USB_CreateReport(report_id, buffer, &len);
    if(result) {
        return len;
    }
    return 0;
}

void USBPad::_onSetFeature(uint8_t report_id, const uint8_t* buffer, uint16_t len)
{
    ModuleDebugInstance.Write("USB Set Feature");
    USB_ProcessReport(report_id, buffer, len);
}

void USBPad::_onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len)
{
    ModuleDebugInstance.Write("USB Output");
    USB_ProcessReport(report_id, buffer, len);
}

// Exports

void HAL_USB_Setup(){
    USB.VID(0x1209);
    USB.PID(0xB196);
    USB.productName("FSR Mini pad");
    USB.manufacturerName("DDR-EXP");

    usbPad.begin();
}

bool HAL_USB_Update()
{
    if(suspended)
    {
        delay(50);
        return true;
    }

    return usbPad.sendInputReport();
}

void HAL_USB_Reconnect()
{
    esp_restart();
}

#endif