#include <Adp.h>

#include <memory>
#include <algorithm>
#include <map>
#include <chrono>
#include <thread>

#include "hidapi.h"
#include <fmt/core.h>

#include <Model/Device.h>
#include <Model/Reporter.h>
#include <Model/Log.h>
#include <Model/Utils.h>
#include <Model/Firmware.h>
#include <Model/Updater.h>

using namespace std;
using namespace chrono;

typedef std::function<void(uint8_t reportId, std::vector<uint8_t> data)> HidReadCallback;
int HID_API_EXPORT HID_API_CALL hid_read_register(HidReadCallback callback);

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


RgbColor::RgbColor(uint8_t r, uint8_t g, uint8_t b)
	: red(r), green(g), blue(b)
{
}

RgbColor::RgbColor(const std::string& input)
	: RgbColor()
{
	const char* p = input.data();
	if (*p == '#') ++p;
	sscanf(p, "%02hhx%02hhx%02hhx", &red, &green, &blue);
}

RgbColor::RgbColor()
	: red(0), green(0), blue(0)
{
}

std::string RgbColor::ToString() const
{
	return fmt::format("#{:02x}{:02x}{:02x}", red, green, blue);
}

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
	Log::Write("pad configuration [");
	Log::Writef("  releaseThreshold: %.2f", ReadF32LE(padConfiguration.releaseThreshold));
	Log::Write("  sensors: [");
	for (int i = 0; i < SENSOR_COUNT_V1; ++i)
	{
		Log::Writef("    sensorToButtonMapping: %i", padConfiguration.sensorToButtonMapping[i]);
		Log::Writef("    sensorThresholds: %i", ReadU16LE(padConfiguration.sensorThresholds[i]));
	}
	Log::Write("  ]");
	Log::Write("]");
}

static void PrintLightRuleReport(const LightRuleReport& r)
{
	Log::Write("light rule [");
	Log::Writef("  lightRuleIndex: %i", r.lightRuleIndex);
	Log::Writef("  flags: %s", fmt::format("{:b}", r.flags).c_str());
	Log::Writef("  onColor: [R%i G%i B%i]", r.onColor.red, r.onColor.green, r.onColor.blue);
	Log::Writef("  offColor: [R%i G%i B%i]", r.offColor.red, r.offColor.green, r.offColor.blue);
	Log::Writef("  onFadeColor: [R%i G%i B%i]", r.onFadeColor.red, r.onFadeColor.green, r.onFadeColor.blue);
	Log::Writef("  offFadeColor: [R%i G%i B%i]", r.offFadeColor.red, r.offFadeColor.green, r.offFadeColor.blue);
	Log::Write("]");
}

static void PrintLedMappingReport(const LedMappingReport& r)
{
	Log::Write("led mapping [");
	Log::Writef("  ledMappingIndex: %i", r.ledMappingIndex);
	Log::Writef("  flags: %s", fmt::format("{:b}", r.flags).c_str());
	Log::Writef("  lightRuleIndex: %i", r.lightRuleIndex);
	Log::Writef("  sensorIndex: %i", r.sensorIndex);
	Log::Writef("  ledIndexBegin: %i", r.ledIndexBegin);
	Log::Writef("  ledIndexEnd: %i", r.ledIndexEnd);
	Log::Write("]");
}

static void PrintSensorReport(const SensorReport& r)
{
	Log::Write("sensor config[");
	Log::Writef("  mappingIndex: %i", r.index);
	Log::Writef("  threshold: %i", ReadU16LE(r.threshold));
	Log::Writef("  releaseThreshold: %i", ReadU16LE(r.releaseThreshold));
	Log::Writef("  buttonMapping: %i", r.buttonMapping);
	Log::Writef("  resistorValue: %i", r.resistorValue);
	Log::Writef("  flags: %s", fmt::format("{:b}", ReadU16LE(r.flags)).c_str());
	Log::Write("]");
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
		: myReporter(reporter)
		, myPath(path)
	{
		mySensors = vector<SensorState>(identification.sensorCount);

		UpdateName(name);
		myPad.maxNameLength = MAX_NAME_LENGTH;

		myPad.numButtons = identification.buttonCount;
		myPad.numSensors = identification.sensorCount;

		auto buffer = (char*)calloc(BOARD_TYPE_LENGTH + 1, sizeof(char));
		if (buffer)
		{
			memcpy(buffer, identification.boardType, BOARD_TYPE_LENGTH);
			myPad.boardType = BoardTypeStruct(buffer);
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

		try {
			SetPropertyReport report;
			report.propertyId = WriteU32LE(SetPropertyReport::SPID_SELECTED_PROPERTY);
			report.propertyValue = WriteU32LE(SetPropertyReport::SPID_RELEASE_MODE);

			if (myReporter->SendAndGet(report)) {
				int resultId = ReadU32LE(report.propertyId);
				if (resultId != SetPropertyReport::SPID_RELEASE_MODE) {
					Log::Write("Fetching release mode bugged (1)");		
				} else {
					myPad.releaseMode = (ReleaseMode)ReadU32LE(report.propertyValue);
				}
			}
		} catch(...) {
			Log::Write("Fetching release mode failed");
		}

		UpdateLightsConfiguration(lightRules, ledMappings);
		myPollingData.lastUpdate = system_clock::now();

		running = true;
		sensorThread = std::thread(&PadDevice::UpdateSensorThread, this);
	}

	~PadDevice()
	{
		running = false;
		if (sensorThread.joinable())
			sensorThread.join();
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
		return running;
	}

	void UpdateSensorThread()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		myPollingData.lastUpdate = system_clock::now();
		while(running)
		{
			if(!UpdateSensorFunc())
			{
				running = false;
				break;
			}

			this_thread::sleep_for(chrono::milliseconds(16));
		}
	}

	bool UpdateSensorFunc()
	{
		SensorValuesReport report;
		std::vector<int> aggregateValues = std::vector<int>(myPad.numSensors, 0);

		int pressedButtons = 0;
		int inputsRead = 0;

		for (int readsLeft = 100; readsLeft > 0; --readsLeft)
		{
			switch (myReporter->Get(report, myPad.numSensors))
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

	bool SetReleaseMode(ReleaseMode mode)
	{
		myPad.releaseMode = mode;

		SetPropertyReport report;
		report.propertyId = WriteU32LE(SetPropertyReport::SPID_RELEASE_MODE);
		report.propertyValue = WriteU32LE((uint32_t)mode);
		
		return myReporter->Send(report);
	}

	bool SetThreshold(int sensorIndex, double threshold, double releaseThreshold)
	{
		mySensors[sensorIndex].threshold = threshold;
		mySensors[sensorIndex].releaseThreshold = releaseThreshold;

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
		auto length = strlen(name);

		if (length > sizeof(report.name))
		{
			Log::Writef("SetName :: name '%hs' exceeds %i chars and was not set", name, sizeof(report.name));
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
		if(index < 0 || index >= myPad.numSensors) {
			return nullptr;
		}

		return &mySensors[index];
	}

	std::string ReadDebug()
	{
		if (!myPad.featureDebug) {
			return "";
		}

		DebugReport report;
		if (!myReporter->Get(report)) {
			return "";
		}

		int messageSize = ReadU16LE(report.messageSize);

		if (messageSize == 0) {
			return "";
		}

		return report.messagePacket;
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

#ifdef DEVICE_SERVER_ENABLED
	void ServerStart()
	{
		myReporter->ServerStart();
	}
#endif

private:
	bool running = false;
	std::thread sensorThread;

	unique_ptr<Reporter>& myReporter;
	DevicePath myPath;
	PadState myPad;
	LightsState myLights;
	vector<SensorState> mySensors;
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

enum ConnectionState
{
	CS_UNKNOWN,
	CS_PROBED,
	CS_CONNECTED,
	CS_FAILED
};

class DeviceConnection
{
public:
	DeviceConnection(std::string path):
		path(path)
	{

	}

	~DeviceConnection()
	{
		if (hidHandle)
			hid_close(hidHandle);
	}

	bool Probe()
	{
		if(state == CS_FAILED)
			return false;

		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		if (!IsWs())
		{
			hidHandle = hid_open_path(path.c_str());
			if (!hidHandle)
			{
				Log::Writef("DeviceConnection :: hid_open failed (%ls) :: %s", hid_error(nullptr), path.c_str());
				state = CS_FAILED;
				return false;
			}

			if (hid_set_nonblocking(hidHandle, 1) < 0)
			{
				Log::Write("ConnectionManager :: hid_set_nonblocking failed");
				state = CS_FAILED;
				return false;
			}

			reporter = make_unique<Reporter>(hidHandle);
		}
		else {
			reporter = make_unique<Reporter>(path);
		}

		if(!reporter->Get(nameReport))
		{
			state = CS_FAILED;
			return false;
		}

		if(!reporter->Get(identificationReport))
		{
			state = CS_FAILED;
			return false;
		}

		state = CS_PROBED;
		return true;
	}

	string GetName(bool update = false)
	{
		if(update) {
			reporter->Get(nameReport);
		}

		return string((const char*)nameReport.name, nameReport.size);
	}

	string GetPath()
	{
		return path;
	}

	unique_ptr<Reporter>& GetReporter()
	{
		return reporter;
	}

	ConnectionState GetState()
	{
		return state;
	}

	void SetFailed()
	{
		state = CS_FAILED;
		reporter.reset();
	}

	bool IsWs()
	{
		return path.substr(0, 2).compare("ws") == 0;
	}

	bool ConnectStage2();

protected:
	ConnectionState state = CS_UNKNOWN;
	std::string path;
	hid_device* hidHandle = nullptr;

	unique_ptr<Reporter> reporter;
	NameReport nameReport;
	IdentificationV2Report identificationReport;
};

class ConnectionManager
{
public:
	~ConnectionManager()
	{
		if (myConnectedDevice)
			myConnectedDevice->SaveChanges();
	}

	PadDevice* ConnectedDevice() const { return myConnectedDevice.get(); }

	void UpdateDeviceMap(std::vector<string>& devicePaths)
	{
		for (auto id : HID_IDS)
		{
			auto foundDevices = hid_enumerate(id.vendorId, id.productId);
			for (auto dev = foundDevices; dev; dev = dev->next)
			{
				devicePaths.push_back(dev->path);
				// if(!newDevice->Probe())
				// 	continue;
				// out.push_back(newDevice);
			}
		}
	}

	bool DiscoverDevice()
	{
		std::vector<string> devicePaths;
		UpdateDeviceMap(devicePaths);

		// Remove any devices that are no longer connected
		for (auto it = devices.begin(); it != devices.end();)
		{
			if (std::find(devicePaths.begin(), devicePaths.end(), it->first) == devicePaths.end())
			{
				if(myConnectedDevice && myConnectedDevice->Path() == it->first)
					myConnectedDevice.reset();

				if (it->second.IsWs() && it->second.GetState() != CS_FAILED)
					continue;

				Log::Writef("ConnectionManager :: device removed (%hs)", it->second.GetName().c_str());
				it = devices.erase(it);
			}
			else ++it;
		}

		for(auto& path : devicePaths)
		{
			if(devices.contains(path))
				continue;

			auto it = devices.emplace(path, path);
			if(!it.first->second.Probe())
			{
				// devices.erase(it.first);
				continue;
			}
		}

		if(!myConnectedDevice)
		{
			int c=0;
			for(auto& it : devices)
			{
				if(it.second.GetState() != CS_FAILED)
					return DeviceSelect(c);
				c++;
			}
		}

		return false;
	}

	bool ConnectToDeviceStage2(DeviceConnection& deviceCon)
	{
		unique_ptr<Reporter>& reporter = deviceCon.GetReporter();
		string devicePath = deviceCon.GetPath();

		NameReport name;
		IdentificationReport padIdentification;
		IdentificationV2Report padIdentificationV2;
		vector<SensorReport> sensors;

		if (reporter == nullptr || !reporter->Get(name))
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
			padIdentification.sensorCount = SENSOR_COUNT_V1;
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
				if(!reporter->Send(selectReport))
					return false;

				if(!reporter->Get(lightReport))
					return false;

				if (lightReport.flags & LRF_ENABLED)
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
				if(!reporter->Send(selectReport))
					return false;

				if(!reporter->Get(ledReport))
					return false;

				if (ledReport.flags & LMF_ENABLED)
				{
					PrintLedMappingReport(ledReport);
					ledMappings.push_back(ledReport);
				}
			}
		}

		SensorReport sensorReport;
		if (padVersion.IsNewer({ 1, 2 })) {
			SetPropertyReport selectReport;
			selectReport.propertyId = WriteU32LE(SetPropertyReport::SELECTED_SENSOR_INDEX);

			for (int i = 0; i < padIdentificationV2.sensorCount; ++i)
			{
				selectReport.propertyValue = WriteU32LE(i);
				if(!reporter->Send(selectReport))
					return false;

				if(!reporter->Get(sensorReport))
					return false;

				PrintSensorReport(sensorReport);
				sensors.push_back(sensorReport);
			}
		}
		else {
			// Backwards compat
			PadConfigurationReport padConfig;
			if (reporter->Get(padConfig)) {
				for (int i = 0; i < SENSOR_COUNT_V1; ++i)
				{
					sensorReport.index = i;
					sensorReport.threshold = padConfig.sensorThresholds[i];
					auto th = ReadU16LE(padConfig.sensorThresholds[i]);
					auto rt = ReadF32LE(padConfig.releaseThreshold);
					sensorReport.releaseThreshold = WriteU16LE(int(th * rt));
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

		std::string boardType = device->State().boardType.ToString();

		Log::Write("ConnectionManager :: new device connected [");
		Log::Writef("  Name: %s", device->State().name.c_str());
		Log::Writef("  Board: %s: %s", padIdentificationV2.boardType, boardType.c_str());
		Log::Writef("  Firmware version: v%u.%u", ReadU16LE(padIdentificationV2.firmwareMajor), ReadU16LE(padIdentificationV2.firmwareMinor));
		Log::Writef("  Feautre flags: %s", fmt::format("{:b}", ReadU16LE(padIdentificationV2.features)).c_str());
		Log::Writef("  Path: %s", devicePath.c_str());
		
		/*
		if(deviceInfo != NULL) {
			Log::Writef("  Product: %ls", deviceInfo->product_string);
			#ifndef __EMSCRIPTEN__
			Log::Writef("  Manufacturer: %ls", deviceInfo->manufacturer_string);

			Log::Writef("  Path: %s", deviceInfo->path);
			#endif

		}
		else {
			Log::Writef("  Product: Dummy");
		}
		*/

		Log::Write("]");

		myConnectedDevice.reset(device);
		return true;
	}

	void DisconnectFailedDevice()
	{
		auto device = myConnectedDevice.get();
		if (device)
		{
			if(devices.contains(device->Path()))
				devices.at(device->Path()).SetFailed();

			myConnectedDevice.reset();
		}
	}

	void AddIncompatibleDevice(hid_device_info* device)
	{
		if (device->product_string) // Can be null on failure, apparently.
			myFailedDevices[device->path] = narrow(device->product_string, wcslen(device->product_string));
	}

	int DeviceNumber()
	{
		return (int)devices.size();
	}

	string GetDeviceName(int index, bool update = false)
	{
		if (index < 0 || index >= devices.size())
			return "";

		auto it = devices.begin();
		std::advance(it, index);
		return it->second.GetName(update);
	}

	bool DeviceSelect(int index)
	{
		if (index < 0 || index >= devices.size())
			return false;

		if(index == DeviceSelected())
			return true;

		auto it = devices.begin();
		std::advance(it, index);

		if(!it->second.ConnectStage2())
			return false;

		return true;
	}

	int DeviceSelected()
	{
		if(!myConnectedDevice)
			return -1;

		int c = 0;
		
		for(auto& it : devices)
		{
			if(myConnectedDevice->Path() == it.first)
				return c;
			c++;
		}

		return -1;
	}

	bool ConnectToUrl(string url)
	{
		if (devices.contains(url)) {
			int c = 0;

			for (auto& it : devices)
			{
				if (it.second.GetPath() == url)
					return c;
				
				return DeviceSelect(c);
			}

			return false;
		}

		auto it = devices.emplace(url, url);
		if (!it.first->second.Probe())
		{
			devices.erase(it.first);
			return false;
		}

		if(!it.first->second.ConnectStage2())
		{
			devices.erase(it.first);
			return false;
		}

		return true;
	}

private:
	map<DevicePath, DeviceConnection> devices;
	
	unique_ptr<PadDevice> myConnectedDevice;
	map<DevicePath, DeviceName> myFailedDevices;
	bool emulator = false;
};

// ====================================================================================================================
// Device API.
// ====================================================================================================================

static ConnectionManager* connectionManager = nullptr;
static bool searching = true;


bool DeviceConnection::ConnectStage2()
{
	if(!connectionManager)
		return false;

	if(state == CS_FAILED)
		return false;

	for(int tries=0; tries<3; ++tries)
	{
		if(connectionManager->ConnectToDeviceStage2(*this))
			return true;

		Log::Writef("DeviceConnection :: ConnectStage2 failed (%d)", tries);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	state = CS_FAILED;
	return false;
}


void Device::Init()
{
	hid_init();
	/*
	hid_read_register([](uint8_t reportId, std::vector<uint8_t> data)
	{
		adp::Log::Write("Data gott");
	});
	*/

	connectionManager = new ConnectionManager();

	// searching = true;
}

#ifdef DEVICE_SERVER_ENABLED
void Device::ServerStart()
{
	auto device = connectionManager->ConnectedDevice();
	if (device) device->ServerStart();
}
#endif

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

		if(changes & DCF_NAME) {
			connectionManager->GetDeviceName(connectionManager->DeviceSelected(), true);
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

std::string Device::ReadDebug()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->ReadDebug() : "";
}

const bool Device::HasUnsavedChanges()
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->HasUnsavedChanges() : false;
}

bool Device::SetThreshold(int sensorIndex, double threshold, double releaseThreshold)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetThreshold(sensorIndex, threshold, releaseThreshold) : false;
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
	if(device) {
		device->CalibrateSensor(sensorIndex);
	}
}

bool Device::SetReleaseMode(ReleaseMode mode)
{
	auto device = connectionManager->ConnectedDevice();
	if(device) {
		return device->SetReleaseMode(mode);
	}
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

int Device::DeviceNumber()
{
	if(!connectionManager) return 0;
	return connectionManager->DeviceNumber();
}

string Device::GetDeviceName(int index)
{
	if(!connectionManager) return "";
	return connectionManager->GetDeviceName(index);
}

bool Device::DeviceSelect(int index)
{
	if(!connectionManager) return false;
	return connectionManager->DeviceSelect(index);
}

int Device::DeviceSelected()
{
	if(!connectionManager) return -1;
	return connectionManager->DeviceSelected();
}

#ifdef DEVICE_CLIENT_ENABLED
bool Device::Connect(std::string url)
{
	return connectionManager->ConnectToUrl(url);
}
#endif


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

			int lastRule = (int)j["ledMappings"].size();
			if(lastRule < MAX_LED_MAPPINGS)
			{
				for(int i = lastRule; i < MAX_LIGHT_RULES; i++) {
					DisableLedMapping(i);
				}
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

			int lastRule = (int)j["lightRules"].size();
			if(lastRule < MAX_LIGHT_RULES)
			{
				for(int i = lastRule; i < MAX_LIGHT_RULES; i++) {
					DisableLightRule(i);
				}
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
				SetThreshold(key, sensor["threshold"], sensor["threshold"]);
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
		if(j.contains("name")) {
			string name = j["name"];
			SetDeviceName( ((std::string)j["name"]).c_str() );
		}
	}
}

void Device::SaveProfile(json& j, DeviceProfileGroups groups)
{
	j["adpToolVersion"] = fmt::format("v{}.{}", ADP_VERSION_MAJOR, ADP_VERSION_MINOR);

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
