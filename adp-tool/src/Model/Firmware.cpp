#include <Adp.h>

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

#include <Model/Firmware.h>
#include <Model/Device.h>
#include <Model/Log.h>
#include <Model/Utils.h>

using namespace std;
using namespace chrono;

namespace adp {

BoardType ParseBoardType(const std::string& str)
{
	if (str == "fsrio1") { return BOARD_FSRIO_V1; }
	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	if (str == "teensy2") { return BOARD_TEENSY2; }
	if (str == "leonardo") { return BOARD_LEONARDO; }
	else { return BOARD_UNKNOWN; }
}

const char* BoardTypeToString(BoardType boardType, bool firmwareFile)
{
	if (boardType == BOARD_FSRIO_V1) {
		return firmwareFile ? "FSRioV1" : "FSRio V1";
	}

	if (boardType == BOARD_FSRMINIPAD) {
		return firmwareFile ? "FSRMiniPad" : "FSR Mini pad";
	}

	if (boardType == BOARD_FSRMINIPAD_V2) {
		return firmwareFile ? "FSRMiniPadV2" : "FSR Mini pad V2";
	}

	if (boardType == BOARD_TEENSY2) {
		return firmwareFile ? "Teensy2" : "Teensy 2";
	}

	if (boardType == BOARD_LEONARDO) {
		return firmwareFile ? "Generic" : "Arduino leonardo/pro micro";
	}

	return "Unknown";
}

const char* BoardTypeToString(BoardType boardType)
{
	return BoardTypeToString(boardType, false);
}

FlashResult FirmwareUploader::UpdateFirmware(string fileName)
{
	flashResult = FLASHRESULT_NOTHING;

	errorMessage = "";

	firmwareFile = fileName;

	auto pad = Device::Pad();
	if (pad == NULL) {
		errorMessage = "No compatible board connected";
		flashResult = FLASHRESULT_FAILURE;
		return flashResult;
	}

	BoardType boardType = BOARD_UNKNOWN;

	ifstream fileStream;
	string fileNameThin(fileName.begin(), fileName.end());
	fileStream.open(fileNameThin);

	if (!fileStream.is_open()) {
		errorMessage = "Could not read firmware file";
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
			// errorMessage = wxString::Format("Selected: %ls, connected: %ls", BoardTypeToString(boardType), BoardTypeToString(pad->boardType));
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
	Log::Write("Saved device config");
	Device::SendDeviceReset();

	while (!foundNewPort)
	{
		auto timeSpent = duration_cast<std::chrono::seconds>(system_clock::now() - startTime);
		if (timeSpent.count() > 10) {
			errorMessage = "Could not find the device in bootloader mode";
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
	return FLASHRESULT_NOTHING;
}

void FirmwareUploader::WritingDone(int exitCode)
{
}

void FirmwareUploader::SetIgnoreBoardType(bool ignoreBoardType)
{
	this->ignoreBoardType = ignoreBoardType;
}

string FirmwareUploader::GetErrorMessage()
{
	return errorMessage;
}

FlashResult FirmwareUploader::GetFlashResult()
{
	return flashResult;
}

}; // namespace adp.
