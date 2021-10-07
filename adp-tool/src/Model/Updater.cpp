#include "Adp.h"

#include <stdio.h>

#include "wx/string.h"
#include "wx/webrequest.h"
#include "wx/stdstream.h"
#include "wx/event.h"
#include <wx/regex.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Model/Updater.h"
#include "Model/Firmware.h"
#include "Model/Log.h"

using namespace std;

namespace adp {

static wxEvtHandler eventHandler;

void Updater::Init()
{
	CheckForUpdates();
}

versionType Updater::AdpVersion()
{
	return { ADP_VERSION_MAJOR, ADP_VERSION_MINOR };
}

versionType Updater::ParseString(string input)
{
	wxRegEx re = "v([0-9]+).([0-9]+)";
	bool v = re.IsValid();
	bool v2 = re.Matches(input);


	if (re.Matches(input)) {
		uint16_t major, minor;

		sscanf(re.GetMatch(input, 1), "%hi", &major);
		sscanf(re.GetMatch(input, 2), "%hi", &minor);

		return {
			major, minor
		};
	}
	else {

	}

	return versionTypeUnknown;
}

bool Updater::IsNewer(versionType current, versionType check)
{
	if (check.major > current.major) {
		return true;
	}

	if (check.major == current.major && check.minor > current.minor) {
		return true;
	}

	return false;
}

void Updater::CheckForUpdates()
{
	auto currentVersion = Updater::AdpVersion();

	// Create the request object
	wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
		&eventHandler,
		"https://api.github.com/repos/electromuis/analog-dance-pad/releases"
	);
	request.SetHeader("Accept", "application/vnd.github.v3+json");
	request.SetHeader("User-Agent", "adp-tool");

	if (!request.IsOk()) {
		// This is not expected, but handle the error somehow.
		Log::Write(L"Request not OK");
	}
	// Bind state event
	eventHandler.Bind(wxEVT_WEBREQUEST_STATE, [currentVersion](wxWebRequestEvent& evt) {
		switch (evt.GetState())
		{
			// Request completed
		case wxWebRequest::State_Completed:
		{
			Log::Write(L"Request done");

			json j;
			wxStdInputStream in(*evt.GetResponse().GetStream());
			in >> j;
			if (!j.is_array()) {
				Log::Write(L"Request output invalid");
				break;
			}
			else {
				Log::Writef(L"Release count: %i", j.size());

				
				for (auto& release : j) {

					for (auto& asset : release["assets"]) {

						if (asset["name"] == "adp-tool.exe") {

							auto version = Updater::ParseString(release["tag_name"]);
							if (Updater::IsNewer(currentVersion, version)) {
								Log::Writef(L"Update available: v%i.%i", version.major, version.minor);
								return;
							}

						}
					}
				}
				
			}

			break;
		}
		// Request failed
		case wxWebRequest::State_Failed:
			Log::Writef(L"Could not load request: %s", evt.GetErrorDescription());
			break;
		}
		});
	// Start the request
	request.Start();
}

}; // namespace adp.