#include "adp_config.hpp"
#ifdef USB_MODE_XINPUT

#include <MPG.h>

#include <USB.h>
#include "hal/hal_USB.hpp"
#include "tusb.h"
#include "device/usbd_pvt.h"
#include "Modules/ModulePad.hpp"

// #include "tusb_option.h"
// #include "esp32-hal-tinyusb.h"
// #include "esp_hid_common.h"

uint8_t endpoint_in=0;
uint8_t endpoint_out=0;

class Gamepad : public MPG {
	public:
		Gamepad(int debounceMS = 5) : MPG(debounceMS)
		{
		}

		void setup()
        {

        }

		/**
		 * @brief Retrieve the inputs and save to the current state. Derived classes must overide this member.
		 */
		void read()
        {
            // Gather raw inputs
            state.buttons = 0
                | ((ModulePadInstance.buttonsPressed[0])     ? GAMEPAD_MASK_UP    : 0)
                | ((ModulePadInstance.buttonsPressed[1])   ? GAMEPAD_MASK_DOWN  : 0)
                | ((ModulePadInstance.buttonsPressed[2])   ? GAMEPAD_MASK_LEFT  : 0)
                | ((ModulePadInstance.buttonsPressed[3])  ? GAMEPAD_MASK_RIGHT : 0)
                ;
        }
};

Gamepad gamepad;

static void sendReportData(void) {

  // Poll every 1ms
    const uint32_t interval_ms = 1;
    static uint32_t start_ms = 0;

    // if (board_millis() - start_ms < interval_ms) return;  // not enough time
    // start_ms += interval_ms;

    // Remote wakeup
    if (tud_suspended()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
    // XboxButtonData.rsize = 20;
    uint8_t* buffer = (uint8_t*)gamepad.getXInputReport();

    if ((tud_ready()) && ((endpoint_in != 0)) && (!usbd_edpt_busy(0, endpoint_in))){
      usbd_edpt_claim(0, endpoint_in);
      usbd_edpt_xfer(0, endpoint_in, buffer, gamepad.getReportSize());
      usbd_edpt_release(0, endpoint_in);
    }   
}

// Exports

void HAL_USB_Setup()
{
    gamepad.setup();
	  gamepad.read();

    // tusb_init();
    USB.begin();
}

void HAL_USB_Update()
{
    gamepad.read(); 
    // gamepad.debounce();
    gamepad.process();
    sendReportData();
    tud_task();
}

static void xinput_init(void) {

}

static void xinput_reset(uint8_t __unused rhport) {
    
}

static uint16_t xinput_open(uint8_t __unused rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len) {
  //+16 is for the unknown descriptor 
  uint16_t const drv_len = sizeof(tusb_desc_interface_t) + itf_desc->bNumEndpoints*sizeof(tusb_desc_endpoint_t) + 16;
  TU_VERIFY(max_len >= drv_len, 0);

  uint8_t const * p_desc = tu_desc_next(itf_desc);
  uint8_t found_endpoints = 0;
  while ( (found_endpoints < itf_desc->bNumEndpoints) && (drv_len <= max_len)  )
  {
    tusb_desc_endpoint_t const * desc_ep = (tusb_desc_endpoint_t const *) p_desc;
    if ( TUSB_DESC_ENDPOINT == tu_desc_type(desc_ep) )
    {
      TU_ASSERT(usbd_edpt_open(rhport, desc_ep));

      if ( tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN )
      {
        endpoint_in = desc_ep->bEndpointAddress;
      }else
      {
        endpoint_out = desc_ep->bEndpointAddress;
      }
      found_endpoints += 1;
    }
    p_desc = tu_desc_next(p_desc);
  }
  return drv_len;
}

static bool xinput_device_control_request(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
  return true;
}

//callback after xfer_transfer
static bool xinput_xfer_cb(uint8_t __unused rhport, uint8_t __unused ep_addr, xfer_result_t __unused result, uint32_t __unused xferred_bytes) {
  return true;
}


static usbd_class_driver_t const xinput_driver ={
  #if CFG_TUSB_DEBUG >= 2
    .name = "XINPUT",
#endif
    .init             = xinput_init,
    .reset            = xinput_reset,
    .open             = xinput_open,
    .control_xfer_cb  = xinput_device_control_request,
    .xfer_cb          = xinput_xfer_cb,
    .sof              = NULL
};

// Implement callback to add our custom driver
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &xinput_driver;
}

#endif