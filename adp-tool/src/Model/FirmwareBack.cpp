#pragma once

#include "Log.h"
#include "Firmware.h"
#include "avrdude-slic3r.hpp"
#include "serial/serial.h"

#include "wjwwood_serial_port.h"
#include "esp_loader.h"
#include "esp_loader_io.h"

#include <fmt/core.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace Slic3r;
using namespace serial;

namespace adp {

// ESP

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

esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address, uint32_t& sizeWritten, FirmwareCallback cb)
{
    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = bin;

    Log::Write("Erasing flash (this may take a while)...\n");
	cb(FLASHRESULT_MESSAGE, "Erasing ...", 0);

    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        Log::Writef("Erasing flash failed with error %d.\n", err);
        return err;
    }
    Log::Write("Start programming\n");
	cb(FLASHRESULT_MESSAGE, "Programming ...", 0);

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
		sizeWritten += to_read;

		cb(FLASHRESULT_PROGRESS, "", sizeWritten);
        // Log::Writef("\rProgress: %d %%", progress);
        // fflush(stdout);
    };

    Log::Write("\nFinished programming\n");
	cb(FLASHRESULT_MESSAGE, "Verrifying ...", 0);

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
	std::vector<uint8_t> fileData;
};

bool FirmwareUploader::FlashEsp32()
{
	loader_wjwwood_serial_config_t config;
    config.portName = (char*)comPort.c_str();
    config.baudrate = 19200;
    config.timeout = 600;

	if (loader_port_wjwwood_serial_init(&config) != ESP_LOADER_SUCCESS) {
		Log::Write("Serial initialization failed.");
		return FlashResult::FLASHRESULT_FAILURE;
	}

	if (connect_to_target(460800) != ESP_LOADER_SUCCESS) {
		Log::Write("Connect to target failed.");
		return FlashResult::FLASHRESULT_FAILURE;
	}

	std::vector<EspFlashCommand> flashCommands = {
		{"bootloader.bin", 0x0},
		{"partitions.bin", 0x8000},
		{"boot_app0.bin", 0xe000},
		{"firmware.bin", 0x10000},
	};

	size_t totalSize = 0;
	for (EspFlashCommand& command : flashCommands) {
		if (!firmware->ReadFile(command.fileName, command.fileData)) {
			if(command.fileName == "firmware.bin")
			{
				Log::Writef("Could not read firmware file: %s", command.fileName.c_str());
				return false;
			}
			else
				continue;
		}

		totalSize += command.fileData.size();
	}

	maxProgress = totalSize;

	FirmwareCallback callback = [this](FlashResult event, std::string message, int progress)
	{
		if(event == FLASHRESULT_PROGRESS)
		{
			this->progress = progress;
		}

		if(this->callback)
		{
			this->callback(event, message, progress);
		}
	};

	uint32_t sizeWritten = 0;
	for(EspFlashCommand& command : flashCommands) {
		if(command.fileData.size() == 0)
			continue;

		Log::Writef("Writing EPS flash: %s", command.fileName.c_str());
		if(flash_binary(command.fileData.data(), command.fileData.size(), command.address, sizeWritten, callback) != ESP_LOADER_SUCCESS) {
			Log::Write("flash binary failed.");
			return false;
		}
	}

	loader_port_wjwwood_serial_deinit();

	return true;
}

// bool FlashEsp32(PortInfo& port, FirmwarePackagePtr firmware, FirmwareCallback callback)
// {
// 	return false;
// }

 // AVR

bool FirmwareUploader::FlashAvr()
{
    AvrDude avrdude;
	// AvrDude::Ptr avrdude = std::make_shared<AvrDude>();
	char tmpFileName [L_tmpnam];
   	std::tmpnam (tmpFileName);

	// if(raw) {
	// 	Log::Write("Raw mode");
	// }
	
	std::vector<uint8_t> buffer;
	if (!firmware->ReadFile("firmware.hex", buffer)) {
		Log::Write("Could not read firmware file");
		return false;
	}

	std::ofstream output_stream(tmpFileName);
	if(!output_stream) {
		Log::Write("Could not open temporary file");
		return false;
	}
	output_stream << std::string(buffer.begin(), buffer.end());
	output_stream.close();

	int uploadPhase = -1;
	std::string lastTask = "";
	maxProgress = 3*100;

	avrdude
		.on_message([this](const char* msg, unsigned size) {
			Log::Writef("avrdude: %s", msg);

			
		})
		.on_progress([this, &lastTask, &uploadPhase](const char* task, unsigned p) {
			std::string taskStr = task;
		
			if(taskStr != lastTask) {
				Log::Writef("avrdude: %s", task);
				lastTask = taskStr;
				uploadPhase++;

				if(callback) {
					callback(FLASHRESULT_MESSAGE, taskStr, 0);
				}
			}

			progress = (uploadPhase*100)+p;
		});

	std::string isp = "avr109";
	// if(raw)
	// 	isp = "usbasp";

	std::vector<std::string> args{ {
			"-v",
			"-p", "atmega32u4",
			"-c", isp,
			"-P", comPort,
			"-b", "115200",
			"-D",
			"-U", fmt::format("flash:w:1:{}:i", tmpFileName)
		} };

	// if(raw) {
	// 	args.push_back("-U");
	// 	args.push_back("lfuse:w:1:0xff:m");
		
	// 	args.push_back("-U");
	// 	args.push_back("hfuse:w:1:0xd8:m");

	// 	args.push_back("-U");
	// 	args.push_back("efuse:w:1:0xcb:m");
	// }

	avrdude.push_args(std::move(args));

	auto ret = avrdude.run_sync();
	remove(tmpFileName);
	return ret == 0;
}

};