#include "ReportsInternal.hpp"
#include "PadReports.hpp"
#include "LightReports.hpp"
#include "SensorReports.hpp"
#include "Modules/ModuleDebug.hpp"

#include "hid_def.h"

#include <string.h>

typedef struct ReportRegistration {
    ReportRegistration();
    ReportRegistration(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write, ProcessReportFunc* Process);
    
    WriteReportFunc* Write = nullptr;
    ProcessReportFunc* Process = nullptr;

    uint8_t ReportID = 0;
    uint16_t ReportSize = 0;
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

bool WriteReport(const uint8_t reqReportID, uint8_t* ReportData, uint16_t* const ReportSize)
{

    uint8_t ReportID = reqReportID;
    if(ReportID == 0) {
        ReportID = INPUT_REPORT_ID;
    }

    if(ReportID >= 18) {
        return false;
        ModuleDebugInstance.Write("Write ReportID >= 18\n");
    }

    if(registrations[ReportID].Write == nullptr) {
        ModuleDebugInstance.Write("Write ReportID not registered: %d\n", ReportID);
        return false;
    }

    *ReportSize = registrations[ReportID].ReportSize;
    registrations[ReportID].Write(ReportData);

    return true;
}

bool ProcessReport(const uint8_t ReportID, const uint8_t* ReportData, uint16_t const ReportSize)
{
    if(ReportID >= 18) {
        ModuleDebugInstance.Write("Process ReportID >= 18\n");
        return false;
    }

    if(registrations[ReportID].Process == nullptr) {
        ModuleDebugInstance.Write("Process ReportID not registered: %d\n", ReportID);
        return false;
    }

    if(ReportSize != registrations[ReportID].ReportSize && registrations[ReportID].ReportSize != 0) {
        ModuleDebugInstance.Write("Process ReportSize mismatch: %d != %d\n", ReportSize, registrations[ReportID].ReportSize);
        return false;
    }

    registrations[ReportID].Process(ReportData);

    return true;
}

#define ADD_DESCRIPTOR_BLOCK(...) \
    { \
        uint8_t descriptorBlock[] = { __VA_ARGS__ }; \
        int blockLength = sizeof(descriptorBlock); \
        memcpy(Buffer + offset, &descriptorBlock, blockLength); \
        offset += blockLength; \
    }

#define HID_USAGE_DEF_1 1, 0x01
#define HID_USAGE_DEF_2 1, 0x02
#define HID_USAGE_DEF_4 1, 0x04
#define HID_USAGE_DEF_9 1, 0x09

#define HID_USAGE_PAGE_DEF_1 1, 0x01
#define HID_USAGE_PAGE_DEF_2 1, 0x02
#define HID_USAGE_PAGE_DEF_4 1, 0x04
#define HID_USAGE_PAGE_DEF_9 1, 0x09

#define HID_USAGE_PAGE_VENDOR 2, 0xFF00

uint16_t WriteDescriptor(uint8_t* Buffer)
{
    uint16_t offset = 0;

    ADD_DESCRIPTOR_BLOCK(
        HID_USAGE_PAGE(DEF_1),
        HID_USAGE(DEF_4),
        HID_COLLECTION(APPLICATION)
    );

    for(auto& report : registrations) {
        if(report.ReportID == 0 || report.ReportID == LIGHTS_REPORT_ID) {
            continue;
        }

        if(report.ReportID == INPUT_REPORT_ID) {
            ADD_DESCRIPTOR_BLOCK(
                HID_REPORT_ID(report.ReportID),
                HID_USAGE_PAGE(DEF_9),
                HID_USAGE_MINIMUM(1, 0x01),
                HID_USAGE_MAXIMUM(1, BUTTON_COUNT),
                HID_LOGICAL_MINIMUM(1, 0),
                HID_LOGICAL_MAXIMUM(1, 1),
                HID_REPORT_SIZE(0x01),
                HID_REPORT_COUNT(BUTTON_COUNT),
                HID_INPUT(DATA, VARIABLE, ABSOLUTE),
                // TODO: padding here if BUTTON_COUNT not divisible by 8
                HID_USAGE_PAGE(VENDOR),
                HID_USAGE(DEF_1),
                HID_COLLECTION(PHYSICAL),
                    HID_USAGE(DEF_1),
                    HID_LOGICAL_MINIMUM(1, 0),
                    HID_LOGICAL_MAXIMUM(1, 0xFF),
                    HID_REPORT_SIZE(0x08),
                    HID_REPORT_COUNT(SENSOR_COUNT * 2),
                    HID_INPUT(DATA, VARIABLE, ABSOLUTE),
                HID_END_COLLECTION(0)
            );

            continue;
        }

        // Output report
        if(report.ReportSize == 0) {
            ADD_DESCRIPTOR_BLOCK(
                HID_REPORT_ID(report.ReportID),
                HID_USAGE_PAGE(VENDOR),
                HID_USAGE(DEF_2),
                HID_OUTPUT(DATA, VARIABLE, ABSOLUTE, NON_VOLATILE)
            );
        } else {
            ADD_DESCRIPTOR_BLOCK(
                HID_REPORT_ID(report.ReportID),
                HID_USAGE_PAGE(VENDOR),
                HID_USAGE(DEF_2),
                HID_COLLECTION(PHYSICAL),
                    HID_USAGE(DEF_2),
                    HID_LOGICAL_MINIMUM(1, 0),
                    HID_LOGICAL_MAXIMUM(1, 0xFF),
                    HID_REPORT_SIZE(0x08),
                    HID_REPORT_COUNT((uint8_t)report.ReportSize),
                    HID_FEATURE(DATA, VARIABLE, ABSOLUTE, NON_VOLATILE),
                HID_END_COLLECTION(0x00)
            );
        }
    }

    ADD_DESCRIPTOR_BLOCK(HID_END_COLLECTION(0x00))

    return offset;
}