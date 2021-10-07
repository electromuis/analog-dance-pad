#pragma once

#include <string>

using namespace std;

namespace adp {

struct versionType {
	uint16_t major;
	uint16_t minor;
};

static const versionType versionTypeUnknown = { 0, 0 };

class Updater
{
public:
	static void Init();
	static void CheckForUpdates();
	static versionType AdpVersion();
	static versionType ParseString(string input);
	static bool IsNewer(versionType current, versionType check);


};

}; // namespace adp.
