#pragma once

#include "stdint.h"
#include <string>

namespace adp {

enum DeviceChangeFlags
{
	DCF_DEVICE         = 1 << 0,
	DCF_BUTTON_MAPPING = 1 << 1,
	DCF_NAME           = 1 << 2,
};
typedef int32_t DeviceChanges;

struct SensorState
{
	double threshold = 0.0;
	double value = 0.0;
	int button = 0; // zero means unmapped.
	bool pressed = false;
};

struct PadState
{
	std::wstring name;
	int maxNameLength = 0;
	int numButtons = 0;
	int numSensors = 0;
	double releaseThreshold = 1.0;
};

class Device
{
public:
	static void Init();

	static void Shutdown();

	static DeviceChanges Update();

	static const PadState* Pad();

	static const SensorState* Sensor(int sensorIndex);

	static bool SetThreshold(int sensorIndex, double threshold);

	static bool SetReleaseThreshold(double threshold);

	static bool SetButtonMapping(int sensorIndex, int button);

	static bool SetDeviceName(const wchar_t* name);

	static void SendDeviceReset();

	static void SendFactoryReset();
};

}; // namespace adp.
