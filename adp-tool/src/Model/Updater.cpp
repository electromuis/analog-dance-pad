#include "Adp.h"

#include <stdio.h>

#include "wx/setup.h"
#if wxUSE_WEBREQUEST_WINHTTP
	#include "wx/msw/wrapwin.h"
	#include <winhttp.h>
#endif

#include "wx/string.h"
#include "wx/webrequest.h"
#include "wx/stdstream.h"
#include "wx/event.h"
#include <wx/regex.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/filefn.h>

#include "Model/Updater.h"
#include "Model/Firmware.h"
#include "Model/Log.h"

using namespace std;

namespace adp {

static wxWebSession* webSession;
static wxEvtHandler eventHandler;
static SoftwareUpdate adpUpdate(versionTypeUnknown, SW_TYPE_ADP_OTHER, BOARD_UNKNOWN, "", "");

bool SoftwareUpdate::Install()
{
	if (softwareType == SW_TYPE_ADP_TOOL) {
		wxString appPath(wxStandardPaths::Get().GetExecutablePath());

		wxWebRequest request = webSession->CreateRequest(
			&eventHandler,
			downloadUrl
		);
		request.SetHeader("Accept", "application/octet-stream");

		if (!request.IsOk()) {
			// This is not expected, but handle the error somehow.
			Log::Write(L"Request not OK");
			return false;
		}
		// Bind state event
		eventHandler.Bind(wxEVT_WEBREQUEST_STATE, [appPath, request](wxWebRequestEvent& evt) {
			switch (evt.GetState())
			{

			// wx winhttp sets the accept headet to */* internally, which does not let us download. Replace the header just in time to fix this
			#if wxUSE_WEBREQUEST_WINHTTP
			case wxWebRequest::State_Active:
			{
				HINTERNET handle = (HINTERNET)request.GetNativeHandle();
				WinHttpAddRequestHeaders(handle, L"Accept: application/octet-stream", (ULONG)-1L, WINHTTP_ADDREQ_FLAG_REPLACE);

				break;
			}
			#endif

				// Request completed
			case wxWebRequest::State_Completed:
			{
				wxString url = evt.GetResponse().GetURL();

				wxTempFileOutputStream out(appPath);
				out.Write(*evt.GetResponse().GetStream());
				::wxRenameFile(appPath, appPath + ".old");
				out.Commit();
				Log::Write(L"Update downloaded");
				return;
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

	return false;
}

void Updater::Init()
{
	webSession = &wxWebSession::GetDefault();
	webSession->AddCommonHeader("User-Agent", ADP_USER_AGENT);
}

void Updater::Shutdown()
{
	delete webSession;
}

VersionType Updater::AdpVersion()
{
	return { ADP_VERSION_MAJOR, ADP_VERSION_MINOR };
}

VersionType Updater::ParseString(string input)
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

	return versionTypeUnknown;
}

bool Updater::IsNewer(VersionType current, VersionType check)
{
	if (check.major > current.major) {
		return true;
	}

	if (check.major == current.major && check.minor > current.minor) {
		return true;
	}

	return false;
}

void Updater::CheckForUpdates(void (*updateFoundCallback)(SoftwareUpdate&))
{
	auto currentVersion = Updater::AdpVersion();

	// Create the request object
	wxWebRequest request = webSession->CreateRequest(
		&eventHandler,
		"https://api.github.com/repos/electromuis/analog-dance-pad/releases"
	);
	request.SetHeader("Accept", "application/vnd.github.v3+json");

	if (!request.IsOk()) {
		// This is not expected, but handle the error somehow.
		Log::Write(L"Request not OK");
		return;
	}
	// Bind state event
	eventHandler.Bind(wxEVT_WEBREQUEST_STATE, [updateFoundCallback, currentVersion](wxWebRequestEvent& evt) {
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

				for (auto& release : j) 
				for (auto& asset : release["assets"]) 
				if (asset["name"] == "adp-tool.exe") {

					auto version = Updater::ParseString(release["tag_name"]);
					if (Updater::IsNewer(currentVersion, version)) {
						adpUpdate = SoftwareUpdate(version, SW_TYPE_ADP_TOOL, BOARD_UNKNOWN, asset["url"], asset["name"]);
						Log::Writef(L"Update available: v%i.%i", version.major, version.minor);
						(*updateFoundCallback)(adpUpdate);
						//adpUpdate.Install();

						return;
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