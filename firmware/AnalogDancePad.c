/*
  Based on LUFA Library example code (www.lufa-lib.org):

  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  See other copyrights in LICENSE file on repository root.

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include <stdlib.h>
#include <string.h>

#include "Config/DancePadConfig.h"
#include "AnalogDancePad.h"
#include "Communication.h"
#include "Descriptors.h"
#include "Pad.h"
#include "Reset.h"
#include "Lights.h"
#include "Debug.h"
#include "ADC.h"

int dataCounter = 0;
static Configuration configuration;

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevHIDReportBuffer[GENERIC_EPSIZE];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Generic_HID_Interface =
    {
        .Config =
            {
                .InterfaceNumber              = INTERFACE_ID_GenericHID,
                .ReportINEndpoint             =
                    {
                        .Address              = GENERIC_IN_EPADDR,
                        .Size                 = GENERIC_EPSIZE,
                        .Banks                = 1,
                    },
                .PrevReportINBuffer           = PrevHIDReportBuffer,
                .PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
            },
    };

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
    SetupHardware();
    GlobalInterruptEnable();
    ConfigStore_LoadConfiguration(&configuration);
    SetupConfiguration();

    for (;;)
    {
        HID_Device_USBTask(&Generic_HID_Interface);
        USB_USBTask();
    }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	LEDs_Init();
	LEDs_SetAllLEDs(LEDS_ALL_LEDS);
	Delay_MS(200);
	LEDs_SetAllLEDs(LED_BOOT);
	
#if (ARCH == ARCH_AVR8)
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
    /* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
    XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
    XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

    /* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
    XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
    XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

    /* Hardware Initialization */
    USB_Init();
}

void SetupConfiguration()
{
	Pad_Initialize(&configuration.padConfiguration);
    Lights_UpdateConfiguration(&configuration.lightConfiguration);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    HID_Device_ConfigureEndpoints(&Generic_HID_Interface);
    USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
    HID_Device_MillisecondElapsed(&Generic_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
    uint8_t* const ReportID,
    const uint8_t ReportType,
    void* ReportData,
    uint16_t* const ReportSize)
{
    if (*ReportID == 0)
    {
		dataCounter ++;
		bool led = false;
		if(dataCounter > 8)
		{
			led = true;
			dataCounter = 0;
			LEDs_TurnOnLEDs(LED_DATA);
		}
		
        // no report id requested - write button and sensor data
        Communication_WriteInputHIDReport(ReportData);
        *ReportID = INPUT_REPORT_ID;
        *ReportSize = sizeof (InputHIDReport);
		
		if(led)
			LEDs_TurnOffLEDs(LED_DATA);
    }
    else if (*ReportID == PAD_CONFIGURATION_REPORT_ID)
    {
        PadConfigurationFeatureHIDReport* report = ReportData;
		
		report->configuration.releaseMultiplier =
			configuration.padConfiguration.sensors[0].threshold /
			configuration.padConfiguration.sensors[0].releaseThreshold;
		
		for (int s = 0; s < SENSOR_COUNT; s++) {
			report->configuration.sensorThresholds[s] = configuration.padConfiguration.sensors[s].threshold;
			report->configuration.sensorToButtonMapping[s] = configuration.padConfiguration.sensors[s].buttonMapping;
		}
        *ReportSize = sizeof (PadConfigurationFeatureHIDReport);
    }
    else if (*ReportID == NAME_REPORT_ID)
    {
        NameFeatureHIDReport* report = ReportData;
        memcpy(&report->nameAndSize, &configuration.nameAndSize, sizeof (report->nameAndSize));
        *ReportSize = sizeof (NameFeatureHIDReport);
    }
    else if (*ReportID == LIGHT_RULE_REPORT_ID)
    {
        LightRuleHIDReport* report = ReportData;
        report->index = LIGHT_CONF.selectedLightRuleIndex;
        if (report->index < MAX_LIGHT_RULES)
            memcpy(&report->rule, &LIGHT_CONF.lightRules[report->index], sizeof(LightRule));
        else
            memset(&report->rule, 0, sizeof(LightRule));
        *ReportSize = sizeof(LightRuleHIDReport);
    }
    else if (*ReportID == IDENTIFICATION_REPORT_ID)
    {
        Communication_WriteIdentificationReport(ReportData);
        *ReportSize = sizeof(IdentificationFeatureReport);
		
		Debug_Message("Welcome!\n");
    }
    else if (*ReportID == IDENTIFICATION_V2_REPORT_ID)
    {
        Communication_WriteIdentificationV2Report(ReportData);
        *ReportSize = sizeof(IdentificationV2FeatureReport);
		
		Debug_Message("Welcome V2!\n");
    }
    else if (*ReportID == LED_MAPPING_REPORT_ID)
    {
        LedMappingHIDReport* report = ReportData;
        report->index = LIGHT_CONF.selectedLedMappingIndex;
        if (report->index < MAX_LED_MAPPINGS)
            memcpy(&report->mapping, &LIGHT_CONF.ledMappings[report->index], sizeof(LedMapping));
        else
            memset(&report->mapping, 0, sizeof(LedMapping));
        *ReportSize = sizeof(LedMappingHIDReport);
    }
    else if (*ReportID == SENSOR_REPORT_ID)
    {
        SensorHIDReport* report = ReportData;
        report->index = PAD_CONF.selectedSensorIndex;
        if (report->index < SENSOR_COUNT)
            memcpy(&report->sensor, &PAD_CONF.sensors[report->index], sizeof(SensorConfig));
        else
            memset(&report->sensor, 0, sizeof(SensorConfig));
        *ReportSize = sizeof(SensorHIDReport);
    }
	#if defined(FEATURE_DEBUG_ENABLED)
	else if (*ReportID == DEBUG_REPORT_ID)
    {
        DebugHIDReport* report = ReportData;
		
		uint16_t readSize = Debug_Available();
		uint16_t maxReadSize = sizeof(report->messagePacket);
		
		if(readSize > 0) {
			if(readSize > maxReadSize) {
				readSize = maxReadSize;
			}
			
			report->messageSize = readSize;
			Debug_ReadBuffer(&report->messagePacket, readSize);
		}
		
        *ReportSize = sizeof(DebugHIDReport);
    }
	#endif

    return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
    if (ReportID == PAD_CONFIGURATION_REPORT_ID && ReportSize == sizeof (PadConfigurationFeatureHIDReport))
    {
        const PadConfigurationFeatureHIDReport* report = ReportData;
		for (int s = 0; s < SENSOR_COUNT; s++) {
			configuration.padConfiguration.sensors[s].threshold = report->configuration.sensorThresholds[s];
			configuration.padConfiguration.sensors[s].releaseThreshold = report->configuration.sensorThresholds[s] * report->configuration.releaseMultiplier;
			configuration.padConfiguration.sensors[s].buttonMapping = report->configuration.sensorToButtonMapping[s];
		}
        Pad_UpdateConfiguration(&configuration.padConfiguration);
    }
    else if (ReportID == RESET_REPORT_ID)
    {
        Reset_JumpToBootloader();
    }
    else if (ReportID == SAVE_CONFIGURATION_REPORT_ID)
    {
        ConfigStore_StoreConfiguration(&configuration);
    }
    else if (ReportID == FACTORY_RESET_REPORT_ID)
    {
		LEDs_SetAllLEDs(0);
        ConfigStore_FactoryDefaults(&configuration);
        ConfigStore_StoreConfiguration(&configuration);
		SetupConfiguration();
        Reconnect_Usb();
		LEDs_SetAllLEDs(LED_BOOT);
    }
    else if (ReportID == NAME_REPORT_ID && ReportSize == sizeof (NameFeatureHIDReport))
    {
        const NameFeatureHIDReport* report = ReportData;
        memcpy(&configuration.nameAndSize, &report->nameAndSize, sizeof (configuration.nameAndSize));
    }
    else if (ReportID == LIGHT_RULE_REPORT_ID && ReportSize == sizeof (LightRuleHIDReport))
    {
        const LightRuleHIDReport* report = ReportData;
        if (report->index < MAX_LIGHT_RULES)
        {
            memcpy(&configuration.lightConfiguration.lightRules[report->index], &report->rule, sizeof(LightRule));
            Lights_UpdateConfiguration(&configuration.lightConfiguration);
        }
    }
    else if (ReportID == LED_MAPPING_REPORT_ID && ReportSize == sizeof(LedMappingHIDReport))
    {
        const LedMappingHIDReport* report = ReportData;
        if (report->index < MAX_LED_MAPPINGS)
        {
            memcpy(&configuration.lightConfiguration.ledMappings[report->index], &report->mapping, sizeof(LedMapping));
            Lights_UpdateConfiguration(&configuration.lightConfiguration);
        }
    }
	
	else if (ReportID == SENSOR_REPORT_ID && ReportSize == sizeof(SensorHIDReport))
    {
        const SensorHIDReport* report = ReportData;
        if (report->index < SENSOR_COUNT)
        {
			SensorConfig* sc = &configuration.padConfiguration.sensors[report->index];
			
			sc->threshold = report->sensor.threshold;
			sc->releaseThreshold = report->sensor.releaseThreshold;
			sc->buttonMapping = report->sensor.buttonMapping;
			sc->resistorValue = report->sensor.resistorValue;
			sc->flags = report->sensor.flags;
			
			/*
            memcpy(
				&configuration.padConfiguration.sensors[report->index],
				&report->sensor,
				sizeof(SensorConfig)
			);
			configuration.padConfiguration.sensors[report->index].preload = 0;
			*/
			
            Pad_UpdateConfiguration(&configuration.padConfiguration);
        }
    }
	
    else if (ReportID == SET_PROPERTY_REPORT_ID && ReportSize == sizeof (SetPropertyHIDReport))
    {
        const SetPropertyHIDReport* report = ReportData;
        switch (report->propertyId)
        {
        case SPID_SELECTED_LIGHT_RULE_INDEX:
            LIGHT_CONF.selectedLightRuleIndex = (uint8_t)report->propertyValue;
            break;

        case SPID_SELECTED_LED_MAPPING_INDEX:
            LIGHT_CONF.selectedLedMappingIndex = (uint8_t)report->propertyValue;
            break;

        case SPID_SELECTED_SENSOR_INDEX:
            PAD_CONF.selectedSensorIndex = (uint8_t)report->propertyValue;
            break;
		case SPID_SENSOR_CAL_PRELOAD:
			PAD_CONF.selectedSensorIndex = (uint8_t)report->propertyValue;
			if (PAD_CONF.selectedSensorIndex < SENSOR_COUNT)
			{
				configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = 0;
				Pad_UpdateConfiguration(&configuration.padConfiguration);
				
				uint16_t value = ADC_Read(PAD_CONF.selectedSensorIndex);
				
				configuration.padConfiguration.sensors[PAD_CONF.selectedSensorIndex].preload = value;
				Pad_UpdateConfiguration(&configuration.padConfiguration);
			}
			break;
        }
    }
}
