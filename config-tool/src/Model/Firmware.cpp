#include "Model/Firmware.h"
#include "Model/Device.h"
#include "Model/Log.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

#include "wx/string.h"
#include "wx/event.h"

using namespace chrono;

namespace adp {

enum AvrdudeEvent
{
	AE_MESSAGE,
	AE_PROGRESS,
	AE_STATUS,
	AE_EXIT,
};

BoardType ParseBoardType(const std::string& str)
{
	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	else { return BOARD_UNKNOWN; }
}

FlashResult FirmwareUploader::UpdateFirmware(wstring fileName)
{
	firmwareFile = fileName;

	auto pad = Device::Pad();
	if (pad == NULL) {
		errorMessage = L"No compatible board connected";
		return FLASHRESULT_FAILURE;
	}

	BoardType boardType = BOARD_UNKNOWN;
	
	//todo fix linux
#ifdef _MSC_VER
	ifstream fileStream = ifstream(firmwareFile);
	if (!fileStream.is_open()) {
		errorMessage = L"Could not read firmware file";
		return FLASHRESULT_FAILURE;
	}

	string line;
	while (getline(fileStream, line)) {
		if (line.size() > 0 && line.substr(0, 1) == ";") {
			BoardType foundType = ParseBoardType(line.substr(1));
			if (foundType != BOARD_UNKNOWN) {
				boardType = foundType;
				break;
			}
		}
	}
	fileStream.close();
#else
	return FLASHRESULT_FAILURE;
#endif

	if (boardType == BoardType::BOARD_UNKNOWN || pad->boardType != boardType) {
		if (!ignoreBoardType) {
			errorMessage = L"Your firmware does not seem compatible with the detected hardware";
			return FLASHRESULT_FAILURE;
		}
	}

	vector<PortInfo> oldPorts = list_ports();
	bool foundNewPort = false;
	auto startTime = system_clock::now();

	Device::SendDeviceReset();

	while (!foundNewPort)
	{
		auto timeSpent = duration_cast<std::chrono::seconds>(system_clock::now() - startTime);
		if (timeSpent.count() > 10) {
			errorMessage = L"Could not find the device in bootloader mode";
			return FLASHRESULT_FAILURE;
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
				comPort = port;
				break;
			}
		}	
	}

	return WriteFirmware();
}

FlashResult FirmwareUploader::WriteFirmware()
{
	if (avrdude == NULL) {
		avrdude = new AvrDude();
	}

	auto comPort = this->comPort.port;
	auto firmwareFile = this->firmwareFile;

	avrdude
		->on_run([comPort, firmwareFile](AvrDude::Ptr avrdude) {
		std::vector<std::string> args{ {
			"-v",
			"-p", "atmega32u4",
			"-c", "avr109",
			"-P", comPort,
			"-b", "115200",
			"-D",
			"-U", wxString::Format("flash:w:1:%s:i", firmwareFile).ToStdString(),
		} };

		avrdude->push_args(std::move(args));

		Log::Write(L"Prepared avrdude args");
			})
		.on_message([](const char* msg, unsigned /* size */) {
				auto wxmsg = wxString::FromUTF8(msg);
				//Log::Write(L"avrdude: " + wxmsg);
			})
		.on_progress([](const char* task, unsigned progress) {
		Log::Write(wxString::Format("Task: %s Progress: %i", task, progress).c_str());

			})
		.on_complete([]() {
				Log::Write(L"avrdude done");
			})
		.run();

		return FLASHRESULT_SUCCESS;
}

void FirmwareUploader::SetIgnoreBoardType(bool ignoreBoardType)
{
	this->ignoreBoardType = ignoreBoardType;
}

void FirmwareUploader::SetEventHandler(wxEvtHandler* handler)
{
	this->eventHandler = handler;
}

}; // namespace adp.