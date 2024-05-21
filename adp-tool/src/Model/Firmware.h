#pragma once

#include "stdint.h"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifndef __EMSCRIPTEN__
	#include "serial/serial.h"

	using namespace serial;
#endif // __EMSCRIPTEN__

namespace adp {
	
enum ArchType {
	ARCH_UNKNOWN,
	ARCH_AVR,
	ARCH_ESP,
	ARCH_AVR_ARD
};

enum BoardType {
	BOARD_UNKNOWN,
	BOARD_FSRMINIPAD,
	BOARD_FSRMINIPAD_V2,
	BOARD_TEENSY2,
	BOARD_LEONARDO,
	BOARD_FSRIO_V1,
	BOARD_FSRIO_V2,
	BOARD_FSRIO_V3,
};

enum FlashResult
{
	FLASHRESULT_NOTHING,
	FLASHRESULT_CONNECTED,
	FLASHRESULT_SUCCESS,
	FLASHRESULT_FAILURE,
	FLASHRESULT_FAILURE_BOARDTYPE,
	FLASHRESULT_RUNNING,
	FLASHRESULT_PROGRESS,
	FLASHRESULT_CANCELLED
};

struct BoardTypeStruct
{
	BoardTypeStruct()
		:archType(ARCH_UNKNOWN), boardType(BOARD_UNKNOWN)
	{

	}

	BoardTypeStruct(std::string boardType);
	BoardTypeStruct(ArchType archType, BoardType boardType)
		:archType(archType), boardType(boardType)
	{

	}

	static ArchType ParseArchType(const std::string& str);
	static BoardType ParseBoardType(const std::string& str);

	bool CompatibleWith(BoardTypeStruct other, bool strict=true) const;

	ArchType archType;
	BoardType boardType;

	std::string ToString() const;
};

class FirmwarePackage
{
public:
	virtual BoardTypeStruct GetBoardType() = 0;
	virtual bool ReadFile(std::string fileName, std::vector<uint8_t>& buffer) = 0;
	virtual std::string GetFileName() = 0;
};

typedef std::shared_ptr<FirmwarePackage> FirmwarePackagePtr;

FirmwarePackagePtr FirmwarePackageRead(std::string fileName);

typedef std::function<void(FlashResult event, std::string message, int progress)> FirmwareCallback;

#ifndef __EMSCRIPTEN__
class FirmwareUploader
{
public:
	FlashResult ConnectDevice();
	FlashResult WriteFirmware(FirmwarePackagePtr package);
	FlashResult WriteFirmwareThreaded(FirmwarePackagePtr package);
	
	bool SetPort(std::string portName);
	void SetIgnoreBoardType(bool ignoreBoardType);
	void SetArchtType(ArchType archType) {connectedBoardType.archType = archType;};
	void WritingDone(int exitCode);
	std::string GetErrorMessage();
	FlashResult GetFlashResult();
	void SetEventHandler(FirmwareCallback callback) {this->callback = callback;};

	int GetProgress() {return progress;};

private:
	std::thread thread;
	int progress = 0;
	BoardTypeStruct connectedBoardType;

	PortInfo comPort;
	std::string errorMessage;
	FlashResult flashResult = FLASHRESULT_NOTHING;
	json configBackup;
	bool ignoreBoardType = false;
	FirmwareCallback callback;
};
#endif //__EMSCRIPTEN__

}; // namespace adp.