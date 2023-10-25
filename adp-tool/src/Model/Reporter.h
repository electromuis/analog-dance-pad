#pragma once

#include "stdint.h"
#include "hidapi.h"
#include <memory>
#include <string>

// Potentially defined by WinSock2.h
#ifdef NO_DATA
#undef NO_DATA
#endif

namespace adp {

#pragma pack(1)

constexpr int MAX_SENSOR_COUNT  = 12;
constexpr int MAX_BUTTON_COUNT  = 16;
constexpr int MAX_NAME_LENGTH   = 50;
constexpr int MAX_SENSOR_VALUE  = 850;
constexpr int MAX_LIGHT_RULES   = 16;
constexpr int MAX_LED_MAPPINGS  = 16;
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
	REPORT_LIGHT_RULE         = 0x7,
	REPORT_FACTORY_RESET	  = 0x8,
	REPORT_IDENTIFICATION	  = 0x9,
	REPORT_LED_MAPPING        = 0xA,
	REPORT_SET_PROPERTY       = 0xB,
	REPORT_SENSOR			  = 0xC,
	REPORT_DEBUG			  = 0xD,
	REPORT_IDENTIFICATION_V2  = 0xE,
};

enum class ReadDataResult
{
	NO_DATA,
	SUCCESS,
	FAILURE,
};

struct uint16_le { uint8_t bytes[2]; };
struct uint32_le { uint8_t bytes[4]; };
struct color24 { uint8_t red, green, blue; };

struct float32_le { uint32_le bits; };

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

struct IdentificationV2Report : public IdentificationReport
{
	IdentificationV2Report()
	{
		reportId = REPORT_IDENTIFICATION_V2;
	}

	enum Features
	{
		FEATURE_DEBUG = 1 << 0,
		FEATURE_DIGIPOT = 1 << 1,
		FEATURE_LIGHTS = 1 << 2,
	};

	uint16_le features;
};

struct LightRuleReport
{
	uint8_t reportId = REPORT_LIGHT_RULE;
	uint8_t lightRuleIndex;
	uint8_t flags;
	color24 onColor;
	color24 offColor;
	color24 onFadeColor;
	color24 offFadeColor;
};

struct LedMappingReport
{
	uint8_t reportId = REPORT_LED_MAPPING;
	uint8_t ledMappingIndex;
	uint8_t flags;
	uint8_t lightRuleIndex;
	uint8_t sensorIndex;
	uint8_t ledIndexBegin;
	uint8_t ledIndexEnd;
};

struct SensorReport
{
	enum Ids
	{
		ADC_DISABLED		= 1 << 0,
	};

	uint8_t reportId = REPORT_SENSOR;
	uint8_t index;
	uint16_le threshold;
	uint16_le releaseThreshold;
	int8_t buttonMapping;
	uint8_t resistorValue;
	uint16_le flags;
};

struct SetPropertyReport
{
	enum Ids
	{
		SELECTED_LIGHT_RULE_INDEX = 0,
		SELECTED_LED_MAPPING_INDEX = 1,
		SELECTED_SENSOR_INDEX = 2
	};
	uint8_t reportId = REPORT_SET_PROPERTY;
	uint32_le propertyId;
	uint32_le propertyValue;
};

struct DebugReport
{
	uint8_t reportId = REPORT_DEBUG;
	uint16_le messageSize;
	char messagePacket[32];
};

#pragma pack()

class DeviceServer;

class ReporterBackend
{
public:
	virtual int get_feature_report(unsigned char *data, size_t length) = 0;
	virtual int send_feature_report(const unsigned char *data, size_t length) = 0;
	virtual int read(unsigned char *data, size_t length) = 0;
	virtual int write(unsigned char *data, size_t length) = 0;
	virtual const wchar_t* error() = 0;
};

class Reporter
{
public:
	Reporter(hid_device* device);
	Reporter(std::string host, std::string port);
	Reporter();
	~Reporter();

	ReadDataResult Get(SensorValuesReport& report);
	bool Get(PadConfigurationReport& report);
	bool Get(NameReport& report);
	bool Get(IdentificationReport& report);
	bool Get(IdentificationV2Report& report);
	bool Get(LightRuleReport& report);
	bool Get(LedMappingReport& report);
	bool Get(SensorReport& report);
	bool Get(DebugReport& report);

	void SendReset();
	void SendFactoryReset();
	bool SendSaveConfiguration();
	bool Send(const PadConfigurationReport& report);
	bool Send(const NameReport& report);
	bool Send(const LightRuleReport& report);
	bool Send(const LedMappingReport& report);
	bool Send(const SensorReport& report);
	bool Send(const SetPropertyReport& report);


	bool SendAndGet(NameReport& report);
	bool SendAndGet(PadConfigurationReport& report);

private:
	// hid_device* myHid;
	bool emulator = false;
	std::unique_ptr<ReporterBackend> backend;
	std::unique_ptr<DeviceServer> deviceServer;


	template <typename T>
	bool GetFeatureReport(T& report, const char* name);

	template <typename T>
	bool SendFeatureReport(const T& report, const char* name);

	template <typename T>
	ReadDataResult ReadData(T& report, const char* name);

	bool WriteData(uint8_t reportId, const char* name, bool performErrorCheck);


};

}; // namespace adp.
