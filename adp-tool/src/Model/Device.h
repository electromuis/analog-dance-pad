#pragma once

#include "stdint.h"
#include <string>
#include <map>
#include "wx/string.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

enum DeviceProfileGroupFlags
{
	DPG_SENSITIVITY	= 1 << 0,
	DPG_MAPPING		= 1 << 1,
	DPG_DEVICE		= 1 << 2,
	DPG_LIGHTS		= 1 << 3,
	
	DGP_ALL			= 0b1111111111111111
};

typedef int32_t DeviceProfileGroups;

struct RgbColor
{
	RgbColor(uint8_t r, uint8_t g, uint8_t b)
		:red(r),green(g),blue(b)
	{ ; }
	
	RgbColor(std::string input) {
		sscanf(input.c_str(), "%02X%02X%02X", &red, &green, &blue);
	}
	
	RgbColor() { ; }
	
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	
	const char* ToString() const {
		return wxString::Format("%02X%02X%02X", red, green, blue).c_str();
	}
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
	int lightRuleIndex;
	int sensorIndex;
	int ledIndexBegin;
	int ledIndexEnd;
};

struct LightRule
{
	bool fadeOn;
	bool fadeOff;
	RgbColor onColor;
	RgbColor offColor;
	RgbColor onFadeColor;
	RgbColor offFadeColor;
};

struct LightsState
{
	std::map<int, LightRule> lightRules;
	std::map<int, LedMapping> ledMappings;
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

	static bool SendLedMapping(int ledMappingIndex, LedMapping mapping);

	static bool DisableLedMapping(int ledMappingIndex);

	static bool SendLightRule(int lightRuleIndex, LightRule rule);

	static bool DisableLightRule(int lightRuleIndex);

	static void SendDeviceReset();

	static void SendFactoryReset();
	
	static void LoadProfile(json& j, DeviceProfileGroups groups);
	
	static void SaveProfile(json& j, DeviceProfileGroups groups);
	
	static void SetSearching(bool s);
};

}; // namespace adp.
