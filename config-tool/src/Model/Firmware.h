#pragma once

#include "stdint.h"
#include <string>

using namespace std;

namespace adp {

enum BoardType {
	BOARD_UNKNOWN,
	BOARD_FSRMINIPAD
};

bool UploadFirmware(wstring fileName);
enum BoardType ParseBoardType(const std::string& str);

}; // namespace adp.
