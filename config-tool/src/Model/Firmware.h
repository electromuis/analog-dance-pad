#pragma once

#include "stdint.h"
#include <string>

#include "avrdude-slic3r.hpp"
#include "serial/serial.h"
#include "wx/event.h"

using namespace std;
using namespace Slic3r;
using namespace serial;

namespace adp {

enum BoardType {
	BOARD_UNKNOWN,
	BOARD_FSRMINIPAD
};

enum FlashResult
{
	FLASHRESULT_SUCCESS,
	FLASHRESULT_FAILURE,
	FLASHRESULT_CANCELLED
};

class FirmwareUploader
{
public:
	FlashResult UpdateFirmware(wstring fileName);
	void SetEventHandler(wxEvtHandler* handler);
	void SetIgnoreBoardType(bool ignoreBoardType);


private:
	FlashResult WriteFirmware();

	PortInfo comPort;
	wstring firmwareFile;
	wxEvtHandler* eventHandler;
	wstring errorMessage;
	bool ignoreBoardType = false;
	AvrDude* avrdude;
};


enum BoardType ParseBoardType(const std::string& str);

}; // namespace adp.
