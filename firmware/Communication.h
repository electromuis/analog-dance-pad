#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

    #include <stdint.h>
    #include "Config/DancePadConfig.h"
    #include "Pad.h"
	#include "Lights.h"
	#include "ADC.h"
    #include "ConfigStore.h"
	#include "Debug.h"

    // small helper macro to do x / y, but rounded up instead of floored.
    #define CEILING(x,y) (((x) + (y) - 1) / (y))

    //
    // INPUT REPORTS
    // ie. from microcontroller to computer
    //

    typedef struct {
        uint8_t buttons[CEILING(BUTTON_COUNT, 8)];
        uint16_t sensorValues[SENSOR_COUNT];
    } __attribute__((packed)) InputHIDReport;

    //
    // FEATURE REPORTS
    // ie. can be requested by computer and written by computer
    //

    typedef struct {
        PadConfiguration configuration;
    } __attribute__((packed)) PadConfigurationFeatureHIDReport;

    typedef struct {
        NameAndSize nameAndSize;
    } __attribute__((packed)) NameFeatureHIDReport;
	
	typedef struct {
        uint8_t index;
        LightRule rule;
    } __attribute__((packed)) LightRuleHIDReport;

    typedef struct {
        uint8_t index;
        LedMapping mapping;
    } __attribute__((packed)) LedMappingHIDReport;
	
	typedef struct {
        uint8_t index;
        SensorConfig sensor;
    } __attribute__((packed)) SensorHIDReport;

    // IDS used by SetPropertyHIDReport.
    #define SPID_SELECTED_LIGHT_RULE_INDEX  0
    #define SPID_SELECTED_LED_MAPPING_INDEX 1
    #define SPID_SELECTED_SENSOR_INDEX 2

    typedef struct {
        uint32_t propertyId;
        uint32_t propertyValue;
    } __attribute__((packed)) SetPropertyHIDReport;

    typedef struct {
		uint16_t firmwareVersionMajor;
		uint16_t firmwareVersionMinor;
        uint8_t buttonCount;
        uint8_t sensorCount;
        uint8_t ledCount;
		uint16_t maxSensorValue;
		char boardType[32];
    } __attribute__((packed)) IdentificationFeatureReport;
	
	typedef struct {
		IdentificationFeatureReport parent;
		uint16_t features;
    } __attribute__((packed)) IdentificationV2FeatureReport;
	
	
	#if defined(FEATURE_DEBUG_ENABLED)
		typedef struct {
			uint16_t messageSize;
			char messagePacket[32];
		} DebugHIDReport;
	#endif
	
    void Communication_WriteInputHIDReport(InputHIDReport* report);
    void Communication_WriteIdentificationReport(IdentificationFeatureReport* report);
    void Communication_WriteIdentificationV2Report(IdentificationV2FeatureReport* report);
#endif
