#pragma once

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../Debug.h"

bool WriteReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize);
bool ProcessReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize);

void RegisterReports();

#ifdef __cplusplus
}
#endif
