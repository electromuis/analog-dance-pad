#include "reporter.h"
#include "log.h"
#include "utils.h"

namespace mpc {

// ====================================================================================================================
// Helper functions.
// ====================================================================================================================

template <typename T>
static bool GetFeatureReport(hid_device* hid, T& report, const wchar_t* name)
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
		Log::Writef(L"%s :: hid_get_feature_report failed (%s)", name, hid_error(hid));
	else
		Log::Writef(L"%s :: unexpected number of bytes read (%i)", name, bytesRead);
	return false;
}

template <typename T>
static bool SendFeatureReport(hid_device* hid, const T& report, const wchar_t* name)
{
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
		Log::Writef(L"%s :: hid_read failed (%s)", name, hid_error(hid));
	else
		Log::Writef(L"%s :: unexpected number of bytes read (%i)", name, bytesRead);

	return ReadDataResult::FAILURE;
}

static bool WriteData(hid_device* hid, uint8_t reportId, const wchar_t* name, bool performErrorCheck)
{
	int bytesWritten = hid_write(hid, &reportId, sizeof(reportId));
	if (bytesWritten > 0 || !performErrorCheck)
	{
		Log::Writef(L"%s :: done", name);
		return true;
	}
	Log::Writef(L"%s :: hid_write failed (%s)", name, hid_error(hid));
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

bool Reporter::Get(LightsReport& report)
{
	return GetFeatureReport(myHid, report, L"GetLightsReport");
}

void Reporter::SendReset()
{
	WriteData(myHid, REPORT_RESET, L"SendResetReport", false);
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

bool Reporter::Send(const LightsReport& report)
{
	return SendFeatureReport(myHid, report, L"SendLightsReport");
}

}; // namespace mpc.