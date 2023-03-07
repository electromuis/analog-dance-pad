#include "ReportsInternal.h"
#include "PadReports.h"
#include "LightReports.h"

ReportRegistration registrations[16];

ReportRegistration::ReportRegistration()
    :ReportID(0), ReportSize(0), Write(nullptr)
{

}

ReportRegistration::ReportRegistration(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write)
    :ReportID(ReportID), ReportSize(ReportSize), Write(Write)
{
    registrations[ReportID].ReportID = ReportID;
    registrations[ReportID].ReportSize = ReportSize;
    registrations[ReportID].Write = Write;
}

void RegisterReports()
{
    RegisterPadReports();
    RegisterLightReports();
}

bool WriteReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize)
{
    if(ReportID >= 16) {
        return false;
    }

    if(registrations[ReportID].ReportID == 0) {
        return false;
    }

    *ReportSize = registrations[ReportID].ReportSize;
    registrations[ReportID].Write(ReportData);

    return true;
}

bool ProcessReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize)
{
    if(ReportID >= 16) {
        return false;
    }

    if(registrations[ReportID].ReportID == 0) {
        return false;
    }

    if(*ReportSize != registrations[ReportID].ReportSize) {
        return false;
    }

    registrations[ReportID].Process(ReportData);

    return true;
}
