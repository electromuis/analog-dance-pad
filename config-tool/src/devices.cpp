#include "devices.h"
#include "logging.h"
#include "hidapi.h"
#include "utils.h"

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <future>
#include <chrono>
#include <map>

using namespace std;
using namespace logging;

namespace devices {

constexpr int HID_VENDOR_ID = 0x1209;
constexpr int HID_PRODUCT_ID = 0xB196;

constexpr int MAX_SENSOR_COUNT = 12;
constexpr int MAX_BUTTON_COUNT = 16;
constexpr int MAX_NAME_LENGTH  = 50;
constexpr int MAX_SENSOR_VALUE = 850;
constexpr int MAX_LIGHT_RULES = 16;

constexpr size_t MAX_REPORT_SIZE = 512;

// ====================================================================================================================
// Report functionality.
// ====================================================================================================================

static_assert(sizeof(float) == sizeof(uint32_t), "32-bit float required");

enum ReportId
{
	REPORT_SENSOR_VALUES      = 0x1,
	REPORT_PAD_CONFIGURATION  = 0x2,
	REPORT_RESET              = 0x3,
	REPORT_SAVE_CONFIGURATION = 0x4,
	REPORT_NAME               = 0x5,
	REPORT_UNUSED_JOYSTICK    = 0x6,
	REPORT_LIGHTS             = 0x7,
};

enum LightRuleFlags
{
	LRF_FADE_ON  = 0x1,
	LRF_FADE_OFF = 0x2,
};

#pragma pack(1)

struct uint16_le { uint8_t bytes[2]; };
struct float32_le { uint8_t bytes[4]; };
struct rgb_color { uint8_t red, green, blue; };

#define DEFINE_REPORT(id) \
	inline static constexpr const wchar_t* NAME = L#id; \
	uint8_t reportId = id;

struct SensorValuesReport
{
	DEFINE_REPORT(REPORT_SENSOR_VALUES);
	uint16_le buttonBits;
	uint16_le sensorValues[MAX_SENSOR_COUNT];
};

struct PadConfigurationReport
{
	DEFINE_REPORT(REPORT_PAD_CONFIGURATION);
	uint16_le sensorThresholds[MAX_SENSOR_COUNT];
	float32_le releaseThreshold;
	int8_t sensorToButtonMapping[MAX_SENSOR_COUNT];
};

struct NameReport
{
	DEFINE_REPORT(REPORT_NAME);
	uint8_t size;
	uint8_t name[MAX_NAME_LENGTH];
};

struct LightRule
{
	int8_t sensorNumber;
	uint8_t fromLight;
	uint8_t toLight;
	rgb_color onColor;
	rgb_color offColor;
	rgb_color onFadeColor;
	rgb_color offFadeColor;
	uint8_t flags;
};

struct LightsReport
{
	DEFINE_REPORT(REPORT_LIGHTS);
	LightRule lightRules[MAX_LIGHT_RULES];
};

#pragma pack()

static bool IsBitSet(int bits, int index)
{
	return (bits & (1 << index)) != 0;
}

static int ReadU16LE(uint16_le u16)
{
	return u16.bytes[0] | u16.bytes[1] << 8;
}

static float ReadF32LE(float32_le f32)
{
	uint32_t u32 = f32.bytes[0] | (f32.bytes[1] << 8) | (f32.bytes[2] << 16) | (f32.bytes[3] << 24);
	return *reinterpret_cast<float*>(&u32);
}

static uint16_le WriteU16LE(int value)
{
	uint16_le u16;
	u16.bytes[0] = value & 0xFF;
	u16.bytes[1] = (value >> 8) & 0xFF;
	return u16;
}

static float32_le WriteF32LE(float value)
{
	float32_le f32;
	uint32_t u32 = *reinterpret_cast<uint32_t*>(&value);
	f32.bytes[0] = u32 & 0xFF;
	f32.bytes[1] = (u32 >> 8) & 0xFF;
	f32.bytes[2] = (u32 >> 16) & 0xFF;
	f32.bytes[3] = (u32 >> 24) & 0xFF;
	return f32;
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

enum class ReadSensorResult { NO_DATA, SUCCESS, FAILURE };

static ReadSensorResult ReadSensorValues(hid_device* hid, SensorValuesReport& report)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	int bytesRead = hid_read(hid, buffer, sizeof(buffer));
	if (bytesRead == sizeof(SensorValuesReport))
	{
		memcpy(&report, buffer, sizeof(SensorValuesReport));
		return ReadSensorResult::SUCCESS;
	}

	if (bytesRead == 0)
		return ReadSensorResult::NO_DATA;

	if (bytesRead < 0)
		Log::Writef(L"ReadSensorValues :: hid_read failed (%s)", hid_error(hid));
	else
		Log::Writef(L"ReadSensorValues :: unexpected number of bytes read (%i)", bytesRead);
	return ReadSensorResult::FAILURE;
}

template <typename T>
static bool GetFeatureReport(hid_device* hid, T& report)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	int bytesRead = hid_get_feature_report(hid, buffer, sizeof(buffer));
	if (bytesRead == sizeof(T) + 1)
	{
		memcpy(&report, buffer, sizeof(T));
		return true;
	}

	if (bytesRead < 0)
		Log::Writef(L"GetFeatureReport (%s) :: hid failed (%s)", T::NAME, hid_error(hid));
	else
		Log::Writef(L"GetFeatureReport (%s) :: unexpected number of bytes read (%i)", T::NAME, bytesRead);
	return false;
}

template <typename T>
static bool SendFeatureReport(hid_device* hid, const T& report)
{
	int bytesWritten = hid_send_feature_report(hid, (const unsigned char*)&report, sizeof(T));
	if (bytesWritten == sizeof(T))
	{
		Log::Writef(L"SendFeatureReport (%s) :: done", T::NAME);
		return true;
	}

	if (bytesWritten < 0)
		Log::Writef(L"SendFeatureReport (%s) :: hid failed (%s)", T::NAME, hid_error(hid));
	else
		Log::Writef(L"SendFeatureReport (%s) :: unexpected number of bytes written (%i)", T::NAME, bytesWritten);
	return false;
}

static bool WriteReport(hid_device* hid, uint8_t reportId, const wchar_t* name)
{
	int bytesWritten = hid_write(hid, &reportId, sizeof(reportId));
	if (bytesWritten > 0)
	{
		Log::Writef(L"WriteReport :: done");
		return true;
	}
	Log::Writef(L"WriteReport (%s) :: hid failed (%s)", name, hid_error(hid));
	return false;
}

static void SendDeviceReset(hid_device* hid)
{
	uint8_t reportId = REPORT_RESET;
	hid_write(hid, &reportId, sizeof(reportId)); // Device resets immediately, so checking result is not reliable.
	Log::Write(L"SendDeviceReset :: done");
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

// ====================================================================================================================
// Pad device.
// ====================================================================================================================

typedef string DevicePath;
typedef wstring DeviceName;

class PadDevice
{
public:
	PadDevice(
		hid_device* hid,
		const char* path,
		const NameReport& name,
		const PadConfigurationReport& config,
		const LightsReport* lights)
		: myHid(hid)
		, myPath(path)
	{
		UpdateName(name);
		myState.maxNameLength = MAX_NAME_LENGTH;
		myState.numButtons = MAX_BUTTON_COUNT;
		myState.numSensors = MAX_SENSOR_COUNT;
		UpdatePadConfiguration(config);
	}

	~PadDevice() { hid_close(myHid); }

	void UpdateName(const NameReport& report)
	{
		myState.name = utils::widen((const char*)report.name, (size_t)report.size);
		myChanges |= DCF_NAME;
	}

	void UpdatePadConfiguration(const PadConfigurationReport& report)
	{
		for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
		{
			auto buttonMapping = (unsigned int)report.sensorToButtonMapping[i];
			mySensors[i].threshold = ToNormalizedSensorValue(ReadU16LE(report.sensorThresholds[i]));
			mySensors[i].button = (buttonMapping >= MAX_BUTTON_COUNT ? 0 : (buttonMapping + 1));
		}
		myReleaseThreshold = ReadF32LE(report.releaseThreshold);
	}

	bool UpdateSensorValues()
	{
		SensorValuesReport report;

		int aggregateValues[MAX_SENSOR_COUNT] = {};
		int pressedButtons = 0;
		int inputsRead = 0;

		for (int readsLeft = 100; readsLeft > 0; readsLeft--)
		{
			switch (ReadSensorValues(myHid, report))
			{
			case ReadSensorResult::SUCCESS:
				pressedButtons |= ReadU16LE(report.buttonBits);
				for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
					aggregateValues[i] += ReadU16LE(report.sensorValues[i]);
				++inputsRead;
				break;

			case ReadSensorResult::NO_DATA:
				readsLeft = 0;
				break;

			case ReadSensorResult::FAILURE:
				return false;
			}
		}

		if (inputsRead > 0)
		{
			for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
			{
				auto button = mySensors[i].button;
				auto value = (double)aggregateValues[i] / (double)inputsRead;
				mySensors[i].pressed = button > 0 && IsBitSet(pressedButtons, button - 1);
				mySensors[i].value = ToNormalizedSensorValue(value);
			}
		}

		return true;
	}

	bool SetThreshold(int sensorIndex, double threshold)
	{
		mySensors[sensorIndex].threshold = clamp(threshold, 0.0, 1.0);
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

		auto name = utils::narrow(rawName, wcslen(rawName));
		if (name.size() > sizeof(report.name))
		{
			Log::Writef(L"SetName :: name '%s' exceeds %i chars and was not set", rawName, MAX_NAME_LENGTH);
			return false;
		}

		report.size = (uint8_t)name.length();
		memcpy(report.name, name.data(), name.length());
		bool result = SendFeatureReport(myHid, report) && GetFeatureReport(myHid, report);
		myHasUnsavedChanges = true;
		UpdateName(report);
		return result;
	}

	void Reset() { SendDeviceReset(myHid); }

	bool SendPadConfiguration()
	{
		PadConfigurationReport report;
		for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
		{
			report.sensorThresholds[i] = WriteU16LE(ToDeviceSensorValue(mySensors[i].threshold));
			report.sensorToButtonMapping[i] = (mySensors[i].button == 0) ? 0xFF : (mySensors[i].button - 1);
		}
		report.releaseThreshold = WriteF32LE((float)myReleaseThreshold);
		bool result = SendFeatureReport(myHid, report) && GetFeatureReport(myHid, report);
		myHasUnsavedChanges = true;
		UpdatePadConfiguration(report);
		return result;
	}

	void SaveChanges()
	{
		if (myHasUnsavedChanges)
		{
			WriteReport(myHid, REPORT_SAVE_CONFIGURATION, L"REPORT_SAVE_CONFIGURATION");
			myHasUnsavedChanges = false;
		}
	}

	const DevicePath& Path() const { return myPath; }

	const PadState& State() const { return myState; }

	const SensorState* Sensor(int index)
	{
		return (index >= 0 && index < MAX_SENSOR_COUNT) ? (mySensors + index) : nullptr;
	}

	DeviceChanges PopChanges()
	{
		auto result = myChanges;
		myChanges = 0;
		return result;
	}

private:
	hid_device* myHid;
	DevicePath myPath;
	PadState myState;
	SensorState mySensors[MAX_SENSOR_COUNT];
	DeviceChanges myChanges = 0;
	bool myHasUnsavedChanges = false;
	double myReleaseThreshold = 1.0;
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
		auto foundDevices = hid_enumerate(HID_VENDOR_ID, HID_PRODUCT_ID);

		// Devices that are incompatible or had a communication failure are tracked in a failed device list to prevent
		// a loop of reconnection attempts. Remove unplugged devices from the list. Then, the user can attempt to
		// reconnect by plugging it back in, as it will be seen as a new device.

		for (auto it = myFailedDevices.begin(); it != myFailedDevices.end();)
		{
			if (!ContainsDevice(foundDevices, it->first))
			{
				Log::Writef(L"ConnectionManager :: failed device removed (%s)", it->second.data());
				it = myFailedDevices.erase(it);
			}
			else ++it;
		}

		// Try to connect to the first compatible device that is not on the failed device list.

		for (auto device = foundDevices; device; device = device->next)
		{
			if (myFailedDevices.count(device->path) == 0 && ConnectToDevice(device))
				break;
		}

		hid_free_enumeration(foundDevices);
		return (bool)myConnectedDevice;
	}

	bool ConnectToDevice(hid_device_info* deviceInfo)
	{
		// Open and configure HID for communicating with the pad.

		auto hid = hid_open_path(deviceInfo->path);
		if (!hid)
		{
			Log::Writef(L"ConnectionManager :: hid_open failed (%s)", hid_error(nullptr));
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

		NameReport name;
		PadConfigurationReport padConfiguration;
		if (!GetFeatureReport(hid, name) || !GetFeatureReport(hid, padConfiguration))
		{
			AddIncompatibleDevice(deviceInfo);
			hid_close(hid);
			return false;
		}

		// If the device supports light configuration, read lights.

		//LightsReport lights;
		//bool supportsLight = GetFeatureReport(hid, lights);

		auto device = new PadDevice(hid, deviceInfo->path, name, padConfiguration, nullptr);

		Log::Write(L"ConnectionManager :: new device connected [");
		Log::Writef(L"  Name: %s", device->State().name.data());
		Log::Writef(L"  Product: %s", deviceInfo->product_string);
		Log::Writef(L"  Manufacturer: %s", deviceInfo->manufacturer_string);
		Log::Writef(L"  Path: %s", utils::widen(deviceInfo->path, strlen(deviceInfo->path)).data());
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
		myFailedDevices[device->path] = device->product_string;
	}

private:
	unique_ptr<PadDevice> myConnectedDevice;
	map<DevicePath, DeviceName> myFailedDevices;
};

// ====================================================================================================================
// Device manager API.
// ====================================================================================================================

static ConnectionManager* connectionManager = nullptr;

void DeviceManager::Init()
{
	hid_init();

	connectionManager = new ConnectionManager();
}

void DeviceManager::Shutdown()
{
	delete connectionManager;
	connectionManager = nullptr;

	hid_exit();
}

DeviceChanges DeviceManager::Update()
{
	DeviceChanges changes = 0;

	// If there is currently no connected device, try to find one.
	auto device = connectionManager->ConnectedDevice();
	if (!device)
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

const PadState* DeviceManager::Pad()
{
	// static PadState dummy;
	// dummy.numSensors = 8;
	// dummy.numButtons = 4;
	// return &dummy;
	auto device = connectionManager->ConnectedDevice();
	return device ? &device->State() : nullptr;
}

const SensorState* DeviceManager::Sensor(int sensorIndex)
{
	// static SensorState dummies[MAX_SENSOR_COUNT];
	// return dummies + index;
	auto device = connectionManager->ConnectedDevice();
	return device ? device->Sensor(sensorIndex) : nullptr;
}

bool DeviceManager::SetThreshold(int sensorIndex, double threshold)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetThreshold(sensorIndex, threshold) : false;
}

bool DeviceManager::SetButtonMapping(int sensorIndex, int button)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SetButtonMapping(sensorIndex, button) : false;
}

bool DeviceManager::SetDeviceName(const wchar_t* name)
{
	auto device = connectionManager->ConnectedDevice();
	return device ? device->SendName(name) : false;
}

void DeviceManager::SendDeviceReset()
{
	auto device = connectionManager->ConnectedDevice();
	if (device) device->Reset();
}

}; // namespace devices.