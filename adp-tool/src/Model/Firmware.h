#pragma once

#include "stdint.h"
#include <string>
#include <functional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifndef __EMSCRIPTEN__
	#include "avrdude-slic3r.hpp"
	#include "serial/serial.h"

	using namespace Slic3r;
	using namespace serial;
#endif // __EMSCRIPTEN__


namespace adp {

enum BoardType {
	BOARD_UNKNOWN,
	BOARD_FSRMINIPAD,
	BOARD_FSRMINIPAD_V2,
	BOARD_TEENSY2,
	BOARD_LEONARDO,
	BOARD_FSRIO_V1,
	BOARD_FSRIO_V2,
	BOARD_FSRIO_V3,
};

enum FlashResult
{
	FLASHRESULT_NOTHING,
	FLASHRESULT_CONNECTED,
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

typedef std::function<void(AvrdudeEvent event, std::string message, int progress)> FirmwareCallback;

#ifndef __EMSCRIPTEN__
class FirmwareUploader
{
public:
	FlashResult UpdateFirmware(std::string fileName);
	FlashResult ConnectDevice();
	bool SetPort(std::string portName);
	void SetIgnoreBoardType(bool ignoreBoardType);
	void WritingDone(int exitCode);
	std::string GetErrorMessage();
	FlashResult GetFlashResult();
	void SetEventHandler(FirmwareCallback callback) {this->callback = callback;};

	AvrDude::Ptr myAvrdude;

	FlashResult WriteFirmwareAvr(std::string fileName);
	FlashResult WriteFirmwareEsp(std::string fileName);

private:

	PortInfo comPort;
	std::string firmwareFile;
	std::string errorMessage;
	FlashResult flashResult = FLASHRESULT_NOTHING;
	json* configBackup = NULL;
	bool ignoreBoardType = false;
	FirmwareCallback callback;
};
#endif //__EMSCRIPTEN__


enum BoardType ParseBoardType(const std::string& str);
const char* BoardTypeToString(BoardType boardType);
const char* BoardTypeToString(BoardType boardType, bool firmwareFile);

}; // namespace adp.