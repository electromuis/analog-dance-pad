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

wxDEFINE_EVENT(EVT_AVRDUDE, wxCommandEvent);

BoardType ParseBoardType(const std::string& str)
{
	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	else { return BOARD_UNKNOWN; }
}

FlashResult FirmwareUploader::UpdateFirmware(wstring fileName)
{
	errorMessage = L"";

	firmwareFile = fileName;

	auto pad = Device::Pad();
	if (pad == NULL) {
		errorMessage = L"No compatible board connected";
		return FLASHRESULT_FAILURE;
	}

	BoardType boardType = BOARD_UNKNOWN;
	
	ifstream fileStream = ifstream(firmwareFile, std::ios::in | std::ios::binary);
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
	AvrDude avrdude;

	auto comPort = this->comPort.port;
	auto firmwareFile = this->firmwareFile;
	auto eventHandler = this->eventHandler;

	avrdude
		.on_run([comPort, firmwareFile, this](AvrDude::Ptr avrdude) {
			this->myAvrdude = std::move(avrdude);

			std::vector<std::string> args{ {
				"-v",
				"-p", "atmega32u4",
				"-c", "avr109",
				"-P", comPort,
				"-b", "115200",
				"-D",
				"-U", wxString::Format("flash:w:1:%s:i", firmwareFile).ToStdString(),
			} };

			this->myAvrdude->push_args(std::move(args));
		})
		.on_message([eventHandler](const char* msg, unsigned size) {
			auto wxmsg = wxString::FromUTF8(msg);
			Log::Write(L"avrdude: " + wxmsg);

			if (eventHandler) {
				auto evt = new wxCommandEvent(EVT_AVRDUDE);
				evt->SetExtraLong(AE_MESSAGE);
				evt->SetString(std::move(wxmsg));
				wxQueueEvent(eventHandler, evt);
			}
		})
		.on_progress([eventHandler](const char* task, unsigned progress) {
			auto wxmsg = wxString::FromUTF8(task);
			
			if (eventHandler) {
				auto evt = new wxCommandEvent(EVT_AVRDUDE);
				evt->SetExtraLong(AE_PROGRESS);
				evt->SetInt(progress);
				evt->SetString(wxmsg);
				wxQueueEvent(eventHandler, evt);
			}
		})
		.on_complete([eventHandler, this]() {
			Log::Write(L"avrdude done");

			if (eventHandler) {
				auto evt = new wxCommandEvent(EVT_AVRDUDE);
				evt->SetExtraLong(AE_EXIT);
				evt->SetInt(this->myAvrdude->exit_code());
				wxQueueEvent(eventHandler, evt);
			}

			this->WritingDone();
		})
		.run();

	return FLASHRESULT_RUNNING;
}

void FirmwareUploader::WritingDone()
{
	//if (myAvrdude) { myAvrdude->join(); }
	//myAvrdude.reset();
}

void FirmwareUploader::SetIgnoreBoardType(bool ignoreBoardType)
{
	this->ignoreBoardType = ignoreBoardType;
}

void FirmwareUploader::SetEventHandler(wxEvtHandler* handler)
{
	this->eventHandler = handler;
}

wstring FirmwareUploader::GetErrorMessage()
{
	return errorMessage;
}

}; // namespace adp.