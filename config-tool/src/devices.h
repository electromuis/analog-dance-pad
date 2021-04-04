#pragma once

#include <string>
#include <functional>

namespace devices {

struct SensorState
{
	enum { UNMAPPED_BUTTON = -1 };

	double threshold = 0.0;
	double value = 0.0;
	int button = UNMAPPED_BUTTON;
	bool pressed = false;
};

struct DeviceState
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

	static const DeviceState* Device();

	static const SensorState* Sensor(int index);
};

}; // namespace devices.