#include <Adp.h>

#include <cstring>
#include <chrono>
#include <thread>

#include <Model/Reporter.h>
#include <Model/Log.h>
#include <Model/Utils.h>

using namespace std;

namespace adp {

// ====================================================================================================================
// Helper functions.
// ====================================================================================================================

template <typename T>
static bool GetFeatureReport(hid_device* hid, T& report, const char* name)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	auto size = sizeof(T);
	auto expectedSize = sizeof(T);

	int bytesRead = hid_get_feature_report(hid, buffer, sizeof(buffer));
	if (bytesRead == expectedSize)
	{
		memcpy(&report, buffer, size);
		Log::Writef("%s :: done", name);
		return true;
	}

	if (bytesRead < 0)
		Log::Writef("%s :: hid_get_feature_report failed (%ls)", name, hid_error(hid));
	else
		Log::Writef("%s :: unexpected number of bytes read (%i) expected (%i)", name, bytesRead, expectedSize);
	return false;
}

template <typename T>
static bool SendFeatureReport(hid_device* hid, const T& report, const char* name)
{
	using namespace std::chrono_literals;

	// Wait for the controller to get into a ready state
	std::this_thread::sleep_for(2ms);

	int bytesWritten = hid_send_feature_report(hid, (const unsigned char*)&report, sizeof(T));
	if (bytesWritten == sizeof(T))
	{
		Log::Writef("%s :: done", name);
		return true;
	}

	if (bytesWritten < 0)
		Log::Writef("%s :: hid_send_feature_report failed (%ls)", name, hid_error(hid));
	else
		Log::Writef("%s :: unexpected number of bytes written (%i)", name, bytesWritten);
	return false;
}

template <typename T>
static ReadDataResult ReadData(hid_device* hid, T& report, const char* name)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	int bytesRead = hid_read(hid, buffer, sizeof(buffer));
	auto expectedSize = sizeof(T);

	if (bytesRead == expectedSize)
	{
		memcpy(&report, buffer, expectedSize);
		return ReadDataResult::SUCCESS;
	}

	if (bytesRead == 0)
		return ReadDataResult::NO_DATA;

	if (bytesRead < 0)
		Log::Writef("%s :: hid_read failed (%ls)", name, hid_error(hid));
	else
		Log::Writef("%s :: unexpected number of bytes read (%i) expected (%i)", name, bytesRead, expectedSize);

	return ReadDataResult::FAILURE;
}

static bool WriteData(hid_device* hid, uint8_t reportId, const char* name, bool performErrorCheck)
{
	// Linux wants reports of at leats 2 bytes
	uint8_t buf[2] = { reportId, 0 };

	int bytesWritten = hid_write(hid, buf, sizeof(buf));
	if (bytesWritten > 0 || !performErrorCheck)
	{
		Log::Writef("%s :: done", name);
		return true;
	}
	Log::Writef("%s :: hid_write failed (%ls)", name, hid_error(hid));
	return false;
}

// ====================================================================================================================
// Reporter.
// ====================================================================================================================

Reporter::Reporter(hid_device* device)
	: myHid(device)
{
}

Reporter::Reporter()
{
	emulator = false;
}

Reporter::~Reporter()
{
	if(!emulator) {
		hid_close(myHid);
	}
}

ReadDataResult Reporter::Get(SensorValuesReport& report)
{
	if(emulator) {
		return ReadDataResult::NO_DATA;
	}
	
	return ReadData(myHid, report, "GetSensorValuesReport");
}

bool Reporter::Get(PadConfigurationReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(myHid, report, "GetPadConfigurationReport");
}

bool Reporter::Get(NameReport& report)
{
	if(emulator) {
		const char* name = "ADP Emulator";
		memcpy(&report.name, name, sizeof(name));
		report.size = sizeof(name);
		
		return true;
	}

	return GetFeatureReport(myHid, report, "GetNameReport");
}

bool Reporter::Get(IdentificationReport& report)
{
	if(emulator) {
		report.buttonCount = 12;
		report.sensorCount = 12;
		report.ledCount = 0;
		
		return true;
	}

	return GetFeatureReport(myHid, report, "GetIdentificationReport");
}

bool Reporter::Get(IdentificationV2Report& report)
{
	if (emulator) {
		report.buttonCount = 12;
		report.sensorCount = 12;
		report.ledCount = 0;

		return true;
	}

	return GetFeatureReport(myHid, report, "GetIdentificationV2Report");
}

bool Reporter::Get(LightRuleReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(myHid, report, "GetLightRuleReport");
}

bool Reporter::Get(LedMappingReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(myHid, report, "GetLedMappingReport");
}

bool Reporter::Get(SensorReport& report)
{
	if (emulator) {
		return true;
	}

	return GetFeatureReport(myHid, report, "GetSensorReport");
}


bool Reporter::Get(DebugReport& report)
{
	if (emulator) {
		return true;
	}

	return GetFeatureReport(myHid, report, "GetDebugReport");
}

void Reporter::SendReset()
{
	WriteData(myHid, REPORT_RESET, "SendResetReport", false);
}

void Reporter::SendFactoryReset()
{
	WriteData(myHid, REPORT_FACTORY_RESET, "SendFactoryResetReport", false);
}

bool Reporter::SendSaveConfiguration()
{
	if(emulator) {
		return true;
	}

	return WriteData(myHid, REPORT_SAVE_CONFIGURATION, "SendSaveConfigurationReport", true);
}

bool Reporter::Send(const PadConfigurationReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(myHid, report, "SendPadConfigurationReport");
}

bool Reporter::Send(const NameReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(myHid, report, "SendNameReport");
}

bool Reporter::Send(const LightRuleReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(myHid, report, "SendLightRuleReport");
}

bool Reporter::Send(const LedMappingReport& report)
{
	if(emulator) {
		return true;
	}
	
	return SendFeatureReport(myHid, report, "SendLedMappingReport");
}

bool Reporter::Send(const SensorReport& report)
{
	return SendFeatureReport(myHid, report, "SendSensorReport");
}

bool Reporter::Send(const SetPropertyReport& report)
{
	if(emulator) {
		return true;
	}
	
	return SendFeatureReport(myHid, report, "SendSetPropertyReport");
}

bool Reporter::SendAndGet(NameReport& report)
{
	if(!Send(report))
		return false;

	// Wait for the controller to get into a ready state
	std::this_thread::sleep_for(2ms);

	if (!Get(report))
		return false;

	return true;
}

bool Reporter::SendAndGet(PadConfigurationReport& report)
{
	if (!Send(report))
		return false;

	// Wait for the controller to get into a ready state
	std::this_thread::sleep_for(2ms);

	if (!Get(report))
		return false;

	return true;
}

}; // namespace adp.
