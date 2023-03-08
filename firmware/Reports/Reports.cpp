#include "ReportsInternal.h"
#include "PadReports.h"
#include "LightReports.h"
#include "SensorReports.h"

typedef struct ReportRegistration {
    ReportRegistration();
    ReportRegistration(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write, ProcessReportFunc* Process);
    
    WriteReportFunc* Write;
    ProcessReportFunc* Process;

    uint8_t ReportID;
    uint16_t ReportSize;
} ReportRegistration;

ReportRegistration registrations[18];

void RegisterReportInternal(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write, ProcessReportFunc* Process)
{
    registrations[ReportID].ReportID = ReportID;
    registrations[ReportID].ReportSize = ReportSize;
    registrations[ReportID].Write = Write;
    registrations[ReportID].Process = Process;
}

ReportRegistration::ReportRegistration()
    :ReportID(0), ReportSize(0), Write(nullptr), Process(nullptr)
{

}

void RegisterReports()
{
    RegisterPadReports();
    RegisterLightReports();
    RegisterSensorReports();
}

bool WriteReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize)
{
    if(ReportID >= 18) {
        return false;
    }

    if(registrations[ReportID].Write == nullptr) {
        return false;
    }

    *ReportSize = registrations[ReportID].ReportSize;
    registrations[ReportID].Write(ReportData);

    return true;
}

bool ProcessReport(const uint8_t ReportID, void* ReportData, uint16_t const ReportSize)
{
    if(ReportID >= 16) {
        return false;
    }

    if(registrations[ReportID].Process == nullptr) {
        return false;
    }

    if(ReportSize != registrations[ReportID].ReportSize && registrations[ReportID].ReportSize != 0) {
        return false;
    }

    registrations[ReportID].Process(ReportData);

    return true;
}