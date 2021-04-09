#pragma once

#include <string>
#include <functional>

namespace devices {

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
	int numButtons = 0;
	int numSensors = 0;
	double releaseThreshold = 1.0;
};

class DeviceManager
{
public:
	struct UpdateResult { bool deviceChanged = false; };

	static void Init();

	static void Shutdown();

	static UpdateResult Update();

	static const PadState* Pad();

	static const SensorState* Sensor(int sensorIndex);

	static bool SetButtonMapping(int sensorIndex, int button);

	static bool ClearButtonMapping();

	static bool SendDeviceReset();
};

}; // namespace devices.