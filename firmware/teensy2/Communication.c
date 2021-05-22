#include <stdbool.h>

#include "Config/DancePadConfig.h"
#include "Communication.h"
#include "Pad.h"

const char boardType[] = BOARD_TYPE;

void Communication_WriteInputHIDReport(InputHIDReport* report) {
    // first, update pad state
    Pad_UpdateState();

    // write buttons to the report
    for (int i = 0; i < BUTTON_COUNT; i++) {
        // trol https://stackoverflow.com/a/47990
        report->buttons[i / 8] ^= (-PAD_STATE.buttonsPressed[i] ^ report->buttons[i / 8]) & (1UL << i % 8);
    }
   
    // write sensor values to the report
    for (int i = 0; i < SENSOR_COUNT; i++) {
        report->sensorValues[i] = PAD_STATE.sensorValues[i];
    }
}

void Communication_WriteIdentificationReport(IdentificationFeatureReport* ReportData) {
    ReportData->usbApiVersion = USB_API_VERSION;
    ReportData->buttonCount = BUTTON_COUNT;
    ReportData->sensorCount = SENSOR_COUNT;
    ReportData->maxLightRules = MAX_LIGHT_RULES;
    ReportData->maxSensorValue = MAX_SENSOR_VALUE;

    ReportData->boardType.size = sizeof(boardType) - 1; // -1 becuase null terminated
    strcpy(ReportData->boardType.name, boardType);
}
