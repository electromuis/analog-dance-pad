#include "devices.h"
#include "hidapi.h"

#include <vector>
#include <string>
#include <iostream>
#include <memory>

using namespace std;

// Constants and data definitions.

static const int SENSOR_COUNT = 12;
static const int BUTTON_COUNT = 16;

static const int VENDOR_ID  = 0x1209;
static const int PRODUCT_ID = 0xB196;

static const int MAX_NAME_SIZE = 50;
static const int MAX_SENSOR_VALUE = 850;

static_assert(sizeof(float) == sizeof(uint32_t), "32-bit float required");

enum ReportID
{
	SENSOR_VALUES      = 0x01,
	PAD_CONFIGURATION  = 0x02,
	RESET              = 0x03,
	SAVE_CONFIGURATION = 0x04,
	NAME               = 0x05,
};

#pragma pack(1)

struct SensorValuesReport
{
	uint8_t reportId = SENSOR_VALUES;
	uint16_t buttonBits;
	uint16_t sensorValues[SENSOR_COUNT];
};

struct PadConfigurationReport
{
	uint8_t reportId = PAD_CONFIGURATION;
	uint16_t sensorThresholds[SENSOR_COUNT];
	uint32_t releaseThreshold;
	uint8_t sensorToButtonMapping[SENSOR_COUNT];
};

struct NameReport
{
	uint8_t reportId = NAME;
	uint8_t size;
	uint8_t name[MAX_NAME_SIZE];
};

#pragma pack()

// Helpers for reading and writing little-endian data.

static int Read16LE(uint16_t in)
{
	auto bytes = (const uint8_t*)&in;
	return bytes[0] | bytes[1] << 8;
}

static float ReadFloatLE(uint32_t in)
{
	auto bytes = (const uint8_t*)&in;
	uint32_t u32 = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
	return *reinterpret_cast<float*>(&u32);
}

static void Write16LE(uint16_t* out, int value)
{
	auto bytes = (uint8_t*)&out;
	bytes[0] = value & 0xFF;
	bytes[1] = (value >> 8) & 0xFF;
}

static void WriteFloatLE(uint32_t* out, float value)
{
	auto bytes = (uint8_t*)&out;
	uint32_t u32 = *reinterpret_cast<uint32_t*>(&value);
	bytes[0] = u32 & 0xFF;
	bytes[1] = (u32 >> 8) & 0xFF;
	bytes[2] = (u32 >> 16) & 0xFF;
	bytes[3] = (u32 >> 24) & 0xFF;
}

// Pad device implementation.

class PadDeviceImpl : public PadDevice
{
public:
	PadDeviceImpl(hid_device_info* device);
	~PadDeviceImpl();

	const char* name() const override { return myName.data(); }

	int numSensors() const override { return SENSOR_COUNT; }

	const SensorState& sensor(int index) const override { return mySensors[index]; }

	double releaseThreshold() const override { return myReleaseThreshold; }

	bool isValid() const { return myIsValid; }

	PadDeviceImpl(const PadDeviceImpl&) = delete;
	PadDeviceImpl& operator = (const PadDeviceImpl&) = delete;

	bool readSensorValues();

private:
	bool readPadConfiguration();
	bool readName();

	bool myIsValid = false;
	hid_device* myDevice = nullptr;
	string myName;
	SensorState mySensors[SENSOR_COUNT];
	float myReleaseThreshold = 1.0;
};

PadDeviceImpl::PadDeviceImpl(hid_device_info* deviceInfo)
{
	myDevice = hid_open_path(deviceInfo->path);
	if (!myDevice)
	{
		wprintf(L"hid_open failed: %s\n", hid_error(nullptr));
		return;
	}

	wprintf(L"Product String: %s\n", deviceInfo->product_string);
	wprintf(L"Manufacturer String: %s\n", deviceInfo->manufacturer_string);

	hid_set_nonblocking(myDevice, 1);

	myIsValid = readPadConfiguration() && readName();
}

PadDeviceImpl::~PadDeviceImpl()
{
	if (myDevice)
		hid_close(myDevice);
}

bool PadDeviceImpl::readSensorValues()
{
	const size_t maxDataSize = 1024;
	unsigned char data[1024];

	int bytesRead = hid_read(myDevice, data, maxDataSize);
	if (bytesRead == 0)
		return false;

	if (bytesRead != sizeof(SensorValuesReport))
	{
		wprintf(L"readSensorValues :: unexpected hid_read (bytesRead: %i)\n", bytesRead);
		return false;
	}

	auto report = reinterpret_cast<SensorValuesReport*>(data);
	for (int i = 0; i < SENSOR_COUNT; ++i)
	{
		auto& sensor = mySensors[i];
		if (sensor.button < BUTTON_COUNT)
		{
			sensor.isPressed = (report->buttonBits & (1 << sensor.button)) != 0;
		}
		else
		{
			sensor.isPressed = false;
		}
		sensor.value = report->sensorValues[i];
	}
	return true;
}

bool PadDeviceImpl::readPadConfiguration()
{
	PadConfigurationReport report;
	int bytesRead = hid_get_feature_report(myDevice, (unsigned char*)&report, sizeof(PadConfigurationReport));
	if (bytesRead != sizeof(PadConfigurationReport) + 1)
	{
		wprintf(L"readPadConfiguration :: hid_get_feature_report failed (hid_error %s)\n", hid_error(nullptr));
		return false;
	}
	for (int i = 0; i < SENSOR_COUNT; ++i)
	{
		auto& sensor = mySensors[i];
		sensor.threshold = Read16LE(report.sensorThresholds[i]);
		sensor.button = report.sensorToButtonMapping[i];
		sensor.isPressed = false;
		sensor.isMapped = (sensor.button != 0xFF);
		sensor.value = 0;
	}
	myReleaseThreshold = ReadFloatLE(report.releaseThreshold);
	return true;
}

bool PadDeviceImpl::readName()
{
	NameReport name;
	int bytesRead = hid_get_feature_report(myDevice, (unsigned char*)&name, sizeof(NameReport));
	if (bytesRead != sizeof(NameReport) + 1)
	{
		wprintf(L"readName :: hid_get_feature_report failed (hid_error %s)\n", hid_error(nullptr));
		myName = "Unidentified pad";
		return false;
	}
	myName = string((const char*)name.name, name.size);
	return true;
}

// Device manager API.

struct DeviceManagerData
{
	vector<shared_ptr<PadDeviceImpl>> devices;
};

static DeviceManagerData* deviceManager = nullptr;

static void ConnectToDevices()
{
	auto deviceInfo = hid_enumerate(VENDOR_ID, PRODUCT_ID);
	for (auto di = deviceInfo; di; di = di->next)
	{
		auto device = make_shared<PadDeviceImpl>(di);
		if (device->isValid())
			deviceManager->devices.push_back(device);
	}
	hid_free_enumeration(deviceInfo);
}

void DeviceManager::init()
{
	deviceManager = new DeviceManagerData();

	hid_init();

	ConnectToDevices();
}

bool DeviceManager::readSensorValues()
{
	bool anyRead = false;
	for (auto& device : deviceManager->devices)
		anyRead |= device->readSensorValues();

	return anyRead;
}

PadDevice* DeviceManager::getDevice()
{
	return deviceManager->devices.size() == 1
		? deviceManager->devices[0].get()
		: nullptr;
}

void DeviceManager::shutdown()
{
	hid_exit();

	delete deviceManager;
	deviceManager = nullptr;
}