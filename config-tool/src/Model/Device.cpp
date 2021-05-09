#include "device.h"
#include "reporter.h"
#include "log.h"
#include "hidapi.h"
#include "utils.h"

#include <memory>
#include <algorithm>
#include <map>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

namespace adp {

struct HidIdentifier
{
	int vendorId;
	int productId;
};

constexpr HidIdentifier HID_IDS[] = { {0x1209, 0xb196}, {0x03eb, 0x204f} };

static_assert(sizeof(float) == sizeof(uint32_t), "32-bit float required");

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
		const PadConfigurationReport& config)
		: myReporter(move(reporter))
		, myPath(path)
	{
		UpdateName(name);
		myPad.maxNameLength = MAX_NAME_LENGTH;
		myPad.numButtons = MAX_BUTTON_COUNT;
		myPad.numSensors = MAX_SENSOR_COUNT;
		UpdatePadConfiguration(config);
		myPollingData.lastUpdate = system_clock::now();
	}

	void UpdateName(const NameReport& report)
	{
		myPad.name = widen((const char*)report.name, (size_t)report.size);
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
		myPad.releaseThreshold = ReadF32LE(report.releaseThreshold);
	}

	bool UpdateSensorValues()
	{
		SensorValuesReport report;

		int aggregateValues[MAX_SENSOR_COUNT] = {};
		int pressedButtons = 0;
		int inputsRead = 0;

		for (int readsLeft = 100; readsLeft > 0; readsLeft--)
		{
			switch (myReporter->Get(report))
			{
			case ReadDataResult::SUCCESS:
				pressedButtons |= ReadU16LE(report.buttonBits);
				for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
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
			for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
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
			Log::Writef(L"SetName :: name '%s' exceeds %i chars and was not set", rawName, MAX_NAME_LENGTH);
			return false;
		}

		report.size = (uint8_t)name.length();
		memcpy(report.name, name.data(), name.length());
		bool result = myReporter->Send(report) && myReporter->Get(report);
		myHasUnsavedChanges = true;
		UpdateName(report);
		return result;
	}

	void Reset() { myReporter->SendReset(); }

	void FactoryReset()
	{
		// Have the device load up and save its defaults
		myReporter->SendFactoryReset();
		
		// Wait for the controller to process the report
		this_thread::sleep_for(2ms);

		// Fetch fresh config from device, and load it into our memory
		NameReport name;
		PadConfigurationReport padConfiguration;

		if (!myReporter->Get(name) || !myReporter->Get(padConfiguration)) {
			Log::Writef(L"FactoryReset :: failed fetching configuration after sending FactoryReset");
			return;
		}

		
		UpdateName(name);
		UpdatePadConfiguration(padConfiguration);

		PrintPadConfigurationReport(padConfiguration);

		myChanges |= DCF_DEVICE | DCF_BUTTON_MAPPING | DCF_NAME;
	}

	bool SendPadConfiguration()
	{
		PadConfigurationReport report;
		for (int i = 0; i < MAX_SENSOR_COUNT; ++i)
		{
			report.sensorThresholds[i] = WriteU16LE(ToDeviceSensorValue(mySensors[i].threshold));
			report.sensorToButtonMapping[i] = (mySensors[i].button == 0) ? 0xFF : (mySensors[i].button - 1);
		}
		report.releaseThreshold = WriteF32LE((float)myPad.releaseThreshold);
		
		bool sendResult = myReporter->Send(report);
		
		// Wait for the controller to process the report
		this_thread::sleep_for(2ms);

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
	unique_ptr<Reporter> myReporter;
	DevicePath myPath;
	PadState myPad;
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
		auto foundDevices = hid_enumerate(0, 0);

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

		auto reporter = make_unique<Reporter>(hid);
		NameReport name;
		PadConfigurationReport padConfiguration;
		if (!reporter->Get(name) || !reporter->Get(padConfiguration))
		{
			AddIncompatibleDevice(deviceInfo);
			hid_close(hid);
			return false;
		}

		// If the device supports light configuration, read lights.

		//LightsReport lights;
		//bool supportsLight = GetFeatureReport(hid, lights);

		auto device = new PadDevice(reporter, deviceInfo->path, name, padConfiguration);

		Log::Write(L"ConnectionManager :: new device connected [");
		Log::Writef(L"  Name: %s", device->State().name.data());
		Log::Writef(L"  Product: %s", deviceInfo->product_string);
		Log::Writef(L"  Manufacturer: %s", deviceInfo->manufacturer_string);
		Log::Writef(L"  Path: %s", widen(deviceInfo->path, strlen(deviceInfo->path)).data());
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
};

// ====================================================================================================================
// Device API.
// ====================================================================================================================

static ConnectionManager* connectionManager = nullptr;

void Device::Init()
{
	hid_init();

	connectionManager = new ConnectionManager();
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

}; // namespace adp.