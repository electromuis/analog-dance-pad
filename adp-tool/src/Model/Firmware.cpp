#include <Adp.h>

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <fmt/core.h>

#include "libzippp.h"

#include <Model/Firmware.h>
#include <Model/Device.h>
#include <Model/Log.h>
#include <Model/Utils.h>

using namespace std;
using namespace chrono;

namespace adp {

size_t split_str(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}

// BoardTypeStruct

BoardTypeStruct::BoardTypeStruct(std::string boardType)
{
	std::vector<std::string> pts;
	split_str(boardType, pts, '_');
	if(pts.size() == 1)
	{
		archType = ARCH_AVR;
		boardType = ParseBoardType(pts[0]);
	}
	else if(pts.size() == 2)
	{
		archType = ParseArchType(pts[0]);
		boardType = ParseBoardType(pts[1]);
	}
}

ArchType BoardTypeStruct::ParseArchType(const std::string& str)
{
	if (str == "avr" || str == "m32u4")
		return ARCH_AVR;
	
	if (str == "esp" || str == "esp32s3")
		return ARCH_ESP;
	
	if (str == "avrard") { return ARCH_AVR_ARD; }
	
	return ARCH_UNKNOWN;
}

BoardType BoardTypeStruct::ParseBoardType(const std::string& str)
{
	if (str == "fsrio1" || str == "fsriov1")
		return BOARD_FSRIO_V1;
	if (str == "fsrio2" || str == "fsriov2")
		return BOARD_FSRIO_V2;
	if (str == "fsrio3" || str == "fsriov3")
		return BOARD_FSRIO_V3;

	if (str == "fsrminipad") { return BOARD_FSRMINIPAD; }
	if (str == "teensy2") { return BOARD_TEENSY2; }
	if (str == "leonardo") { return BOARD_LEONARDO; }
	
	return BOARD_UNKNOWN;
}

bool BoardTypeStruct::CompatibleWith(BoardTypeStruct other, bool strict) const
{
	ArchType compArchType = other.archType;
	if(compArchType == ARCH_AVR_ARD)
		compArchType = ARCH_AVR;

	if (archType != compArchType)
		return false;

	if (strict && boardType != other.boardType)
		return false;

	return true;
}

std::string BoardTypeStruct::ToString() const
{
	std::string ret = "";

	switch (archType)
	{
		case ARCH_AVR:
		case ARCH_AVR_ARD:
			ret += "avr_";
			break;
		case ARCH_ESP:
			ret += "esp_";
			break;
	}

	switch (boardType)
	{
		case BOARD_FSRIO_V1:
			ret += "fsrio1";
			break;
		case BOARD_FSRIO_V2:
			ret += "fsriov2";
			break;
		case BOARD_FSRIO_V3:
			ret += "fsriov3";
			break;
		case BOARD_FSRMINIPAD:
			ret += "fsrminipad";
			break;
		default:
			ret += "unknown";
	}

	return ret;
}

// FirmwarePackage

class FirmwarePackageImpl : public FirmwarePackage
{
public:
	FirmwarePackageImpl(std::string fileName)
	:zipFile(fileName), fileName(fileName)
	{
		if(!zipFile.open(libzippp::ZipArchive::ReadOnly))
			throw std::runtime_error("Could not open firmware file.");

		auto btEntry = zipFile.getEntry("boardtype.txt");
		if(btEntry.isNull())
			throw std::runtime_error("Could not read boardtype.txt");
		
		btEntry.readAsText();
		std::stringstream btBuffer;
		btEntry.readContent(btBuffer);
		std::string boardTypeStr = btBuffer.str();
		boardType = BoardTypeStruct(boardTypeStr);
		if(boardType.archType == ARCH_UNKNOWN || boardType.boardType == BOARD_UNKNOWN)
			throw std::runtime_error("Invalid board type.");
	}

	bool ReadFile(std::string fileName, std::vector<uint8_t>& buffer) override
	{
		auto zippedFile = zipFile.getEntry(fileName);
		if(zippedFile.isNull())
			return false;
		zippedFile.readAsBinary();
		std::stringstream fileBuffer;
		zippedFile.readContent(fileBuffer);
		fileBuffer.seekg(0, std::ios::end);
		buffer.resize(fileBuffer.tellp());
		memcpy(buffer.data(), fileBuffer.str().c_str(), buffer.size());
		return true;
	}

	BoardTypeStruct GetBoardType() override
	{
		return boardType;
	}

	std::string GetFileName() override
	{
		return fileName;
	}

protected:
	BoardTypeStruct boardType;
	std::string fileName;
	libzippp::ZipArchive zipFile;
};

FirmwarePackagePtr FirmwarePackageRead(std::string fileName)
{
	return std::make_shared<FirmwarePackageImpl>(fileName);
}

// FirmwareUploader

bool FlashEsp32(PortInfo& port, FirmwarePackagePtr firmware, FirmwareCallback callback);
bool FlashAvr(PortInfo& port, FirmwarePackagePtr firmware, FirmwareCallback callback, bool raw = false);

FlashResult FirmwareUploader::WriteFirmware(FirmwarePackagePtr package)
{
	if(flashResult != FLASHRESULT_CONNECTED)
	{
		errorMessage = "Not connected to a device";
		return FLASHRESULT_FAILURE;
	}

	if(!package->GetBoardType().CompatibleWith(connectedBoardType, !this->ignoreBoardType))
	{
		errorMessage = "Firmware is not compatible with the connected device";
		Log::Writef("Package type: %s - Connected type: %s", package->GetBoardType().ToString().c_str(), connectedBoardType.ToString().c_str());
		return FLASHRESULT_FAILURE;
	}

	Device::SetSearching(false);
	flashResult = FLASHRESULT_RUNNING;

	FirmwareCallback callback = [this](FlashResult event, std::string message, int progress)
	{
		if(event == FLASHRESULT_SUCCESS)
		{
			flashResult = FLASHRESULT_SUCCESS;
		}
		// else if(event == FLASHRESULT_FAILURE)
		// {
		// 	flashResult = FLASHRESULT_FAILURE;
		// 	errorMessage = message;
		// }
		else if(event == FLASHRESULT_PROGRESS)
		{
			this->progress = progress;
		}
	};

	bool result = false;

	if(connectedBoardType.archType == ARCH_ESP)
	{
		result = FlashEsp32(comPort, package, callback);
	}
	else if(connectedBoardType.archType == ARCH_AVR)
	{
		result = FlashAvr(comPort, package, callback);
	}
	else if(connectedBoardType.archType == ARCH_AVR_ARD)
	{
		result = FlashAvr(comPort, package, callback, true);
	}
	else
	{
		errorMessage = "Unsupported architecture";
		return FLASHRESULT_FAILURE;
	}

	if(!result)
	{
		errorMessage = "Could not flash firmware";
		return FLASHRESULT_FAILURE;
	}

	return FLASHRESULT_SUCCESS;
}


FlashResult FirmwareUploader::WriteFirmwareThreaded(FirmwarePackagePtr package)
{
	thread = std::thread([this, package]()
	{
		flashResult = WriteFirmware(package);
		WritingDone(0);
	});

	return FLASHRESULT_RUNNING;
}

FlashResult FirmwareUploader::ConnectDevice()
{
	flashResult = FLASHRESULT_NOTHING;

	errorMessage = "";

	auto pad = Device::Pad();
	if (pad == NULL) {
		errorMessage = "No compatible board connected";
		flashResult = FLASHRESULT_FAILURE;
		return flashResult;
	}

	connectedBoardType = pad->boardType;

	vector<PortInfo> oldPorts = list_ports();
	bool foundNewPort = false;
	auto startTime = system_clock::now();

	configBackup = json();
	Device::SaveProfile(configBackup, DeviceProfileGroupFlags::DGP_ALL);
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

	this_thread::sleep_for(500ms);

	flashResult = FLASHRESULT_CONNECTED;
	return flashResult;
}

bool FirmwareUploader::SetPort(std::string portName)
{
	this->comPort.port = portName;
	flashResult = FLASHRESULT_CONNECTED;
	return true;
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