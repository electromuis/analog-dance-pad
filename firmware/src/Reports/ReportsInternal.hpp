#pragma once

#include "Reports.hpp"
#include "Modules/ModuleConfig.hpp"

typedef void WriteReportFunc(uint8_t* ReportData);
typedef void ProcessReportFunc(const uint8_t* ReportData);

void RegisterReportInternal(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write, ProcessReportFunc* Process);

#define REGISTER_REPORT_PRE(ID, T, WRITE, PROCESS) RegisterReportInternal(ID, sizeof(T), WRITE, PROCESS);

#define REGISTER_REPORT(ID, T) \
    REGISTER_REPORT_PRE(ID, T, [](uint8_t* ReportData){ \
        T* instance = (T*)ReportData; \
        *instance = T(); \
    }, [](const uint8_t* ReportData){ \
        T* instance = (T*)ReportData; \
        instance->Process(); \
    });

#define REGISTER_REPORT_WRITE(ID, T) \
    REGISTER_REPORT_PRE(ID, T, [](uint8_t* ReportData){ \
        T* instance = (T*)ReportData; \
        *instance = T(); \
    }, nullptr);

#define REGISTER_REPORT_PROCESS(ID, T) \
    REGISTER_REPORT_PRE(ID, T, nullptr, [](const uint8_t* ReportData){ \
        T* instance = (T*)ReportData; \
        instance->Process(); \
    });

#define REGISTER_REPORT_PROCESS_FUNC(ID, PROCESS) RegisterReportInternal(ID, 0, nullptr, [](const uint8_t* ReportData){ \
        PROCESS(); \
    });
