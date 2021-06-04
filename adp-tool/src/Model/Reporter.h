#pragma once

#include "stdint.h"
#include "hidapi.h"

// Potentially defined by WinSock2.h
#ifdef NO_DATA
#undef NO_DATA
#endif

namespace adp {

#pragma pack(1)

constexpr int MAX_SENSOR_COUNT = 12;
constexpr int MAX_BUTTON_COUNT = 16;
constexpr int MAX_NAME_LENGTH  = 50;
constexpr int MAX_SENSOR_VALUE = 850;
constexpr int MAX_LIGHT_RULES  = 16;
constexpr int BOARD_TYPE_LENGTH = 32;

constexpr size_t MAX_REPORT_SIZE = 512;

enum ReportId
{
	REPORT_SENSOR_VALUES      = 0x1,
	REPORT_PAD_CONFIGURATION  = 0x2,
	REPORT_RESET              = 0x3,
	REPORT_SAVE_CONFIGURATION = 0x4,
	REPORT_NAME               = 0x5,
	REPORT_UNUSED_JOYSTICK    = 0x6,
	REPORT_LIGHTS             = 0x7,
	REPORT_FACTORY_RESET	  = 0x8,
	REPORT_IDENTIFICATION	  = 0x9
};

enum class ReadDataResult
{
	NO_DATA,
	SUCCESS,
	FAILURE,
};

struct uint16_le { uint8_t bytes[2]; };
struct float32_le { uint8_t bytes[4]; };
struct color24 { uint8_t red, green, blue; };

struct SensorValuesReport
{
	uint8_t reportId = REPORT_SENSOR_VALUES;
	uint16_le buttonBits;
	uint16_le sensorValues[MAX_SENSOR_COUNT];
};

struct PadConfigurationReport
{
	uint8_t reportId = REPORT_PAD_CONFIGURATION;
	uint16_le sensorThresholds[MAX_SENSOR_COUNT];
	float32_le releaseThreshold;
	int8_t sensorToButtonMapping[MAX_SENSOR_COUNT];
};

struct NameReport
{
	uint8_t reportId = REPORT_NAME;
	uint8_t size;
	uint8_t name[MAX_NAME_LENGTH];
};

struct IdentificationReport
{
	uint8_t reportId = REPORT_IDENTIFICATION;
	uint16_le firmwareMajor;
	uint16_le firmwareMinor;
	uint8_t buttonCount;
	uint8_t sensorCount;
	uint8_t ledCount;
	uint16_le maxSensorValue;
	char boardType[BOARD_TYPE_LENGTH];
};

#pragma pack()

class Reporter
{
public:
	Reporter(hid_device* device);
	~Reporter();

	ReadDataResult Get(SensorValuesReport& report);
	bool Get(PadConfigurationReport& report);
	bool Get(NameReport& report);
	bool Get(IdentificationReport& report);

	void SendReset();
	void SendFactoryReset();
	bool SendSaveConfiguration();
	bool Send(const PadConfigurationReport& report);
	bool Send(const NameReport& report);

private:
	hid_device* myHid;
};

}; // namespace adp.