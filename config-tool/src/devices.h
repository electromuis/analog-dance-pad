#pragma once

#include <vector>

struct SensorState
{
	int threshold;
	int value;
	int button;
	bool isPressed;
	bool isMapped;
};

class PadDevice
{
public:
	virtual const char* name() const = 0;

	virtual int numSensors() const = 0;

	virtual const SensorState& sensor(int index) const = 0;

	virtual double releaseThreshold() const = 0;
};

class DeviceManager
{
public:
	static void init();

	static bool readSensorValues();

	static PadDevice* getDevice();

	static void shutdown();
};