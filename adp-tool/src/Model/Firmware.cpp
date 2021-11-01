#include "Adp.h"

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

#include "wx/string.h"
#include "wx/event.h"

#include "Model/Firmware.h"
#include "Model/Device.h"
#include "Model/Log.h"

using namespace chrono;

namespace adp {

wxDEFINE_EVENT(EVT_AVRDUDE, wxCommandEvent);

BoardType ParseBoardType(const std::string& str)
{
	if (str == "fsrio1") { return BOARD_FSRIO_V1; }
	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	if (str == "teensy2") { return BOARD_TEENSY2; }
	if (str == "leonardo") { return BOARD_LEONARDO; }
	else { return BOARD_UNKNOWN; }
}

wstring BoardTypeToString(BoardType boardType)
{
	if (boardType == BOARD_FSRIO_V1) {
		return L"FSRio V1";
	}
	if (boardType == BOARD_FSRMINIPAD) {
		return L"FSR Mini pad";
	}

	if (boardType == BOARD_TEENSY2) {
		return L"Teensy 2";
	}

	if (boardType == BOARD_LEONARDO) {
		return L"Arduino leonardo/pro micro";
	}

	return L"Unknown";
}

FlashResult FirmwareUploader::UpdateFirmware(wstring fileName)
{
	flashResult = FLASHRESULT_NOTHING;

	errorMessage = L"";

	firmwareFile = fileName;

	auto pad = Device::Pad();
	if (pad == NULL) {
		errorMessage = L"No compatible board connected";
		flashResult = FLASHRESULT_FAILURE;
		return flashResult;
	}

	BoardType boardType = BOARD_UNKNOWN;
	
	ifstream fileStream;
	string fileNameThin(fileName.begin(), fileName.end());
	fileStream.open(fileNameThin);

	if (!fileStream.is_open()) {
		errorMessage = L"Could not read firmware file";
		flashResult = FLASHRESULT_FAILURE;
		return flashResult;
	}

	string line;
	while (getline(fileStream, line)) {
		if (line.size() > 0 && line.substr(0, 1) == ";") {
			line = line.substr(1);
			if (line.substr(line.length() - 1, line.length()) == "\r") {
				line = line.substr(0, line.length() -1);
			}

			BoardType foundType = ParseBoardType(line);
			if (foundType != BOARD_UNKNOWN) {
				boardType = foundType;
				break;
			}
		}
	}
	fileStream.close();

	if (boardType == BoardType::BOARD_UNKNOWN || pad->boardType != boardType) {
		if (!ignoreBoardType) {
			errorMessage = L"Selected: " + BoardTypeToString(boardType) + ", connected: " + BoardTypeToString(pad->boardType);
			flashResult = FLASHRESULT_FAILURE_BOARDTYPE;
			return flashResult;
		}
	}

	vector<PortInfo> oldPorts = list_ports();
	bool foundNewPort = false;
	auto startTime = system_clock::now();

	if (configBackup) {
		delete configBackup;
	}
	configBackup = new json;
	Device::SaveProfile(*configBackup, DeviceProfileGroupFlags::DGP_ALL);
	Log::Write(L"Saved device config");
	Device::SendDeviceReset();

	while (!foundNewPort)
	{
		auto timeSpent = duration_cast<std::chrono::seconds>(system_clock::now() - startTime);
		if (timeSpent.count() > 10) {
			errorMessage = L"Could not find the device in bootloader mode";
			flashResult = FLASHRESULT_FAILURE;
			return flashResult;
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
		.on_run([eventHandler, comPort, firmwareFile, this](AvrDude::Ptr avrdude) {
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

			if (eventHandler) {
				auto evt = new wxCommandEvent(EVT_AVRDUDE);
				evt->SetExtraLong(AE_START);
				wxQueueEvent(eventHandler, evt);
			}
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

			int exitCode = this->myAvrdude->exit_code();
			this->WritingDone(exitCode);

			if (eventHandler) {
				auto evt = new wxCommandEvent(EVT_AVRDUDE);
				evt->SetExtraLong(AE_EXIT);
				evt->SetInt(exitCode);
				wxQueueEvent(eventHandler, evt);
			}
		});


	// Wait a bit since the COM port might still be initializing
	this_thread::sleep_for(500ms);

	Device::SetSearching(false);
	avrdude.run();

	flashResult = FLASHRESULT_RUNNING;
	return flashResult;
}

void FirmwareUploader::WritingDone(int exitCode)
{
	// Wait for the device to come back online so we can restore the config
	Device::SetSearching(true);
	auto startTime = system_clock::now();
	do {
		auto timeSpent = duration_cast<std::chrono::seconds>(system_clock::now() - startTime);
		
		if (eventHandler) {
			auto evt = new wxCommandEvent(EVT_AVRDUDE);
			evt->SetExtraLong(AE_PROGRESS);
			evt->SetInt(timeSpent.count() * 20);
			evt->SetString("Restarting");
			wxQueueEvent(eventHandler, evt);
		}
		
		if (timeSpent.count() > 5) {
			errorMessage = L"Device failed to come back online";
			flashResult = FLASHRESULT_FAILURE;
			return;
		}

		this_thread::sleep_for(100ms);
	} while (!Device::Pad());

	if (configBackup) {
		//Device::LoadProfile(*configBackup, DeviceProfileGroupFlags::DGP_ALL);
		//Device::SaveChanges();
		Log::Write(L"Restored device config");

		delete configBackup;
	}
	
	if (exitCode == 0) {
		flashResult = FLASHRESULT_SUCCESS;
	}
	else {
		errorMessage = L"Flashing failed, see log tab for more details.";
		flashResult = FLASHRESULT_FAILURE;
	}

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

FlashResult FirmwareUploader::GetFlashResult()
{
	return flashResult;
}

}; // namespace adp.