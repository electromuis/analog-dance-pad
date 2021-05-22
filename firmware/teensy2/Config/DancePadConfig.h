#ifndef _DANCE_PAD_CONFIG_H_
#define _DANCE_PAD_CONFIG_H_
    //Version 2 since Kauhsa's initial version will be considered version 1
    #define USB_API_VERSION 2

    #define MAX_SENSOR_VALUE 1024

    // this value doesn't mean we're actively using all these buttons.
    // it's just what we report and is technically possible to use.
    // for now, should be divisible by 8.
    #define BUTTON_COUNT 16

    // this value doesn't mean we're reading all these sensors.
    // teensy 2.0 has 12 analog sensors, so that's what we use.
    #define SENSOR_COUNT 12
	
	#define MAX_LIGHT_RULES 16

    #define BOARD_TYPE "fsrminipad";
#endif
