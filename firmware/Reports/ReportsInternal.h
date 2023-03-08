#pragma once

#include "Reports.h"


extern "C" {
#include "../Config/DancePadConfig.h"
#include "../ConfigStore.h"
#include "../Descriptors.h"
}

typedef void WriteReportFunc(void* ReportData);
typedef void ProcessReportFunc(void* ReportData);

void RegisterReportInternal(const uint8_t ReportID, uint16_t const ReportSize, WriteReportFunc* Write, ProcessReportFunc* Process);

#define REGISTER_REPORT_PRE(ID, T, WRITE, PROCESS) RegisterReportInternal(ID, sizeof(T), WRITE, PROCESS);

#define REGISTER_REPORT(ID, T) \
    REGISTER_REPORT_PRE(ID, T, [](void* ReportData){ \
        T* instance = (T*)ReportData; \
        *instance = T(); \
    }, [](void* ReportData){ \
        T* instance = (T*)ReportData; \
        instance->Process(); \
    });

#define REGISTER_REPORT_WRITE(ID, T) \
    REGISTER_REPORT_PRE(ID, T, [](void* ReportData){ \
        T* instance = (T*)ReportData; \
        *instance = T(); \
    }, nullptr);

#define REGISTER_REPORT_PROCESS(ID, T) \
    REGISTER_REPORT_PRE(ID, T, nullptr, [](void* ReportData){ \
        T* instance = (T*)ReportData; \
        instance->Process(); \
    });

#define REGISTER_REPORT_PROCESS_FUNC(ID, PROCESS) RegisterReportInternal(ID, 0, nullptr, [](void* ReportData){ \
        PROCESS(); \
    });