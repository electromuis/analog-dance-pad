#include "Reports/PadReports.hpp"
#include "Reports/Reports.hpp"
#include "tusb.h"
// #include "bsp/board_api.h"
#include "Modules/ModuleDebug.hpp"

#include "hal/usb_hal.h"
#include "hal/gpio_ll.h"
#include "soc/usb_struct.h"
#include "soc/usb_reg.h"
#include "soc/usb_wrap_reg.h"
#include "soc/usb_wrap_struct.h"
#include "soc/usb_periph.h"

#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#include "hal/hal_USB.hpp"
#include <string.h>
#include <mutex>
#include <Arduino.h>

extern "C" {

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x1209,
    .idProduct          = 0xB196,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 100),

  // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
  TUD_HID_DESCRIPTOR(1, 0, HID_ITF_PROTOCOL_NONE, 99, 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

typedef char tusb_str_t[127];
static tusb_str_t USB_DEVICE_PRODUCT      = DEFAULT_NAME;
static tusb_str_t USB_DEVICE_MANUFACTURER = "DDR-EXP";
static tusb_str_t USB_DEVICE_SERIAL       = "";
static tusb_str_t USB_DEVICE_LANGUAGE     = "\x09\x04";//English (0x0409)

#define MAX_STRING_DESCRIPTORS 20
static uint32_t tinyusb_string_descriptor_len = 4;
static char * tinyusb_string_descriptor[MAX_STRING_DESCRIPTORS] = {
        // array of pointer to string descriptors
        USB_DEVICE_LANGUAGE,    // 0: is supported language
        USB_DEVICE_MANUFACTURER,// 1: Manufacturer
        USB_DEVICE_PRODUCT,     // 2: Product
        USB_DEVICE_SERIAL,      // 3: Serials, should use chip ID
};

bool hid_task(void)
{
  if(tud_hid_n_ready(0))
  {

    // static uint32_t lastReportTime = 0;
    // uint32_t currentTime = millis();

    // if(lastReportTime == currentTime)
    //   return true;

    // lastReportTime = currentTime;

    InputHIDReport report;
    return tud_hid_n_report(0, INPUT_REPORT_ID, &report, sizeof(report));
  }

  return true;
}

bool usbLock = false;
std::mutex usbMutex;

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    //DEBUG_WRITE("GET_FEATURE: %d, %d", report_id, reqlen);
    
    bool result = USB_CreateReport(report_id, buffer, &reqlen);
    return reqlen;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    //DEBUG_WRITE("SET_FEATURE: %d, %d", report_id, bufsize);

    USB_ProcessReport(report_id, buffer, bufsize);
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  DEBUG_WRITE("DEVICE DESCRIPTOR");
  return (uint8_t const *) &desc_device;
}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
  DEBUG_WRITE("HID REPORT DESCRIPTOR");
  return USB_GetDescriptor();
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    DEBUG_WRITE("CONFIGURATION DESCRIPTOR");

  (void) index; // for multiple configurations

    uint8_t desc_configuration_new[] =
    {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(1, 0, HID_ITF_PROTOCOL_NONE, USB_DescriptorSize(), 0x81, CFG_TUD_HID_EP_BUFSIZE, 1)
    };

    memcpy(desc_configuration, desc_configuration_new, sizeof(desc_configuration_new));

  return desc_configuration;
}

/**
 * @brief Invoked when received GET STRING DESCRIPTOR request.
 */
__attribute__ ((weak)) uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    //log_d("%u (0x%x)", index, langid);
    static uint16_t _desc_str[127];
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], tinyusb_string_descriptor[0], 2);
        chr_count = 1;
    } else {
        // Convert ASCII string into UTF-16
        if (index >= tinyusb_string_descriptor_len) {
            return NULL;
        }
        const char *str = tinyusb_string_descriptor[index];
        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 126) {
            chr_count = 126;
        }
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is len, second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

    return _desc_str;
}

// Invoked when device is mounted (configured)
void tud_mount_cb(void){
    DEBUG_WRITE("USB Mounted");
}

// Invoked when device is unmounted
void tud_umount_cb(void){
    DEBUG_WRITE("USB Unmounted");
}

static void configure_pins(usb_hal_context_t *usb)
{
    for (const usb_iopin_dsc_t *iopin = usb_periph_iopins; iopin->pin != -1; ++iopin) {
        if ((usb->use_external_phy) || (iopin->ext_phy_only == 0)) {
            esp_rom_gpio_pad_select_gpio((gpio_num_t)iopin->pin);
            if (iopin->is_output) {
                esp_rom_gpio_connect_out_signal((gpio_num_t)iopin->pin, iopin->func, false, false);
            } else {
                esp_rom_gpio_connect_in_signal((gpio_num_t)iopin->pin, iopin->func, false);
                if ((iopin->pin != GPIO_FUNC_IN_LOW) && (iopin->pin != GPIO_FUNC_IN_HIGH)) {
                    gpio_ll_input_enable(&GPIO, (gpio_num_t)iopin->pin);
                }
            }
            esp_rom_gpio_pad_unhold((gpio_num_t)iopin->pin);
        }
    }
    if (!usb->use_external_phy) {
        gpio_set_drive_capability((gpio_num_t)USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
        gpio_set_drive_capability((gpio_num_t)USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
    }
}


void HAL_USB_Setup()
{
    DEBUG_WRITE("USB Setup");

    periph_module_reset(PERIPH_USB_MODULE);
    periph_module_enable(PERIPH_USB_MODULE);

    usb_hal_context_t hal = {
        .use_external_phy = false
    };

    usb_hal_init(&hal);
    configure_pins(&hal);
  
   if(tusb_init()) {
        DEBUG_WRITE("USB Setup good");
   } else {
        DEBUG_WRITE("USB Setup bad");
   }
}

bool first = true;

bool HAL_USB_Update()
{
  if(first) {
    DEBUG_WRITE("USB Update");
    first = false;
  }
  
  tud_task();
  return hid_task();
}

void HAL_USB_Reconnect()
{
    esp_restart();
}

} // extern "C"