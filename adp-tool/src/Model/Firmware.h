#pragma once

#include "stdint.h"
#include <string>

#include "avrdude-slic3r.hpp"
#include "serial/serial.h"
#include "wx/event.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;
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

wxDECLARE_EVENT(EVT_AVRDUDE, wxCommandEvent);

class FirmwareUploader
{
public:
	FlashResult UpdateFirmware(wstring fileName);
	void SetEventHandler(wxEvtHandler* handler);
	void SetIgnoreBoardType(bool ignoreBoardType);
	void WritingDone(int exitCode);
	wstring GetErrorMessage();
	FlashResult GetFlashResult();

	AvrDude::Ptr myAvrdude;

private:
	FlashResult WriteFirmware();

	PortInfo comPort;
	wstring firmwareFile;
	wxEvtHandler* eventHandler;
	wstring errorMessage;
	FlashResult flashResult = FLASHRESULT_NOTHING;
	json* configBackup = NULL;
	bool ignoreBoardType = false;
};


enum BoardType ParseBoardType(const std::string& str);
const wchar_t* BoardTypeToString(BoardType boardType);
const wchar_t* BoardTypeToString(BoardType boardType, bool firmwareFile);

}; // namespace adp.
