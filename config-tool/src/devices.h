#pragma once

#include <string>
#include <functional>

namespace devices {

enum DeviceChangeFlags
{
	DCF_DEVICE         = 0x1,
	DCF_BUTTON_MAPPING = 0x2,
	DCF_NAME           = 0x2,
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

class DeviceManager
{
public:
	static void Init();

	static void Shutdown();

	static DeviceChanges Update();

	static const PadState* Pad();

	static const SensorState* Sensor(int sensorIndex);

	static bool SetThreshold(int sensorIndex, double threshold);

	static bool SetButtonMapping(int sensorIndex, int button);

	static bool SetDeviceName(const wchar_t* name);

	static void SendDeviceReset();
};

}; // namespace devices.