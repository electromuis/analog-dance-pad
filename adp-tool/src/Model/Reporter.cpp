#include "Adp.h"

#include <cstring>
#include <chrono>
#include <thread>

#include "Model/Reporter.h"
#include "Model/Log.h"
#include "Model/Utils.h"

using namespace std;

namespace adp {

// ====================================================================================================================
// Helper functions.
// ====================================================================================================================

template <typename T>
static bool GetFeatureReport(hid_device* hid, T& report, const wchar_t* name)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	auto size = sizeof(T);
	auto expectedSize = sizeof(T);
	#if _WIN32
        expectedSize ++;
	#endif

	int bytesRead = hid_get_feature_report(hid, buffer, sizeof(buffer));
	if (bytesRead == expectedSize)
	{
		memcpy(&report, buffer, size);
		return true;
	}

	if (bytesRead < 0)
		Log::Writef(L"%ls :: hid_get_feature_report failed (%ls)", name, hid_error(hid));
	else
		Log::Writef(L"%ls :: unexpected number of bytes read (%i) expected (%i)", name, bytesRead, expectedSize);
	return false;
}

template <typename T>
static bool SendFeatureReport(hid_device* hid, const T& report, const wchar_t* name)
{
	using namespace std::chrono_literals;

	// Wait for the controller to get into a ready state
	std::this_thread::sleep_for(2ms);

	int bytesWritten = hid_send_feature_report(hid, (const unsigned char*)&report, sizeof(T));
	if (bytesWritten == sizeof(T))
	{
		Log::Writef(L"%s :: done", name);
		return true;
	}

	if (bytesWritten < 0)
		Log::Writef(L"%s :: hid_send_feature_report failed (%s)", name, hid_error(hid));
	else
		Log::Writef(L"%s :: unexpected number of bytes written (%i)", name, bytesWritten);
	return false;
}

template <typename T>
static ReadDataResult ReadData(hid_device* hid, T& report, const wchar_t* name)
{
	uint8_t buffer[MAX_REPORT_SIZE];
	buffer[0] = report.reportId;

	int bytesRead = hid_read(hid, buffer, sizeof(buffer));
	if (bytesRead == sizeof(SensorValuesReport))
	{
		memcpy(&report, buffer, sizeof(SensorValuesReport));
		return ReadDataResult::SUCCESS;
	}

	if (bytesRead == 0)
		return ReadDataResult::NO_DATA;

	if (bytesRead < 0)
		Log::Writef(L"%ls :: hid_read failed (%ls)", name, hid_error(hid));
	else
		Log::Writef(L"%ls :: unexpected number of bytes read (%i)", name, bytesRead);

	return ReadDataResult::FAILURE;
}

static bool WriteData(hid_device* hid, uint8_t reportId, const wchar_t* name, bool performErrorCheck)
{
	int bytesWritten = hid_write(hid, &reportId, sizeof(reportId));
	if (bytesWritten > 0 || !performErrorCheck)
	{
		Log::Writef(L"%ls :: done", name);
		return true;
	}
	Log::Writef(L"%ls :: hid_write failed (%ls)", name, hid_error(hid));
	return false;
}

// ====================================================================================================================
// Reporter.
// ====================================================================================================================

Reporter::Reporter(hid_device* device)
	: myHid(device)
{
}

Reporter::~Reporter()
{
	hid_close(myHid);
}

ReadDataResult Reporter::Get(SensorValuesReport& report)
{
	return ReadData(myHid, report, L"GetSensorValuesReport");
}

bool Reporter::Get(PadConfigurationReport& report)
{
	return GetFeatureReport(myHid, report, L"GetPadConfigurationReport");
}

bool Reporter::Get(NameReport& report)
{
	return GetFeatureReport(myHid, report, L"GetNameReport");
}

bool Reporter::Get(IdentificationReport& report)
{
	return GetFeatureReport(myHid, report, L"GetIdentificationReport");
}

bool Reporter::Get(LightRuleReport& report)
{
	return GetFeatureReport(myHid, report, L"GetLightRuleReport");
}

bool Reporter::Get(LedMappingReport& report)
{
	return GetFeatureReport(myHid, report, L"GetLedMappingReport");
}

void Reporter::SendReset()
{
	WriteData(myHid, REPORT_RESET, L"SendResetReport", false);
}

void Reporter::SendFactoryReset()
{
	WriteData(myHid, REPORT_FACTORY_RESET, L"SendFactoryResetReport", false);
}

bool Reporter::SendSaveConfiguration()
{
	return WriteData(myHid, REPORT_SAVE_CONFIGURATION, L"SendSaveConfigurationReport", true);
}

bool Reporter::Send(const PadConfigurationReport& report)
{
	return SendFeatureReport(myHid, report, L"SendPadConfigurationReport");
}

bool Reporter::Send(const NameReport& report)
{
	return SendFeatureReport(myHid, report, L"SendNameReport");
}

bool Reporter::Send(const LightRuleReport& report)
{
	return SendFeatureReport(myHid, report, L"SendLightRuleReport");
}

bool Reporter::Send(const LedMappingReport& report)
{
	return SendFeatureReport(myHid, report, L"SendLedMappingReport");
}

bool Reporter::Send(const SetPropertyReport& report)
{
	return SendFeatureReport(myHid, report, L"SendSetPropertyReport");
}

}; // namespace adp.
