#include "Model/Firmware.h"
#include "Model/Device.h"
#include "Model/Log.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

#include "serial/serial.h"
#include "wx/string.h"
#include "avrdude-slic3r.hpp"

using namespace serial;
using namespace chrono;
using namespace Slic3r;

namespace adp {

enum AvrdudeEvent
{
	AE_MESSAGE,
	AE_PROGRESS,
	AE_STATUS,
	AE_EXIT,
};

enum AvrDudeComplete
{
	AC_NONE,
	AC_SUCCESS,
	AC_FAILURE,
	AC_USER_CANCELLED,
};

BoardType ParseBoardType(const std::string& str)
{
	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	else { return BOARD_UNKNOWN; }
}

bool UploadFirmware(wstring fileName)
{
	auto pad = Device::Pad();
	if (pad == NULL) {
		return false;
	}

	BoardType boardType = BOARD_UNKNOWN;
	ifstream firmwareFile(fileName);
	if (!firmwareFile.is_open()) {
		return false;
	}

	string line;
	while (getline(firmwareFile, line)) {
		if (line.size() > 0 && line.substr(0, 1) == ";") {
			BoardType foundType = ParseBoardType(line.substr(1));
			if (foundType != BOARD_UNKNOWN) {
				boardType = foundType;
				break;
			}
		}
	}
	firmwareFile.close();

	if (boardType == BoardType::BOARD_UNKNOWN || pad->boardType != boardType) {
		// In the future make it a user decicion to override
		return false;
	}

	vector<PortInfo> oldPorts = list_ports();
	PortInfo newPort;
	bool foundNewPort = false;
	auto startTime = system_clock::now();

	Device::SendDeviceReset();

	while (!foundNewPort)
	{
		auto timeSpent = duration_cast<std::chrono::seconds>(system_clock::now() - startTime);
		if (timeSpent.count() > 10) {
			return false;
		}

		this_thread::sleep_for(10ms);
		vector<PortInfo> newPorts = list_ports();

		for (PortInfo port : newPorts) {
			bool found = std::find_if(
				oldPorts.begin(),
				oldPorts.end(),
				[&currentPort = port]
				(const PortInfo& checkPort) -> bool { 
					return currentPort.hardware_id == checkPort.hardware_id &&
						currentPort.port == checkPort.port;
				}
			) != oldPorts.end();

			if (!found) {
				foundNewPort = true;
				newPort = port;
				break;
			}
		}	
	}

	AvrDude avrdude;

	avrdude
		.on_run([newPort, fileName](AvrDude::Ptr avrdude) {
			std::vector<std::string> args{ {
				"-v",
				"-p", "atmega32u4",
				"-c", "avr109",
				"-P", newPort.port,
				"-b", "115200",
				"-D",	
				"-U", wxString::Format("flash:w:1:%s:i", fileName).ToStdString(),
			} };

			avrdude->push_args(std::move(args));

			Log::Write(L"Prepared avrdude args");
		})
		.on_message([](const char* msg, unsigned /* size */) {
			auto wxmsg = wxString::FromUTF8(msg);
			//Log::Write(L"avrdude: " + wxmsg);
		})
		.on_progress([](const char* /* task */, unsigned progress) {
			Log::Write(wxString::Format("Progress: %i", progress).c_str());
		})
		.on_complete([]() {
			Log::Write(L"avrdude done");
		})
		.run();

	return true;
}

}; // namespace adp.