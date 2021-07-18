#pragma once

#include "stdint.h"
#include <string>

#include "Model/Firmware.h"

namespace adp {

enum DeviceChangeFlags
{
	DCF_DEVICE         = 1 << 0,
	DCF_BUTTON_MAPPING = 1 << 1,
	DCF_NAME           = 1 << 2,
	DCF_LIGHTS         = 1 << 3,
};
typedef int32_t DeviceChanges;

struct RgbColor
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

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
	BoardType boardType = BOARD_UNKNOWN;
};

struct LedMapping
{
	int index;
	int lightRuleIndex;
	int sensorIndex;
	int ledIndexBegin;
	int ledIndexEnd;
};

struct LightRule
{
	int index;
	bool fadeOn;
	bool fadeOff;
	RgbColor onColor;
	RgbColor offColor;
	RgbColor onFadeColor;
	RgbColor offFadeColor;
	std::vector<LedMapping> ledMappings;
};

struct LightsState
{
	std::vector<LightRule> lightRules;
};

class Device
{
public:
	static void Init();

	static void Shutdown();

	static DeviceChanges Update();

	static int PollingRate();

	static const PadState* Pad();

	static const LightsState* Lights();

	static const SensorState* Sensor(int sensorIndex);

	static bool SetThreshold(int sensorIndex, double threshold);

	static bool SetReleaseThreshold(double threshold);

	static bool SetButtonMapping(int sensorIndex, int button);

	static bool SetDeviceName(const wchar_t* name);

	static bool SendLedMapping(LedMapping mapping);

	static bool DisableLedMapping(int ledMappingIndex);

	static bool SendLightRule(LightRule rule);

	static bool DisableLightRule(int lightRuleIndex);

	static void SendDeviceReset();

	static void SendFactoryReset();
};

}; // namespace adp.
