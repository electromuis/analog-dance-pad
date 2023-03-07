#pragma once

#include "Reports.h"

typedef void WriteReportFunc(void* ReportData);
typedef void ProcessReportFunc(void* ReportData);

struct ReportRegistration {
    ReportRegistration();
    ReportRegistration(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write);
    
    WriteReportFunc* Write;
    ProcessReportFunc* Process;

    uint8_t ReportID;
    uint16_t ReportSize;
};

#define REGISTER_REPORT_PRE(ID, T, WRITE, PROCESS) ReportRegistration T ## Registration(ID, sizeof(T), WRITE, PROCESS);

#define REGISTER_REPORT(ID, T) \
    REGISTER_REPORT_PRE([](void* ReportData){ \
        T* instance = (T*)ReportData; \
        *instance = T(); \
    }, [](void* ReportData){ \
        T* instance = (T*)ReportData; \
        instance->Process(); \
    });
