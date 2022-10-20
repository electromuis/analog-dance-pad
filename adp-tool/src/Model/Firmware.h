#pragma once

#include "stdint.h"
#include <string>

#include "avrdude-slic3r.hpp"
#include "serial/serial.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace Slic3r;
using namespace serial;

namespace adp {

enum BoardType {
	BOARD_UNKNOWN,
	BOARD_FSRMINIPAD,
	BOARD_FSRMINIPAD_V2,
	BOARD_TEENSY2,
	BOARD_LEONARDO,
	BOARD_FSRIO_V1
};

enum FlashResult
{
	FLASHRESULT_NOTHING,
	FLASHRESULT_SUCCESS,
	FLASHRESULT_FAILURE,
	FLASHRESULT_FAILURE_BOARDTYPE,
	FLASHRESULT_RUNNING,
	FLASHRESULT_CANCELLED
};

enum AvrdudeEvent
{
	AE_START,
	AE_MESSAGE,
	AE_PROGRESS,
	AE_STATUS,
	AE_EXIT,
};

class FirmwareUploader
{
public:
	FlashResult UpdateFirmware(std::string fileName);
	void SetIgnoreBoardType(bool ignoreBoardType);
	void WritingDone(int exitCode);
	std::string GetErrorMessage();
	FlashResult GetFlashResult();

	AvrDude::Ptr myAvrdude;

private:
	FlashResult WriteFirmware();

	PortInfo comPort;
	std::string firmwareFile;
	std::string errorMessage;
	FlashResult flashResult = FLASHRESULT_NOTHING;
	json* configBackup = NULL;
	bool ignoreBoardType = false;
};


enum BoardType ParseBoardType(const std::string& str);
const char* BoardTypeToString(BoardType boardType);
const char* BoardTypeToString(BoardType boardType, bool firmwareFile);

}; // namespace adp.
