#pragma once

#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Model/Firmware.h"

using namespace std;

namespace adp {

#define ADP_USER_AGENT "adp-tool"

struct VersionType {
	uint16_t major;
	uint16_t minor;
};

enum SoftwareType {
	SW_TYPE_ADP_TOOL,
	SW_TYPE_ADP_FIRMWARE,
	SW_TYPE_ADP_OTHER
};

static const VersionType versionTypeUnknown = { 0, 0 };

class SoftwareUpdate
{
public:
	SoftwareUpdate(
		VersionType version,
		SoftwareType softwareType,
		BoardType boardType,
		string downloadUrl,
		string binaryName)
		:version(version),
		softwareType(softwareType),
		boardType(boardType),
		downloadUrl(downloadUrl),
		binaryName(binaryName)
	{}

	bool Install();
	VersionType GetVersion() { return version; };

private:
	VersionType version;
	SoftwareType softwareType;
	BoardType boardType;
	string downloadUrl;
	string binaryName;
};

class Updater
{
public:
	static void Init();
	static void Shutdown();
	static void CheckForUpdates(void (*updateFoundCallback)(SoftwareUpdate&));

	static VersionType AdpVersion();
	static VersionType ParseString(string input);
	static bool IsNewer(VersionType current, VersionType check);
};

}; // namespace adp.
