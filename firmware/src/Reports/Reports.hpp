#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "Structs.hpp"

bool WriteReport(const uint8_t ReportID, uint8_t* ReportData, uint16_t* const ReportSize);
bool ProcessReport(const uint8_t ReportID, const uint8_t* ReportData, uint16_t const ReportSize);

uint16_t WriteDescriptor(uint8_t* Buffer);

void RegisterReports();

