#include "SensorReports.h"
#include "ReportsInternal.h"
#include "./../Descriptors.h"

extern "C" {
#include "../Config/DancePadConfig.h"
#include "../ConfigStore.h"
#include "../AnalogDancePad.h"
}

PadConfigurationFeatureHIDReport::PadConfigurationFeatureHIDReport()
{
    reportConfiguration.releaseMultiplier =
        configuration.padConfiguration.sensors[0].threshold /
        configuration.padConfiguration.sensors[0].releaseThreshold;
		
    for (int s = 0; s < SENSOR_COUNT; s++) {
        reportConfiguration.sensorThresholds[s] = configuration.padConfiguration.sensors[s].threshold;
        reportConfiguration.sensorToButtonMapping[s] = configuration.padConfiguration.sensors[s].buttonMapping;
    }
}

void PadConfigurationFeatureHIDReport::Process()
{
    for (int s = 0; s < SENSOR_COUNT; s++) {
        configuration.padConfiguration.sensors[s].threshold = reportConfiguration.sensorThresholds[s];
        configuration.padConfiguration.sensors[s].releaseThreshold = reportConfiguration.sensorThresholds[s] * reportConfiguration.releaseMultiplier;
        configuration.padConfiguration.sensors[s].buttonMapping = reportConfiguration.sensorToButtonMapping[s];
    }
    Pad_UpdateConfiguration(&configuration.padConfiguration);
}

SensorHIDReport::SensorHIDReport()
{
    index = PAD_CONF.selectedSensorIndex;
    if (index < SENSOR_COUNT)
        memcpy(&sensor, &PAD_CONF.sensors[index], sizeof(SensorConfig));
    else
        memset(&sensor, 0, sizeof(SensorConfig));
}

void SensorHIDReport::Process()
{
    if (index < SENSOR_COUNT) {
        SensorConfig* sc = &configuration.padConfiguration.sensors[index];
        
        sc->threshold = sensor.threshold;
        sc->releaseThreshold = sensor.releaseThreshold;
        sc->buttonMapping = sensor.buttonMapping;
        sc->resistorValue = sensor.resistorValue;
        sc->flags = sensor.flags;

        Pad_UpdateConfiguration(&configuration.padConfiguration);
    }
}

void RegisterSensorReports()
{
    REGISTER_REPORT(PAD_CONFIGURATION_REPORT_ID, PadConfigurationFeatureHIDReport)
    REGISTER_REPORT(SENSOR_REPORT_ID, SensorHIDReport)
}