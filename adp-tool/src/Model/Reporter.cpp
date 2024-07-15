#include <Adp.h>

#include <cstring>
#include <chrono>
#include <thread>

#include <Model/Reporter.h>
#include <Model/Log.h>
#include <Model/Utils.h>
#include <Model/DeviceServer.h>
#include <Model/DeviceClient.h>

using namespace std;

namespace adp {

// ====================================================================================================================
// Helper functions.
// ====================================================================================================================

template <typename T>
bool Reporter::GetFeatureReport(T& report, const char* name)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	auto size = sizeof(T);
	
	#ifdef __MINGW32__
	auto expectedSize = sizeof(T)+1;
	#else
	auto expectedSize = sizeof(T);
	#endif

	int bytesRead = backend->get_feature_report(buffer, sizeof(buffer));
	if (bytesRead == expectedSize)
	{
		memcpy(&report, buffer, size);
		Log::Writef("%s :: done", name);
		return true;
	}

	if (bytesRead < 0)
		Log::Writef("%s :: hid_get_feature_report failed (%ls)", name, backend->error());
	else
		Log::Writef("%s :: unexpected number of bytes read (%i) expected (%i)", name, bytesRead, expectedSize);
	return false;
}

template <typename T>
bool Reporter::SendFeatureReport(const T& report, const char* name)
{
	using namespace std::chrono_literals;

	// Wait for the controller to get into a ready state
	std::this_thread::sleep_for(2ms);

	int bytesWritten = backend->send_feature_report((const unsigned char*)&report, sizeof(T));
	if (bytesWritten == sizeof(T))
	{
		Log::Writef("%s :: done", name);
		return true;
	}

	if (bytesWritten < 0)
		Log::Writef("%s :: hid_send_feature_report failed (%ls)", name, backend->error());
	else
		Log::Writef("%s :: unexpected number of bytes written (%i) expected (%i)", name, bytesWritten, sizeof(T));
	return false;
}

template <typename T>
ReadDataResult Reporter::ReadData(T& report, const char* name, int expectedSize)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	int bytesRead = backend->read(buffer, sizeof(buffer));

	if (bytesRead == expectedSize)
	{
		memcpy(&report, buffer, expectedSize);
		return ReadDataResult::SUCCESS;
	}

	if (bytesRead == 0)
		return ReadDataResult::NO_DATA;

	if (bytesRead < 0)
		Log::Writef("%s :: hid_read failed (%ls)", name, backend->error());
	else
		Log::Writef("%s :: unexpected number of bytes read (%i) expected (%i)", name, bytesRead, expectedSize);

	return ReadDataResult::FAILURE;
}

bool Reporter::WriteData(uint8_t reportId, const char* name, bool performErrorCheck)
{
	// Linux wants reports of at leats 2 bytes
	uint8_t buf[2] = { reportId, 0 };

	int bytesWritten = backend->write(buf, sizeof(buf));
	if (bytesWritten > 0 || !performErrorCheck)
	{
		Log::Writef("%s :: done", name);
		return true;
	}
	Log::Writef("%s :: hid_write failed (%ls)", name, backend->error());
	return false;
}

class BackendHid : public ReporterBackend
{
public:
	BackendHid(hid_device* device)
		:ReporterBackend(), myHid(device)
	{
	}

	~BackendHid()
	{
		hid_close(myHid);
	}
	
	int get_feature_report(unsigned char *data, size_t length)
	{
		return hid_get_feature_report(myHid, data, length);
	}

	int send_feature_report(const unsigned char *data, size_t length)
	{
		return hid_send_feature_report(myHid, data, length);
	}

	int read(unsigned char *data, size_t length)
	{
		return hid_read(myHid, data, length);
		// return hid_read_timeout(myHid, data, length, 1);
	}

	int write(unsigned char *data, size_t length)
	{
		return hid_write(myHid, data, length);
	}

	const wchar_t* error()
	{
		return hid_error(myHid);
	}
protected:
	hid_device* myHid;
};

// ====================================================================================================================
// Reporter.
// ====================================================================================================================

Reporter::Reporter(hid_device* device)
	// : backend(new BackendHid(device)), deviceServer(DeviceServerCreate(device))
	: backend(new BackendHid(device))
{
	
}

#ifdef DEVICE_CLIENT_ENABLED
Reporter::Reporter(std::string url)
	:backend(ReporterBackendWsCreate(url))
{

}
#endif

Reporter::Reporter()
{
	emulator = false;
}

Reporter::~Reporter()
{
	
}

#ifdef DEVICE_SERVER_ENABLED
bool Reporter::ServerStart()
{
	if (deviceServer)
		return false;

	deviceServer.reset(DeviceServerCreate(*backend));

	return true;
}
#endif

ReadDataResult Reporter::Get(SensorValuesReport& report, int numSensors)
{
	if(emulator) {
		return ReadDataResult::NO_DATA;
	}

	int expectedSize = sizeof(uint8_t) + sizeof(uint16_le) + (sizeof(uint16_le) * numSensors);
	
	return ReadData(report, "GetSensorValuesReport", expectedSize);
}

bool Reporter::Get(PadConfigurationReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(report, "GetPadConfigurationReport");
}

bool Reporter::Get(NameReport& report)
{
	if(emulator) {
		const char* name = "ADP Emulator";
		memcpy(&report.name, name, sizeof(name));
		report.size = sizeof(name);
		
		return true;
	}

	return GetFeatureReport(report, "GetNameReport");
}

bool Reporter::Get(IdentificationReport& report)
{
	if(emulator) {
		report.buttonCount = 12;
		report.sensorCount = 12;
		report.ledCount = 0;
		
		return true;
	}

	return GetFeatureReport(report, "GetIdentificationReport");
}

bool Reporter::Get(IdentificationV2Report& report)
{
	if (emulator) {
		report.buttonCount = 12;
		report.sensorCount = 12;
		report.ledCount = 0;

		return true;
	}

	return GetFeatureReport(report, "GetIdentificationV2Report");
}

bool Reporter::Get(LightRuleReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(report, "GetLightRuleReport");
}

bool Reporter::Get(LedMappingReport& report)
{
	if(emulator) {
		return true;
	}

	return GetFeatureReport(report, "GetLedMappingReport");
}

bool Reporter::Get(SensorReport& report)
{
	if (emulator) {
		return true;
	}

	return GetFeatureReport(report, "GetSensorReport");
}

bool Reporter::Get(DebugReport& report)
{
	if (emulator) {
		return true;
	}

	return GetFeatureReport(report, "GetDebugReport");
}

bool Reporter::Get(SetPropertyReport& report)
{
	if (emulator) {
		return true;
	}

	return GetFeatureReport(report, "SetPropertyReport");
}

void Reporter::SendReset()
{
	WriteData(REPORT_RESET, "SendResetReport", false);
}

void Reporter::SendFactoryReset()
{
	WriteData(REPORT_FACTORY_RESET, "SendFactoryResetReport", false);
}

bool Reporter::SendSaveConfiguration()
{
	if(emulator) {
		return true;
	}

	return WriteData(REPORT_SAVE_CONFIGURATION, "SendSaveConfigurationReport", true);
}

bool Reporter::Send(const PadConfigurationReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(report, "SendPadConfigurationReport");
}

bool Reporter::Send(const NameReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(report, "SendNameReport");
}

bool Reporter::Send(const LightRuleReport& report)
{
	if(emulator) {
		return true;
	}

	return SendFeatureReport(report, "SendLightRuleReport");
}

bool Reporter::Send(const LedMappingReport& report)
{
	if(emulator) {
		return true;
	}
	
	return SendFeatureReport(report, "SendLedMappingReport");
}

bool Reporter::Send(const SensorReport& report)
{
	return SendFeatureReport(report, "SendSensorReport");
}

bool Reporter::Send(const SetPropertyReport& report)
{
	if(emulator) {
		return true;
	}
	
	return SendFeatureReport(report, "SendSetPropertyReport");
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

bool Reporter::SendAndGet(SetPropertyReport& report)
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
