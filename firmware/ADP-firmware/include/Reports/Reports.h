#pragma once

#include "stdint.h"
#include "stdbool.h"

bool WriteReport(const uint8_t ReportID, void* ReportData, uint16_t* const ReportSize);
bool ProcessReport(const uint8_t ReportID, void* ReportData, uint16_t const ReportSize);

void RegisterReports();