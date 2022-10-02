#include <Adp.h>

#include <stdio.h>

#include <Model/Updater.h>
#include <Model/Firmware.h>
#include <Model/Log.h>
#include <Model/Device.h>
#include <Model/Utils.h>

using namespace std;

namespace adp {

static SoftwareUpdate adpUpdate(versionTypeUnknown, SW_TYPE_ADP_OTHER, BOARD_UNKNOWN, "", "");

void SoftwareUpdate::Install(void (*updateInstalledCallback)(bool))
{

#if wxUSE_WEBREQUEST
	if (softwareType == SW_TYPE_ADP_TOOL) {
		Log::Write(L"Installing ADP update");

		wxString appPath(wxStandardPaths::Get().GetExecutablePath());


		wxEvtHandler* myEvtHandler = new wxEvtHandler();
		wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
			myEvtHandler,
			downloadUrl
		);
		request.SetHeader("Accept", "application/octet-stream");

		if (!request.IsOk()) {
			// This is not expected, but handle the error somehow.
			(*updateInstalledCallback)(false);
		}
		// Bind state event
		myEvtHandler->Bind(wxEVT_WEBREQUEST_STATE, [myEvtHandler, updateInstalledCallback, appPath, request](wxWebRequestEvent& evt) {
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

				// Place the old app file to be removed
				::wxRenameFile(appPath, appPath + ".remove");
				out.Commit();
				Log::Write(L"Update installed");
				(*updateInstalledCallback)(true);
				delete myEvtHandler;
				return;
			}
			// Request failed
			case wxWebRequest::State_Failed:
				Log::Writef(L"Downloading update failed: %s", evt.GetErrorDescription());
				(*updateInstalledCallback)(false);
				delete myEvtHandler;
				break;
			}
			});
		// Start the request
		request.Start();
		return;
	}

	if (softwareType == SW_TYPE_ADP_FIRMWARE) {
		if (boardType == BOARD_UNKNOWN) {
			(*updateInstalledCallback)(false);
			return;
		}

		auto pad = Device::Pad();
		if (pad == NULL) {
			(*updateInstalledCallback)(false);
			return;
		}

		Log::Writef(L"Installing %s update", BoardTypeToString(pad->boardType));

		wxEvtHandler* myEvtHandler = new wxEvtHandler();
		wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
			myEvtHandler,
			downloadUrl
		);
		request.SetHeader("Accept", "application/octet-stream");

		if (!request.IsOk()) {
			// This is not expected, but handle the error somehow.
			(*updateInstalledCallback)(false);
		}
		// Bind state event
		myEvtHandler->Bind(wxEVT_WEBREQUEST_STATE, [myEvtHandler, updateInstalledCallback, pad, request](wxWebRequestEvent& evt) {
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

				//out.Write(*evt.GetResponse().GetStream());

				Log::Write(L"Update installed ...");
				(*updateInstalledCallback)(true);
				delete myEvtHandler;
				return;
			}
			// Request failed
			case wxWebRequest::State_Failed:
				Log::Writef("Downloading update failed: %ls", evt.GetErrorDescription());
				(*updateInstalledCallback)(false);
				delete myEvtHandler;
				break;
			}
			});
		// Start the request
		request.Start();
		return;
	}

#endif
	(*updateInstalledCallback)(false);
	return;
}

void Updater::Init()
{
}

void Updater::Shutdown()
{

}

VersionType Updater::AdpVersion()
{
	return { ADP_VERSION_MAJOR, ADP_VERSION_MINOR };
}

VersionType Updater::ParseString(string input)
{
	return versionTypeUnknown;
}

void Updater::CheckForAdpUpdates(void (*updateFoundCallback)(SoftwareUpdate&))
{
}

void Updater::CheckForFirmwareUpdates(void (*updateFoundCallback)(SoftwareUpdate&))
{
	auto pad = Device::Pad();
	if (!pad) {
		return;
	}

	if (pad->boardType == BOARD_UNKNOWN) {
		Log::Write("Unknown board, can't search updates");
		return;
	}

	auto currentVersion = pad->firmwareVersion;

#if wxUSE_WEBREQUEST
	wxEvtHandler* myEventHandler = new wxEvtHandler();

	// Create the request object
	wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
		myEventHandler,
		"https://api.github.com/repos/electromuis/analog-dance-pad/releases"
	);
	request.SetHeader("Accept", "application/vnd.github.v3+json");

	Log::Writef(L"Finding %s updates ...", BoardTypeToString(pad->boardType));

	if (!request.IsOk()) {
		// This is not expected, but handle the error somehow.
		return;
	}


	myEventHandler->Bind(wxEVT_WEBREQUEST_STATE, [myEventHandler, pad, updateFoundCallback, currentVersion](wxWebRequestEvent& evt) {
		switch (evt.GetState())
		{
			// Request completed
		case wxWebRequest::State_Completed:
		{
			json j;
			wxStdInputStream in(*evt.GetResponse().GetStream());
			in >> j;

			wxRegEx re = wxRegEx(wxString::Format("%s-v([0-9]+).([0-9]+).hex", BoardTypeToString(pad->boardType, true)));

			if (!j.is_array()) {
				break;
			}
			else {

				for (auto& release : j)
					for (auto& asset : release["assets"]) {
						if (re.Matches((string)asset["name"])) {
							auto version = Updater::ParseString(asset["name"]);
							if (version.IsNewer(currentVersion)) {
								adpUpdate = SoftwareUpdate(version, SW_TYPE_ADP_FIRMWARE, pad->boardType, asset["url"], asset["name"]);
								Log::Writef(L"%s, update available: v%i.%i", BoardTypeToString(pad->boardType), version.major, version.minor);
								(*updateFoundCallback)(adpUpdate);
								delete myEventHandler;
								return;
							}
						}
					}
			}

			Log::Write(L"No new version found");

			break;
		}
		// Request failed
		case wxWebRequest::State_Failed:
			Log::Writef(L"Finding updates failed: %ls", evt.GetErrorDescription());
			delete myEventHandler;
			break;
		}
		});
	// Start the request
	request.Start();
#endif
}

}; // namespace adp.
