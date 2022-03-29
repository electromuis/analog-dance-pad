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
#include "Model/Updater.h"

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

SensorReport SensorState::ToReport(int index)
{
	SensorReport report;

	report.index = index;
	report.threshold = WriteU16LE(ToDeviceSensorValue(threshold));
	report.releaseThreshold = WriteU16LE(ToDeviceSensorValue(releaseThreshold));
	report.resistorValue = resistorValue;
	report.buttonMapping = button == 0 ? 0xFF : (button - 1);

	return report;
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

static void PrintSensorReport(const SensorReport& r)
{
	Log::Write(L"sensor config[");
	Log::Writef(L"  index: %i", r.index);
	Log::Writef(L"  threshold: %i", ReadU16LE(r.threshold));
	Log::Writef(L"  releaseThreshold: %i", ReadU16LE(r.releaseThreshold));
	Log::Writef(L"  buttonMapping: %i", r.buttonMapping);
	Log::Writef(L"  resistorValue: %i", r.resistorValue);
	Log::Writef(L"  flags: %i", ReadU16LE(r.flags));
	Log::Write(L"]");
}

// ====================================================================================================================
// Pad device.
// ====================================================================================================================

typedef string DevicePath;
typedef string DeviceName;

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
		const IdentificationV2Report& identification,
		const vector<LightRuleReport>& lightRules,
		const vector<LedMappingReport>& ledMappings,
		const vector<SensorReport>& sensors)
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

		myPad.firmwareVersion.major = ReadU16LE(identification.firmwareMajor);
		myPad.firmwareVersion.minor = ReadU16LE(identification.firmwareMinor);

		uint16_t features = ReadU16LE(identification.features);
		myPad.featureDebug = (features & IdentificationV2Report::FEATURE_DEBUG) != 0;
		myPad.featureDigipot = (features & IdentificationV2Report::FEATURE_DIGIPOT) != 0;
		myPad.featureLights = (features & IdentificationV2Report::FEATURE_LIGHTS) != 0;

		for (auto sensor : sensors)
		{
			UpdateSensor(sensor);
		}

		if (myPad.firmwareVersion.IsNewer({ 1, 2 })) {
			myPad.releaseThreshold = mySensors[0].releaseThreshold / mySensors[0].threshold;
		}
		else if (myPad.firmwareVersion.IsNewer({ 1, 1 })) {
			myPad.featureLights = (bool)(identification.ledCount > 0);
		}

		UpdateLightsConfiguration(lightRules, ledMappings);
		myPollingData.lastUpdate = system_clock::now();
	}

	~PadDevice()
	{

	}

	void UpdateName(const NameReport& report)
	{
        if(report.size <= MAX_NAME_LENGTH) {
            myPad.name = "";
            myPad.name.append((const char*)report.name, (size_t)report.size);
        }
        else {
            myPad.name = "Unknown";
        }

        myChanges |= DCF_NAME;
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

		// Use the loop to save changes if needed
		if (myHasUnsavedChanges && duration_cast<std::chrono::milliseconds>(now - myLastPendingChange).count() > 2000) {
			SaveChanges();
		}

		return true;
	}

	bool SetThreshold(int sensorIndex, double threshold)
	{
		mySensors[sensorIndex].threshold = threshold;
		mySensors[sensorIndex].releaseThreshold = threshold * myPad.releaseThreshold;

		// From v1.3 we have the SensorReport. Before that it's the PadConfiguration report
		if (myPad.firmwareVersion.IsNewer({ 1, 2 })) {
			return SendSensor(sensorIndex);
		}
		else {
			return SendPadConfiguration();
		}
	}

	bool SetReleaseThreshold(double threshold)
	{
		myPad.releaseThreshold = clamp(threshold, 0.01, 1.00);

		// From v1.3 we have the SensorReport. Before that it's the PadConfiguration report
		if (myPad.firmwareVersion.IsNewer({ 1, 2 })) {
			for (int i = 0; i < myPad.numSensors; ++i) {
				mySensors[i].releaseThreshold = mySensors[i].threshold * myPad.releaseThreshold;
				if (!SendSensor(i)) {
					return false;
				}
			}

			return true;
		}
		else {
			return SendPadConfiguration();
		}
	}

	bool SetAdcConfig(int sensorIndex, int resistorValue)
	{
		mySensors[sensorIndex].resistorValue = resistorValue;

		return SendSensor(sensorIndex);
	}

	void UpdateSensor(SensorReport sensor)
	{
		if (sensor.index < 0 || sensor.index > myPad.numSensors) {
			return;
		}

		mySensors[sensor.index].threshold = ToNormalizedSensorValue(ReadU16LE(sensor.threshold));
		mySensors[sensor.index].releaseThreshold = ToNormalizedSensorValue(ReadU16LE(sensor.releaseThreshold));
		mySensors[sensor.index].resistorValue = sensor.resistorValue;
		mySensors[sensor.index].button = (sensor.buttonMapping >= myPad.numButtons ? 0 : (sensor.buttonMapping + 1));
	}

	bool SendSensor(int sensorIndex)
	{
		SensorReport report = mySensors[sensorIndex].ToReport(sensorIndex);

		bool success = myReporter->Send(report);

		if (success) {
			NotifyUnsavedChanges();
			UpdateSensor(report);
		}

		return success;
	}

	bool SetButtonMapping(int sensorIndex, int button)
	{
		mySensors[sensorIndex].button = button;
		myChanges |= DCF_BUTTON_MAPPING;

		// From v1.3 we have the SensorReport. Before that it's the PadConfiguration report
		if (myPad.firmwareVersion.IsNewer({ 1, 2 })) {
			return SendSensor(sensorIndex);
		}
		else {
			return SendPadConfiguration();
		}
	}

	bool SendName(const char* name)
	{

		NameReport report;
		int length = strlen(name);

		if (length > sizeof(report.name))
		{
			Log::Writef(L"SetName :: name '%hs' exceeds %i chars and was not set", name, sizeof(report.name));
			return false;
		}

		report.size = (uint8_t)length;
		memcpy(report.name, name, length);
		bool result = myReporter->SendAndGet(report);
		NotifyUnsavedChanges();
		UpdateName(report);
		return result;
	}

	bool SendLedMappingReport(const LedMappingReport& report)
	{
		if (!myReporter->Send(report))
			return false;

		UpdateLedMapping(report);

        // Only set when to update the tab
		// myChanges |= DCF_LIGHTS;

		NotifyUnsavedChanges();
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

		// Only set when to update the tab
		// myChanges |= DCF_LIGHTS;

		NotifyUnsavedChanges();
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

	void CalibrateSensor(int sensorIndex)
	{
		SetPropertyReport report;
		report.propertyId = WriteU32LE(SetPropertyReport::CALIBRATE_SENSOR);
		report.propertyValue = WriteU32LE(sensorIndex);
		bool result = myReporter->Send(report);
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

		bool result = myReporter->SendAndGet(report);

		NotifyUnsavedChanges();
		return result;
	}

	void NotifyUnsavedChanges()
	{
		myHasUnsavedChanges = true;
		myLastPendingChange = system_clock::now();
	}

	bool HasUnsavedChanges()
	{
		return myHasUnsavedChanges;
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

	wstring ReadDebug()
	{
		if (!myPad.featureDebug) {
			return L"";
		}

		DebugReport report;
		if (!myReporter->Get(report)) {
			return L"";
		}

		int messageSize = ReadU16LE(report.messageSize);

		if (messageSize == 0) {
			return L"";
		}

		return widen(report.messagePacket, messageSize);
	}

	DeviceChanges PopChanges()
	{
		auto result = myChanges;
		myChanges = 0;
		return result;
	}

	void TriggerChange(int type)
	{
        myChanges |= type;
	}

private:
	unique_ptr<Reporter> myReporter;
	DevicePath myPath;
	PadState myPad;
	LightsState myLights;
	SensorState mySensors[MAX_SENSOR_COUNT];
	DeviceChanges myChanges = 0;
	bool myHasUnsavedChanges = false;
	time_point<system_clock> myLastPendingChange;
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

		auto foundDevices = hid_enumerate(0x0, 0x0);

		// Devices that are incompatible or had a communication failure are tracked in a failed device list to prevent
		// a loop of reconnection attempts. Remove unplugged devices from the list. Then, the user can attempt to
		// reconnect by plugging it back in, as it will be seen as a new device.

		for (auto it = myFailedDevices.begin(); it != myFailedDevices.end();)
		{
			if (!ContainsDevice(foundDevices, it->first))
			{
				Log::Writef(L"ConnectionManager :: failed device removed (%hs)", it->second.data());
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

		using namespace std::chrono_literals;
		// Wait for any udev rules to run
		std::this_thread::sleep_for(200ms);

		// Open and configure HID for communicating with the pad.

		auto hid = hid_open_path(deviceInfo->path);
		if (!hid)
		{
			Log::Writef(L"ConnectionManager :: hid_open failed (%ls) :: %hs", hid_error(nullptr), deviceInfo->path);

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
			// hid_close already happend becuase Reporter gets destructed
			return false;
		}

		return result;
	}

	bool ConnectToDeviceStage2(unique_ptr<Reporter>& reporter, hid_device_info* deviceInfo)
	{
		NameReport name;
		IdentificationReport padIdentification;
		IdentificationV2Report padIdentificationV2;
		vector<SensorReport> sensors;

		if (!reporter->Get(name))
		{
			return false;
		}

		VersionType padVersion = versionTypeUnknown;

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

			memcpy(&padIdentificationV2, &padIdentification, sizeof(padIdentification));
			padIdentificationV2.features = WriteU16LE(0);
		}
		else
		{
			padVersion = { (uint16_t)ReadU16LE(padIdentification.firmwareMajor), (uint16_t)ReadU16LE(padIdentification.firmwareMinor) };

			if (padVersion.IsNewer({1, 2})) {
				if (!reporter->Get(padIdentificationV2)) {
					memcpy(&padIdentificationV2, &padIdentification, sizeof(padIdentification));
					padIdentificationV2.features = WriteU16LE(0);
				}
			}
			else {
				memcpy(&padIdentificationV2, &padIdentification, sizeof(padIdentification));
				padIdentificationV2.features = WriteU16LE(0);
			}
		}

		// If we got some lights, try to read the light rules.
		vector<LightRuleReport> lightRules;
		vector<LedMappingReport> ledMappings;
		if (padIdentification.ledCount > 0 && padVersion.IsNewer({1, 1}))
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

		SensorReport sensorReport;
		if (padVersion.IsNewer({ 1, 2 })) {
			SetPropertyReport selectReport;
			selectReport.propertyId = WriteU32LE(SetPropertyReport::SELECTED_SENSOR_INDEX);

			for (int i = 0; i < padIdentificationV2.sensorCount; ++i)
			{
				selectReport.propertyValue = WriteU32LE(i);
				bool sendResult = reporter->Send(selectReport);

				if (sendResult && reporter->Get(sensorReport))
				{
					PrintSensorReport(sensorReport);
					sensors.push_back(sensorReport);
				}
			}
		}
		else {
			// Backwards compat
			PadConfigurationReport padConfig;
			if (reporter->Get(padConfig)) {
				for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
				{
					sensorReport.index = i;
					sensorReport.threshold = padConfig.sensorThresholds[i];
					sensorReport.releaseThreshold = WriteU16LE(ReadU16LE(padConfig.sensorThresholds[i]) * ReadF32LE(padConfig.releaseThreshold));
					sensorReport.buttonMapping = padConfig.sensorToButtonMapping[i];
					sensorReport.resistorValue = 0;
					sensorReport.flags = WriteU16LE(0);

					sensors.push_back(sensorReport);
				}
			}
		}

		auto device = new PadDevice(
			reporter,
			devicePath.c_str(),
			name,
			padIdentificationV2,
			lightRules,
			ledMappings,
			sensors);

		Log::Write(L"ConnectionManager :: new device connected [");
		Log::Writef(L"  Name: %hs", device->State().name.c_str());
		Log::Writef(L"  Board: %ls", BoardTypeToString(device->State().boardType));
		Log::Writef(L"  Firmware version: v%u.%u", ReadU16LE(padIdentificationV2.firmwareMajor), ReadU16LE(padIdentificationV2.firmwareMinor));
		Log::Writef(L"  Feautre flags: %u", ReadU16LE(padIdentificationV2.features));
		if(deviceInfo != NULL) {
			Log::Writef(L"  Product: %ls", deviceInfo->product_string);
			Log::Writef(L"  Manufacturer: %ls", deviceInfo->manufacturer_string);
			Log::Writef(L"  Path: %hs", deviceInfo->path);
		}
		else {
			Log::Writef(L"  Product: Dummy");
		}
		Log::Write(L"]");

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
			myFailedDevices[device->path] = narrow(device->product_string, wcslen(device->product_string));
	}

private:
	unique_ptr<PadDevice> myConnectedDevice;
	map<DevicePath, DeviceName> myFailedDevices;
	bool emulator = false;
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

wstring Device::ReadDebug()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->ReadDebug() : L"";
}

const bool Device::HasUnsavedChanges()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->HasUnsavedChanges() : false;
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

bool Device::SetAdcConfig(int sensorIndex, int resistorValue)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetAdcConfig(sensorIndex, resistorValue) : false;
}

bool Device::SetButtonMapping(int sensorIndex, int button)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetButtonMapping(sensorIndex, button) : false;
}

bool Device::SetDeviceName(const char* name)
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

void Device::CalibrateSensor(int sensorIndex)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->CalibrateSensor(sensorIndex) : false;
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

void Device::SaveChanges()
{
	auto device = connectionManager->ConnectedDevice();
	if (device) device->SaveChanges();
}

void Device::SetSearching(bool s)
{
	searching = s;
}

void Device::LoadProfile(json& j, DeviceProfileGroups groups)
{
	if((groups & DPG_LIGHTS) > 0 && Pad()->featureLights) {
		if(j["ledMappings"].is_array()) {
			for(int key = 0; key < j["ledMappings"].size(); key++) {
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
			for(int key = 0; key < j["lightRules"].size(); key++) {
				auto value = j["lightRules"][key];

				LightRule lr = {
					value["fadeOn"],
					value["fadeOff"],
					value["onColor"].is_string() ? RgbColor((string)value["onColor"]) : RgbColor(0,0,0),
					value["offColor"].is_string() ? RgbColor((string)value["offColor"]) : RgbColor(0,0,0),
					value["onFadeColor"].is_string() ? RgbColor((string)value["onFadeColor"]) : RgbColor(0,0,0),
					value["offFadeColor"].is_string() ? RgbColor((string)value["offFadeColor"]) : RgbColor(0,0,0)
				};

				SendLightRule(key, lr);
			}
		}

		auto device = connectionManager->ConnectedDevice();
		if(device) {
            device->TriggerChange(DCF_LIGHTS);
		}
	}

	if (j["sensors"].is_array()) {
		for (int key = 0; key < j["sensors"].size(); key++) {
			auto sensor = j["sensors"][key];

			if (groups & DPG_SENSITIVITY && sensor.contains("threshold")) {
				SetThreshold(key, sensor["threshold"]);
			}

			if (groups & DPG_MAPPING && sensor.contains("button")) {
				SetButtonMapping(key, sensor["button"]);
			}

			if (groups & DPG_MAPPING && sensor.contains("resistorValue") && Pad()->featureDigipot) {
				SetAdcConfig(key, sensor["resistorValue"]);
			}
		}
	}

	if(groups & DPG_SENSITIVITY) {
		if(j["releaseThreshold"].is_number()) {
			SetReleaseThreshold(j["releaseThreshold"]);
		}
	}

	if(groups & DPG_DEVICE) {
		string name = j["name"];
		SetDeviceName( ((std::string)j["name"]).c_str() );
	}
}

void Device::SaveProfile(json& j, DeviceProfileGroups groups)
{
	j["adpToolVersion"] = wxString::Format("v%i.%i", ADP_VERSION_MAJOR, ADP_VERSION_MINOR);

	if((groups & DPG_LIGHTS) && Pad()->featureLights && Lights()) {
		auto lights = Lights();

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

	if(groups & (DPG_SENSITIVITY | DPG_MAPPING)) {
		j["sensors"] = json::array();
		for (int i = 0; i < Device::Pad()->numSensors; ++i)
		{
			if (groups & DPG_SENSITIVITY) {
				j["sensors"][i]["threshold"] = Device::Sensor(i)->threshold;
				j["sensors"][i]["releaseThreshold"] = Device::Sensor(i)->releaseThreshold;
			}

			if (groups & DPG_MAPPING) {
				j["sensors"][i]["button"] = Device::Sensor(i)->button;
				j["sensors"][i]["resistorValue"] = Device::Sensor(i)->resistorValue;
			}
		}

		j["releaseThreshold"] = Device::Pad()->releaseThreshold;
	}

	if(groups & DPG_DEVICE) {
		j["name"] = Pad()->name;
	}
}

}; // namespace adp.
