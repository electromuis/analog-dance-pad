#include "SensorReports.hpp"
#include "ReportsInternal.hpp"
#include "Modules/ModulePad.hpp"

#include <string.h>

PadConfigurationFeatureHIDReport::PadConfigurationFeatureHIDReport()
{
    reportConfiguration.releaseMultiplier =
        configuration.padConfiguration.sensors[0].threshold /
        configuration.padConfiguration.sensors[0].releaseThreshold;
		
    for (int s = 0; s < SENSOR_COUNT_V1; s++) {
        reportConfiguration.sensorThresholds[s] = configuration.padConfiguration.sensors[s].threshold;
        reportConfiguration.sensorToButtonMapping[s] = configuration.padConfiguration.sensors[s].buttonMapping;
    }
}

void PadConfigurationFeatureHIDReport::Process()
{
    for (int s = 0; s < SENSOR_COUNT_V1; s++) {
        configuration.padConfiguration.sensors[s].threshold = reportConfiguration.sensorThresholds[s];
        configuration.padConfiguration.sensors[s].releaseThreshold = reportConfiguration.sensorThresholds[s] * reportConfiguration.releaseMultiplier;
        configuration.padConfiguration.sensors[s].buttonMapping = reportConfiguration.sensorToButtonMapping[s];
    }
}

SensorHIDReport::SensorHIDReport()
{
    index = ModulePadInstance.selectedSensorIndex;
    if (index < SENSOR_COUNT)
        memcpy(&sensor, &configuration.padConfiguration.sensors[index], sizeof(SensorConfig));
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

        // Pad_UpdateConfiguration(&configuration.padConfiguration);
    }
}

void RegisterSensorReports()
{
    REGISTER_REPORT(PAD_CONFIGURATION_REPORT_ID, PadConfigurationFeatureHIDReport)
    REGISTER_REPORT(SENSOR_REPORT_ID, SensorHIDReport)
}
