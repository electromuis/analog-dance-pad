#include "Adp.h"

#include <memory>
#include <algorithm>
#include <map>
#include <chrono>
#include <thread>

#include "hidapi.h"

#include "Model/Device.h"
#include "Model/Reporter.h"
#include "Model/Log.h"
#include "Model/Utils.h"
#include "Model/Firmware.h"

using namespace std;
using namespace chrono;

namespace adp {

struct HidIdentifier
{
	int vendorId;
	int productId;
};

constexpr HidIdentifier HID_IDS[] =
{
	// TODO: document what these correspond to.
	{0x1209, 0xb196},
	{0x03eb, 0x204f},
};

static_assert(sizeof(float) == sizeof(uint32_t), "32-bit float required");

enum LedMappingFlags
{
	LMF_ENABLED = 1 << 0,
};

enum LightRuleFlags
{
	LRF_ENABLED  = 1 << 0,
	LRF_FADE_ON  = 1 << 1,
	LRF_FADE_OFF = 1 << 2,
};

// ====================================================================================================================
// Helper functions.
// ====================================================================================================================

static bool IsBitSet(int bits, int index)
{
	return (bits & (1 << index)) != 0;
}

static int ReadU16LE(uint16_le u16)
{
	return u16.bytes[0] | u16.bytes[1] << 8;
}

static uint32_t ReadU32LE(uint32_le u32)
{
	return u32.bytes[0] | (u32.bytes[1] << 8) | (u32.bytes[2] << 16) | (u32.bytes[3] << 24);
}

static float ReadF32LE(float32_le f32)
{
	uint32_t u32 = ReadU32LE(f32.bits);
	return *reinterpret_cast<float*>(&u32);
}

static uint16_le WriteU16LE(int value)
{
	uint16_le u16;
	u16.bytes[0] = value & 0xFF;
	u16.bytes[1] = (value >> 8) & 0xFF;
	return u16;
}

static uint32_le WriteU32LE(uint32_t value)
{
	uint32_le u32;
	u32.bytes[0] = value & 0xFF;
	u32.bytes[1] = (value >> 8) & 0xFF;
	u32.bytes[2] = (value >> 16) & 0xFF;
	u32.bytes[3] = (value >> 24) & 0xFF;
	return u32;
}

static float32_le WriteF32LE(float value)
{
	uint32_t u32 = *reinterpret_cast<uint32_t*>(&value);
	return { WriteU32LE(u32) };
}

template <typename T>
static double ToNormalizedSensorValue(T deviceValue)
{
	constexpr double scalar = 1.0 / (double)MAX_SENSOR_VALUE;
	return max(0.0, min(1.0, deviceValue * scalar));
}

static int ToDeviceSensorValue(double normalizedValue)
{
	int mapped = (int)lround(normalizedValue * MAX_SENSOR_VALUE);
	return max(0, min(MAX_SENSOR_VALUE, mapped));
}

static RgbColor ToRgbColor(color24 color)
{
	return { color.red, color.green, color.blue };
}

static color24 ToColor24(RgbColor color)
{
	return { color.red, color.green, color.blue };
}

static void PrintPadConfigurationReport(const PadConfigurationReport& padConfiguration)
{
	Log::Write(L"pad configuration [");
	Log::Writef(L"  releaseThreshold: %.2f", ReadF32LE(padConfiguration.releaseThreshold));
	Log::Write(L"  sensors: [");
	for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
	{
		Log::Writef(L"    sensorToButtonMapping: %i", padConfiguration.sensorToButtonMapping[i]);
		Log::Writef(L"    sensorThresholds: %i", ReadU16LE(padConfiguration.sensorThresholds[i]));
	}
	Log::Write(L"  ]");
	Log::Write(L"]");
}

static void PrintLightRuleReport(const LightRuleReport& r)
{
	Log::Write(L"light rule [");
	Log::Writef(L"  lightRuleIndex: %i", r.lightRuleIndex);
	Log::Writef(L"  flags: %i", r.flags);
	Log::Writef(L"  onColor: [R%i G%i B%i]", r.onColor.red, r.onColor.green, r.onColor.blue);
	Log::Writef(L"  offColor: [R%i G%i B%i]", r.offColor.red, r.offColor.green, r.offColor.blue);
	Log::Writef(L"  onFadeColor: [R%i G%i B%i]", r.onFadeColor.red, r.onFadeColor.green, r.onFadeColor.blue);
	Log::Writef(L"  offFadeColor: [R%i G%i B%i]", r.offFadeColor.red, r.offFadeColor.green, r.offFadeColor.blue);
	Log::Write(L"]");
}

static void PrintLedMappingReport(const LedMappingReport& r)
{
	Log::Write(L"led mapping [");
	Log::Writef(L"  ledMappingIndex: %i", r.ledMappingIndex);
	Log::Writef(L"  flags: %i", r.flags);
	Log::Writef(L"  lightRuleIndex: %i", r.lightRuleIndex);
	Log::Writef(L"  sensorIndex: %i", r.sensorIndex);
	Log::Writef(L"  ledIndexBegin: %i", r.ledIndexBegin);
	Log::Writef(L"  ledIndexEnd: %i", r.ledIndexEnd);
	Log::Write(L"]");
}

// ====================================================================================================================
// Pad device.
// ====================================================================================================================

typedef string DevicePath;
typedef wstring DeviceName;

struct PollingData
{
	int readsSinceLastUpdate = 0;
	int pollingRate = 0;
	time_point<system_clock> lastUpdate;
};

class PadDevice
{
public:
	PadDevice(
		unique_ptr<Reporter>& reporter,
		const char* path,
		const NameReport& name,
		const PadConfigurationReport& config,
		const IdentificationReport& identification,
		const vector<LightRuleReport>& lightRules,
		const vector<LedMappingReport>& ledMappings)
		: myReporter(move(reporter))
		, myPath(path)
	{
		UpdateName(name);
		myPad.maxNameLength = MAX_NAME_LENGTH;
		myPad.numButtons = identification.buttonCount;
		myPad.numSensors = identification.sensorCount;

		auto buffer = (char*)calloc(BOARD_TYPE_LENGTH + 1, sizeof(char));
		if (buffer)
		{
			memcpy(buffer, identification.boardType, BOARD_TYPE_LENGTH);
			myPad.boardType = ParseBoardType(buffer);
			free(buffer);
		}

		UpdatePadConfiguration(config);
		UpdateLightsConfiguration(lightRules, ledMappings);
		myPollingData.lastUpdate = system_clock::now();
	}

	void UpdateName(const NameReport& report)
	{
		myPad.name = widen((const char*)report.name, (size_t)report.size);
		myChanges |= DCF_NAME;
	}

	void UpdatePadConfiguration(const PadConfigurationReport& report)
	{
		for (int i = 0; i < myPad.numSensors; ++i)
		{
			auto buttonMapping = (uint8_t)report.sensorToButtonMapping[i];
			mySensors[i].threshold = ToNormalizedSensorValue(ReadU16LE(report.sensorThresholds[i]));
			mySensors[i].button = (buttonMapping >= myPad.numButtons ? 0 : (buttonMapping + 1));
		}
		myPad.releaseThreshold = ReadF32LE(report.releaseThreshold);
	}

	void UpdateLightRule(const LightRuleReport& report)
	{
		if (report.flags & LRF_ENABLED)
		{
			LightRule result;
			result.fadeOn = (report.flags & LRF_FADE_ON) != 0;
			result.fadeOff = (report.flags & LRF_FADE_OFF) != 0;
			result.onColor = ToRgbColor(report.onColor);
			result.onFadeColor = ToRgbColor(report.onFadeColor);
			result.offColor = ToRgbColor(report.offColor);
			result.offFadeColor = ToRgbColor(report.offFadeColor);
			myLights.lightRules[report.lightRuleIndex] = result;
		}
		else
		{
			myLights.lightRules.erase(report.lightRuleIndex);
		}
	}

	void UpdateLedMapping(const LedMappingReport& report)
	{
		if (report.flags & LMF_ENABLED)
		{
			LedMapping result;
			result.lightRuleIndex = report.lightRuleIndex;
			result.sensorIndex = report.sensorIndex;
			result.ledIndexBegin = report.ledIndexBegin;
			result.ledIndexEnd = report.ledIndexEnd;
			myLights.ledMappings[report.ledMappingIndex] = result;
		}
		else
		{
			myLights.ledMappings.erase(report.ledMappingIndex);
		}
	}

	void UpdateLightsConfiguration(const vector<LightRuleReport>& lightRules, const vector<LedMappingReport>& ledMappings)
	{
		for (auto& report : lightRules)
			UpdateLightRule(report);

		for (auto& report : ledMappings)
			UpdateLedMapping(report);
	}

	bool UpdateSensorValues()
	{
		SensorValuesReport report;

		int aggregateValues[MAX_SENSOR_COUNT] = {};
		int pressedButtons = 0;
		int inputsRead = 0;

		for (int readsLeft = 100; readsLeft > 0; --readsLeft)
		{
			switch (myReporter->Get(report))
			{
			case ReadDataResult::SUCCESS:
				pressedButtons |= ReadU16LE(report.buttonBits);
				for (int i = 0; i < myPad.numSensors; ++i)
					aggregateValues[i] += ReadU16LE(report.sensorValues[i]);
				++inputsRead;
				break;

			case ReadDataResult::NO_DATA:
				readsLeft = 0;
				break;

			case ReadDataResult::FAILURE:
				return false;
			}
		}

		if (inputsRead > 0)
		{
			for (int i = 0; i < myPad.numSensors; ++i)
			{
				auto button = mySensors[i].button;
				auto value = (double)aggregateValues[i] / (double)inputsRead;
				mySensors[i].pressed = button > 0 && IsBitSet(pressedButtons, button - 1);
				mySensors[i].value = ToNormalizedSensorValue(value);
			}
			myPollingData.readsSinceLastUpdate += inputsRead;
		}

		auto now = system_clock::now();
		if (now > myPollingData.lastUpdate + 1s)
		{
			auto dt = duration<double>(now - myPollingData.lastUpdate).count();
			myPollingData.pollingRate = (int)lround(myPollingData.readsSinceLastUpdate / dt);
			myPollingData.readsSinceLastUpdate = 0;
			myPollingData.lastUpdate = now;
		}

		return true;
	}

	bool SetThreshold(int sensorIndex, double threshold)
	{
		mySensors[sensorIndex].threshold = clamp(threshold, 0.0, 1.0);
		return SendPadConfiguration();
	}

	bool SetReleaseThreshold(double threshold)
	{
		myPad.releaseThreshold = clamp(threshold, 0.01, 1.00);
		return SendPadConfiguration();
	}

	bool SetButtonMapping(int sensorIndex, int button)
	{
		mySensors[sensorIndex].button = button;
		myChanges |= DCF_BUTTON_MAPPING;
		return SendPadConfiguration();
	}

	bool SendName(const wchar_t* rawName)
	{
		NameReport report;

		auto name = narrow(rawName, wcslen(rawName));
		if (name.size() > sizeof(report.name))
		{
			Log::Writef(L"SetName :: name '%ls' exceeds %i chars and was not set", rawName, sizeof(report.name));
			return false;
		}

		report.size = (uint8_t)name.length();
		memcpy(report.name, name.data(), name.length());
		bool result = myReporter->Send(report) && myReporter->Get(report);
		myHasUnsavedChanges = true;
		UpdateName(report);
		return result;
	}

	bool SendLedMappingReport(const LedMappingReport& report)
	{
		if (!myReporter->Send(report))
			return false;
		
		UpdateLedMapping(report);
		myChanges |= DCF_LIGHTS;
		myHasUnsavedChanges = true;
		return true;
	}

	bool SendLedMapping(int ledMappingIndex, LedMapping mapping)
	{
		LedMappingReport report;
		report.ledMappingIndex = ledMappingIndex;
		report.lightRuleIndex = mapping.lightRuleIndex;
		report.flags = LMF_ENABLED;
		report.ledIndexBegin = mapping.ledIndexBegin;
		report.ledIndexEnd = mapping.ledIndexEnd;
		report.sensorIndex = mapping.sensorIndex;
		return SendLedMappingReport(report);
	}

	bool DisableLedMapping(int ledMappingIndex)
	{
		LedMappingReport report;
		report.ledMappingIndex = ledMappingIndex;
		report.lightRuleIndex = 0;
		report.flags = 0;
		report.ledIndexBegin = 0;
		report.ledIndexEnd = 0;
		report.sensorIndex = 0;
		return SendLedMappingReport(report);
	}

	bool SendLightRuleReport(const LightRuleReport& report)
	{
		if (!myReporter->Send(report))
			return false;

		UpdateLightRule(report);
		myChanges |= DCF_LIGHTS;
		myHasUnsavedChanges = true;
		return true;
	}

	bool SendLightRule(int lightRuleIndex, LightRule rule)
	{
		LightRuleReport report;
		report.lightRuleIndex = lightRuleIndex;
		report.flags = LRF_ENABLED | (LRF_FADE_ON * rule.fadeOn) | (LRF_FADE_OFF * rule.fadeOff);
		report.onColor = ToColor24(rule.onColor);
		report.offColor = ToColor24(rule.offColor);
		report.onFadeColor = ToColor24(rule.onFadeColor);
		report.offFadeColor = ToColor24(rule.offFadeColor);
		return SendLightRuleReport(report);
	}

	bool DisableLightRule(int lightRuleIndex)
	{
		LightRuleReport report;
		report.lightRuleIndex = lightRuleIndex;
		report.flags = 0;
		report.onColor = {0, 0, 0};
		report.offColor = {0, 0, 0};
		report.onFadeColor = {0, 0, 0};
		report.offFadeColor = {0, 0, 0};
		return SendLightRuleReport(report);
	}

	void Reset() { myReporter->SendReset(); }

	void FactoryReset()
	{
		// Have the device load up and save its defaults
		myReporter->SendFactoryReset();
	}

	bool SendPadConfiguration()
	{
		PadConfigurationReport report;
		for (int i = 0; i < myPad.numSensors; ++i)
		{
			report.sensorThresholds[i] = WriteU16LE(ToDeviceSensorValue(mySensors[i].threshold));
			report.sensorToButtonMapping[i] = (mySensors[i].button == 0) ? 0xFF : (mySensors[i].button - 1);
		}
		report.releaseThreshold = WriteF32LE((float)myPad.releaseThreshold);

		bool sendResult = myReporter->Send(report);
		bool getResult = myReporter->Get(report);

		myHasUnsavedChanges = true;
		UpdatePadConfiguration(report);
		return sendResult && getResult;
	}

	void SaveChanges()
	{
		if (myHasUnsavedChanges)
		{
			myReporter->SendSaveConfiguration();
			myHasUnsavedChanges = false;
		}
	}

	const DevicePath& Path() const { return myPath; }

	const int PollingRate() const { return myPollingData.pollingRate; }

	const PadState& State() const { return myPad; }

	const LightsState& Lights() const { return myLights; }

	const SensorState* Sensor(int index)
	{
		return (index >= 0 && index < myPad.numSensors) ? (mySensors + index) : nullptr;
	}

	DeviceChanges PopChanges()
	{
		auto result = myChanges;
		myChanges = 0;
		return result;
	}

private:
	unique_ptr<Reporter> myReporter;
	DevicePath myPath;
	PadState myPad;
	LightsState myLights;
	SensorState mySensors[MAX_SENSOR_COUNT];
	DeviceChanges myChanges = 0;
	bool myHasUnsavedChanges = false;
	PollingData myPollingData;
};

// ====================================================================================================================
// Connection manager.
// ====================================================================================================================

static bool ContainsDevice(hid_device_info* devices, DevicePath path)
{
	for (auto device = devices; device; device = device->next)
		if (path == device->path)
			return true;

	return false;
}

class ConnectionManager
{
public:
	~ConnectionManager()
	{
		if (myConnectedDevice)
			myConnectedDevice->SaveChanges();
	}

	PadDevice* ConnectedDevice() const { return myConnectedDevice.get(); }

	bool DiscoverDevice()
	{
		if(emulator) {
			auto reporter = make_unique<Reporter>();
			return ConnectToDeviceStage2(reporter, NULL);
		}

		auto foundDevices = hid_enumerate(0, 0);

		// Devices that are incompatible or had a communication failure are tracked in a failed device list to prevent
		// a loop of reconnection attempts. Remove unplugged devices from the list. Then, the user can attempt to
		// reconnect by plugging it back in, as it will be seen as a new device.

		for (auto it = myFailedDevices.begin(); it != myFailedDevices.end();)
		{
			if (!ContainsDevice(foundDevices, it->first))
			{
				Log::Writef(L"ConnectionManager :: failed device removed (%ls)", it->second.data());
				it = myFailedDevices.erase(it);
			}
			else ++it;
		}

		// Try to connect to the first compatible device that is not on the failed device list.

		for (auto device = foundDevices; device; device = device->next)
		{
			if (myFailedDevices.count(device->path) == 0 && ConnectToDeviceStage1(device))
				break;
		}

		hid_free_enumeration(foundDevices);
		return (bool)myConnectedDevice;
	}

	bool ConnectToDeviceStage1(hid_device_info* deviceInfo)
	{
		// Check if the vendor and product are compatible.

		bool compatible = false;

		for (auto id : HID_IDS)
		{
			if (deviceInfo->vendor_id == id.vendorId && deviceInfo->product_id == id.productId)
			{
				compatible = true;
				break;
			}
		}

		if (!compatible)
			return false;

		// Open and configure HID for communicating with the pad.

		auto hid = hid_open_path(deviceInfo->path);
		if (!hid)
		{
			Log::Writef(L"ConnectionManager :: hid_open failed (%ls) :: %ls", hid_error(nullptr), deviceInfo->path);
			AddIncompatibleDevice(deviceInfo);
			return false;
		}
		if (hid_set_nonblocking(hid, 1) < 0)
		{
			Log::Write(L"ConnectionManager :: hid_set_nonblocking failed");
			AddIncompatibleDevice(deviceInfo);
			hid_close(hid);
			return false;
		}

		// Try to read the pad configuration and name.
		// If both succeeded, we'll assume the device is valid.

		auto reporter = make_unique<Reporter>(hid);
		bool result = ConnectToDeviceStage2(reporter, deviceInfo);
		if(!result) {
			AddIncompatibleDevice(deviceInfo);
			hid_close(hid);
			return false;
		}
		
		return result;
	}

	bool ConnectToDeviceStage2(unique_ptr<Reporter>& reporter, hid_device_info* deviceInfo)
	{
		NameReport name;
		PadConfigurationReport padConfiguration;
		IdentificationReport padIdentification;
		if (!reporter->Get(name) || !reporter->Get(padConfiguration))
		{
			return false;
		}

		// The other checks were fine, which means the pad doesn't support identification yet. Loading defaults.
		if (!reporter->Get(padIdentification))
		{
			padIdentification.firmwareMajor = WriteU16LE(0);
			padIdentification.firmwareMinor = WriteU16LE(0);
			padIdentification.buttonCount = MAX_BUTTON_COUNT;
			padIdentification.sensorCount = MAX_SENSOR_COUNT;
			padIdentification.ledCount = 0;
			padIdentification.maxSensorValue = WriteU16LE(MAX_SENSOR_VALUE);
			memset(padIdentification.boardType, 0, BOARD_TYPE_LENGTH);
			strcpy(padIdentification.boardType, "unknown");
		}

		// If we got some lights, try to read the light rules.
		vector<LightRuleReport> lightRules;
		vector<LedMappingReport> ledMappings;
		if (padIdentification.ledCount > 0)
		{
			SetPropertyReport selectReport;

			LightRuleReport lightReport;
			selectReport.propertyId = WriteU32LE(SetPropertyReport::SELECTED_LIGHT_RULE_INDEX);
			for (int i = 0; i < MAX_LIGHT_RULES; ++i)
			{
				selectReport.propertyValue = WriteU32LE(i);
				bool sendResult = reporter->Send(selectReport);

				if (sendResult && reporter->Get(lightReport) && (lightReport.flags & LRF_ENABLED))
				{
					PrintLightRuleReport(lightReport);
					lightRules.push_back(lightReport);
				}
			}

			LedMappingReport ledReport;
			selectReport.propertyId = WriteU32LE(SetPropertyReport::SELECTED_LED_MAPPING_INDEX);
			for (int i = 0; i < MAX_LED_MAPPINGS; ++i)
			{
				selectReport.propertyValue = WriteU32LE(i);
				bool sendResult = reporter->Send(selectReport);

				if (sendResult && reporter->Get(ledReport) && (ledReport.flags & LMF_ENABLED))
				{
					PrintLedMappingReport(ledReport);
					ledMappings.push_back(ledReport);
				}
			}
		}
		
		string devicePath = "";
		if(deviceInfo != NULL) {
			devicePath = deviceInfo->path;
		}
		else if(emulator) {
			devicePath = "Dummy";
		}

		auto device = new PadDevice(
			reporter,
			devicePath.c_str(),
			name,
			padConfiguration,
			padIdentification,
			lightRules,
			ledMappings);

		auto boardTypeString = BoardTypeToString(device->State().boardType);
		Log::Write(L"ConnectionManager :: new device connected [");
		Log::Writef(L"  Name: %ls", device->State().name.data());
		Log::Writef(L"  Board: %ls", boardTypeString.c_str());
		Log::Writef(L"  Firmware version: v%u.%u", ReadU16LE(padIdentification.firmwareMajor), ReadU16LE(padIdentification.firmwareMinor));
		if(deviceInfo != NULL) {
			Log::Writef(L"  Product: %ls", deviceInfo->product_string);
			Log::Writef(L"  Manufacturer: %ls", deviceInfo->manufacturer_string);
			Log::Writef(L"  Path: %ls", widen(deviceInfo->path, strlen(deviceInfo->path)).data());
		}
		else {
			Log::Writef(L"  Product: Dummy");
		}
		Log::Write(L"]");
		PrintPadConfigurationReport(padConfiguration);

		myConnectedDevice.reset(device);
		return true;
	}

	void DisconnectFailedDevice()
	{
		auto device = myConnectedDevice.get();
		if (device)
		{
			myFailedDevices[device->Path()] = device->State().name;
			myConnectedDevice.reset();
		}
	}

	void AddIncompatibleDevice(hid_device_info* device)
	{
		if (device->product_string) // Can be null on failure, apparently.
			myFailedDevices[device->path] = device->product_string;
	}

private:
	unique_ptr<PadDevice> myConnectedDevice;
	map<DevicePath, DeviceName> myFailedDevices;
	bool emulator = true;
};

// ====================================================================================================================
// Device API.
// ====================================================================================================================

static ConnectionManager* connectionManager = nullptr;
static bool searching = true;

void Device::Init()
{
	hid_init();

	connectionManager = new ConnectionManager();
	
	searching = true;
}

void Device::Shutdown()
{
	delete connectionManager;
	connectionManager = nullptr;

	hid_exit();
}

DeviceChanges Device::Update()
{
	DeviceChanges changes = 0;

	// If there is currently no connected device, try to find one.
	auto device = connectionManager->ConnectedDevice();
	if (!device && searching)
	{
		if (connectionManager->DiscoverDevice())
			changes |= DCF_DEVICE;

		device = connectionManager->ConnectedDevice();
	}

	// If there is a device, update it.
	if (device)
	{
		changes |= device->PopChanges();
		if (!device->UpdateSensorValues())
		{
			connectionManager->DisconnectFailedDevice();
			changes |= DCF_DEVICE;
		}
	}

	return changes;
}

int Device::PollingRate()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->PollingRate() : 0;
}

const PadState* Device::Pad()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? &device->State() : nullptr;
}

const LightsState* Device::Lights()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? &device->Lights() : nullptr;
}

const SensorState* Device::Sensor(int sensorIndex)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->Sensor(sensorIndex) : nullptr;
}

bool Device::SetThreshold(int sensorIndex, double threshold)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetThreshold(sensorIndex, threshold) : false;
}

bool Device::SetReleaseThreshold(double threshold)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetReleaseThreshold(threshold) : false;
}

bool Device::SetButtonMapping(int sensorIndex, int button)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetButtonMapping(sensorIndex, button) : false;
}

bool Device::SetDeviceName(const wchar_t* name)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SendName(name) : false;
}

bool Device::SendLedMapping(int ledMappingIndex, LedMapping mapping)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SendLedMapping(ledMappingIndex, mapping) : false;
}

bool Device::DisableLedMapping(int ledMappingIndex)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->DisableLedMapping(ledMappingIndex) : false;
}

bool Device::SendLightRule(int lightRuleIndex, LightRule rule)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SendLightRule(lightRuleIndex, rule) : false;
}

bool Device::DisableLightRule(int lightRuleIndex)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->DisableLightRule(lightRuleIndex) : false;
}

void Device::SendDeviceReset()
{
	auto device = connectionManager->ConnectedDevice();
	if (device) device->Reset();
}

void Device::SendFactoryReset()
{
	auto device = connectionManager->ConnectedDevice();
	if (device) device->FactoryReset();
}

void Device::SetSearching(bool s)
{
	searching = s;
}

void Device::LoadProfile(json& j, DeviceProfileGroups groups)
{
	if(groups & DPG_LIGHTS) {
		if(j["ledMappings"].is_array()) {
			for(int key = 0; key < j.count("ledMappings"); key++) {
				auto value = j["ledMappings"][key];
				
				LedMapping lm = {
					value["lightRuleIndex"],
					value["sensorIndex"],
					value["ledIndexBegin"],
					value["ledIndexEnd"]
				};
				
				SendLedMapping(key, lm);
			}
		}
		
		if(j["lightRules"].is_array()) {
			for(int key = 0; key < j.count("lightRules"); key++) {
				auto value = j["lightRules"][key];
				
				LightRule lr = {
					value["fadeOn"],
					value["fadeOff"],
					RgbColor(value["onColor"]),
					RgbColor(value["offColor"]),
					RgbColor(value["fadeOnColor"]),
					RgbColor(value["fadeOffColor"])
				};
				
				SendLightRule(key, lr);
			}
		}
	}
	
	if(groups & DPG_SENSITIVITY) {
		if(j["sensitivity"].is_array()) {
			for(int key = 0; key < j.count("sensitivity"); key++) {
				float value = j["sensitivity"][key];
				
				if(key < 0 || key > Device::Pad()->numSensors) {
					continue;
				}
				
				SetThreshold(key, value);
			}
		}
		
		if(j["releaseThreshold"].is_number()) {
			SetReleaseThreshold(j["releaseThreshold"]);
		}
	}
	
	if(groups & DPG_MAPPING) {
		if(j["mapping"].is_array()) {
			for(int key = 0; key < j.count("mapping"); key++) {
				int value = j["mapping"][key];
				
				if(key < 0 || key > Device::Pad()->numSensors) {
					continue;
				}
				
				if(value < 0 || value > Device::Pad()->numButtons) {
					continue;
				}
				
				SetButtonMapping(key, value);
			}
		}
	}
	
	if(groups & DPG_DEVICE) {
		string name = j["name"];
		SetDeviceName(widen(name.c_str(), name.length()).c_str());
	}
}

void Device::SaveProfile(json& j, DeviceProfileGroups groups)
{
	j["adpToolVersion"] = wxString::Format("v%i.%i", ADP_VERSION_MAJOR, ADP_VERSION_MINOR);

	if(groups & DPG_LIGHTS) {
		auto lights = Device::Lights();
		
		j["ledMappings"] = json::array();
		for(const auto& [index, lm] : lights->ledMappings) {
			j["ledMappings"][index]["lightRuleIndex"] = lm.lightRuleIndex;
			j["ledMappings"][index]["sensorIndex"] = lm.sensorIndex;
			j["ledMappings"][index]["ledIndexBegin"] = lm.ledIndexBegin;
			j["ledMappings"][index]["ledIndexEnd"] = lm.ledIndexEnd;
		}
		
		j["lightRules"] = json::array();
		for(const auto& [index, lr] : lights->lightRules) {
			j["lightRules"][index]["fadeOn"] = lr.fadeOn;
			j["lightRules"][index]["fadeOff"] = lr.fadeOff;
			j["lightRules"][index]["onColor"] = lr.onColor.ToString();
			j["lightRules"][index]["offColor"] = lr.offColor.ToString();
			j["lightRules"][index]["onFadeColor"] = lr.onFadeColor.ToString();
			j["lightRules"][index]["offFadeColor"] = lr.offFadeColor.ToString();
		}
		
	}
	
	if(groups & DPG_SENSITIVITY) {
		j["sensitivity"] = json::array();
		for (int i = 0; i < Device::Pad()->numSensors; ++i)
		{
			j["sensitivity"][i] = Device::Sensor(i)->threshold;
		}
		
		j["releaseThreshold"] = Device::Pad()->releaseThreshold;
	}
		
	if(groups & DPG_MAPPING) {
		j["mapping"] = json::array();
		for (int i = 0; i < Device::Pad()->numSensors; ++i)
		{
			j["mapping"][i] = Device::Sensor(i)->button;
		}
	}

	if(groups & DPG_DEVICE) {
		const wchar_t* name = Pad()->name.c_str();
		j["name"] = narrow(name, wcslen(name));
	}
}

}; // namespace adp.
