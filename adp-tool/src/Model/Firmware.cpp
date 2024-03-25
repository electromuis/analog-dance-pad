#include <Adp.h>

#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>
#include <fmt/core.h>

#include "serial_port.h"
#include "esp_loader.h"
#include "esp_loader_io.h"

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
	if (str == "fsriov2") { return BOARD_LEONARDO; }
	if (str == "FSRio V3") { return BOARD_FSRIO_V3; }
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

	if(boardType == BOARD_FSRIO_V3) {
		return firmwareFile ? "FSRioV3" : "FSRio V3";
	}

	return "Unknown";
}

const char* BoardTypeToString(BoardType boardType)
{
	return BoardTypeToString(boardType, false);
}


FlashResult FirmwareUploader::UpdateFirmware(string fileName)
{
	this->firmwareFile = fileName;
	return FLASHRESULT_NOTHING;
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

	BoardType boardType = BOARD_UNKNOWN;

	// ifstream fileStream;
	// string fileNameThin(fileName.begin(), fileName.end());
	// fileStream.open(fileNameThin);

	// if (!fileStream.is_open()) {
	// 	errorMessage = "Could not read firmware file";
	// 	flashResult = FLASHRESULT_FAILURE;
	// 	return flashResult;
	// }

	// string line;
	// while (getline(fileStream, line)) {
	// 	if (line.size() > 0 && line.substr(0, 1) == ";") {
	// 		line = line.substr(1);
	// 		if (line.substr(line.length() - 1, line.length()) == "\r") {
	// 			line = line.substr(0, line.length() -1);
	// 		}

	// 		BoardType foundType = ParseBoardType(line);
	// 		if (foundType != BOARD_UNKNOWN) {
	// 			boardType = foundType;
	// 			break;
	// 		}
	// 	}
	// }
	// fileStream.close();

	if (boardType == BoardType::BOARD_UNKNOWN || pad->boardType != boardType) {
		if (!ignoreBoardType) {
			errorMessage = fmt::format("Selected: %s, connected: %s", BoardTypeToString(boardType), BoardTypeToString(pad->boardType));
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

	return FLASHRESULT_CONNECTED;
	// return WriteFirmwareAvr();
}

bool FirmwareUploader::SetPort(std::string portName)
{
	this->comPort.port = portName;
	return true;
}

FlashResult FirmwareUploader::WriteFirmwareAvr(string fileName)
{
	this->firmwareFile = fileName;
	AvrDude avrdude;

	auto comPort = this->comPort.port;
	auto firmwareFile = this->firmwareFile;
	auto eventHandler = callback;

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
				"-U", fmt::format("flash:w:1:%s:i", firmwareFile)
			} };

			this->myAvrdude->push_args(std::move(args));

			if (eventHandler) {
				eventHandler(AE_START, "", -1);
			}
		})
		.on_message([eventHandler](const char* msg, unsigned size) {
			std::string wxmsg = msg;
			Log::Writef("avrdude: %s", wxmsg);

			if (eventHandler) {
				eventHandler(AE_MESSAGE, wxmsg, -1);
			}
		})
		.on_progress([eventHandler](const char* task, unsigned progress) {
			std::string wxmsg = task;

			if (eventHandler) {
				eventHandler(AE_PROGRESS, wxmsg, progress);
			}
		})
		.on_complete([eventHandler, this]() {
			Log::Write("avrdude done");

			int exitCode = this->myAvrdude->exit_code();
			this->WritingDone(exitCode);

			if (eventHandler) {
				eventHandler(AE_EXIT, "", exitCode);
			}
		});


	// Wait a bit since the COM port might still be initializing
	this_thread::sleep_for(500ms);

	Device::SetSearching(false);
	avrdude.run();

	flashResult = FLASHRESULT_RUNNING;
	return flashResult;
}

esp_loader_error_t connect_to_target(uint32_t higher_transmission_rate)
{
    esp_loader_connect_args_t connect_config = {
		.sync_timeout = 200, 
		.trials = 20,
	};

    esp_loader_error_t err = esp_loader_connect(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        Log::Writef("Cannot connect to target. Error: %u\n", err);
        return err;
    }

    Log::Write("Connected to target\n");

	err = esp_loader_change_transmission_rate(higher_transmission_rate);
	if (err != ESP_LOADER_SUCCESS) {
		Log::Write("Unable to change transmission rate on target.");
		return err;
	} else {
		err = loader_port_change_transmission_rate(higher_transmission_rate);
		if (err != ESP_LOADER_SUCCESS) {
			Log::Write("Unable to change transmission rate.");
			return err;
		}
		Log::Write("Transmission rate changed changed\n");
	}
    

    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address)
{
    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = bin;

    Log::Write("Erasing flash (this may take a while)...\n");
    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        Log::Writef("Erasing flash failed with error %d.\n", err);
        return err;
    }
    Log::Write("Start programming\n");

    size_t binary_size = size;
    size_t written = 0;

    while (size > 0) {
        size_t to_read = std::min(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);

        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            Log::Writef("\nPacket could not be written! Error %d.\n", err);
            return err;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        Log::Writef("\rProgress: %d %%", progress);
        fflush(stdout);
    };

    Log::Write("\nFinished programming\n");

    err = esp_loader_flash_verify();
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
        return err;
    }
    Log::Write("Flash verified\n");

    return ESP_LOADER_SUCCESS;
}

struct EspFlashCommand {
	std::string fileName;
	uint32_t address;
};

FlashResult FirmwareUploader::WriteFirmwareEsp(string fileName)
{
	this->firmwareFile = fileName;
	loader_serial_config_t config;
    config.portName = (char*)this->comPort.port.c_str();
    config.baudrate = 19200;
    config.timeout = 600;

	if (loader_port_serial_init(&config) != ESP_LOADER_SUCCESS) {
		Log::Write("Serial initialization failed.");
		return FlashResult::FLASHRESULT_FAILURE;
	}

	if (connect_to_target(460800) != ESP_LOADER_SUCCESS) {
		Log::Write("Connect to target failed.");
		return FlashResult::FLASHRESULT_FAILURE;
	}

	std::vector<EspFlashCommand> flashCommands = {
		{this->firmwareFile + "\\bootloader.bin", 0x0},
		{this->firmwareFile + "\\partitions.bin", 0x8000},
		{this->firmwareFile + "\\boot_app0.bin", 0xe000},
		{this->firmwareFile + "\\firmware.bin", 0x10000},
	};

	for (EspFlashCommand command : flashCommands) {
		std::ifstream file(command.fileName, std::ios::binary);
		if (!file.is_open()) {
			if(command.fileName == "\\firmware.bin")
			{
				Log::Writef("Could not read firmware file: %s", command.fileName.c_str());
				return FlashResult::FLASHRESULT_FAILURE;
			}
			else
				continue;
		}
		std::vector<uint8_t> fileBuffer(std::istreambuf_iterator<char>(file), {});
		uint8_t *fileData = fileBuffer.data();

		Log::Writef("Writing EPS flash: %s", command.fileName.c_str());
		if(flash_binary(fileData, fileBuffer.size(), command.address) != ESP_LOADER_SUCCESS) {
			Log::Write("flash binary failed.");
			return FlashResult::FLASHRESULT_FAILURE;
		}
	}

	return FlashResult::FLASHRESULT_SUCCESS;
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