#include <Adp.h>

#define _HAS_STD_BYTE 0

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <ctime>

#include <Model/Device.h>
#include <Model/Log.h>
#include <Model/Utils.h>

#include <View/Application.h>
#include <View/Image.h>
#include <View/DeviceTab.h>
#include <View/LightsTab.h>
#include <View/MappingTab.h>
#include <View/SensitivityTab.h>
#include <View/UserConfig.h>

#ifndef __EMSCRIPTEN__
	#include <nfd.h>
#else
	#include <emscripten.h>
	#include <emscripten/val.h>

	EM_JS(void, ScanDevices, (), {
		navigator.hid.requestDevice({filters: []});
	});
#endif

#include <fmt/core.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;
typedef std::chrono::steady_clock Clock;

namespace adp {

static const char* TOOL_NAME = "ADP Tool";

class AdpApplication : public Walnut::Application
{
public:
	AdpApplication();

	void MenuCallback() override;
	void RenderCallback() override;

	void RenderIdleTab();
	void RenderAboutTab();
	void RenderLogTab();

#ifndef __EMSCRIPTEN__
	void LoadProfile();
	void SaveProfile();
#endif

private:
	string myLastProfile;
	DeviceTab myDeviceTab;
	LightsTab myLightsTab;
	MappingTab myMappingTab;
	SensitivityTab mySensitivityTab;
	Clock::time_point myLastUpdateTime;
};

AdpApplication::AdpApplication()
	: Walnut::Application(TOOL_NAME)
{
}

#ifndef __EMSCRIPTEN__
void AdpApplication::LoadProfile()
{
	if (!Device::Pad()) {
		return;
	}

	nfdchar_t* rawOutPath = nullptr;
	const nfdchar_t* defaultPath = myLastProfile.empty() ? nullptr : myLastProfile.data();
	auto result = NFD_OpenDialog("json", nullptr, &rawOutPath);
	if (result != NFD_OKAY)
		return;
		
	string outPath(rawOutPath ? rawOutPath : "");
	free(rawOutPath);

	ifstream fileStream;
	fileStream.open(outPath);

	if (!fileStream.is_open())
	{
		Log::Writef("Could not read profile: %s", outPath.data());
		return;
	}

	try {
		myLastProfile = outPath;

		json j;

		fileStream >> j;
		fileStream.close();

		Device::LoadProfile(j, DGP_ALL);
	}
	catch (exception e) {
		Log::Writef("Could not read profile: %s", e.what());
	}
}

void AdpApplication::SaveProfile()
{
	if (!Device::Pad()) {
		return;
	}

	string path;
	nfdchar_t* rawOutPath = NULL;
	const nfdchar_t* defaultPath = myLastProfile.empty() ? nullptr : myLastProfile.data();
	auto result = NFD_SaveDialog("json", defaultPath, &rawOutPath);
	if (result != NFD_OKAY)
		return;

	string outPath(rawOutPath);
	if(outPath.find(".") == string::npos)
		outPath += ".json";

	free(rawOutPath);

	ofstream output_stream(outPath);
	if (!output_stream)
	{
		Log::Writef("Could not save profile: %s", outPath.data());
		return;
	}

	try {
		myLastProfile = outPath;

		json j;

		Device::SaveProfile(j, DGP_ALL);

		output_stream << j.dump(4);
		output_stream.close();
	}
	catch (exception e) {
		Log::Writef("Could not save profile: %s", e.what());
	}
}
#endif// __EMSCRIPTEN__

void AdpApplication::MenuCallback()
{
	if (ImGui::BeginMenu("File"))
	{
#ifndef __EMSCRIPTEN__
		bool saveEnabled = Device::Pad() != nullptr;
		if(!saveEnabled)
			ImGui::BeginDisabled();

		if (ImGui::MenuItem("Load profile"))
		{
			LoadProfile();
		}

		if (ImGui::MenuItem("Save profile"))
			SaveProfile();

		if(!saveEnabled)
			ImGui::EndDisabled();

		if (ImGui::MenuItem("Exit"))
			Close();
#endif

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Device"))
	{
		#ifdef __EMSCRIPTEN__
			if (ImGui::MenuItem("Scan"))
				ScanDevices();
		#endif

		// if (ImGui::MenuItem("Disconnect"))
		// 	Device::Disconnect();

#ifdef DEVICE_CLIENT_ENABLED
		static char addrBuffer[64] = "ws://fsrio.local/ws";
		static bool failed = false;

		

		ImGui::InputText("Device address", addrBuffer, 64);
		ImGui::SameLine();
		if (ImGui::Button("Connect")) {
			failed = !Device::Connect(std::string(addrBuffer));
		}
		
#endif

		if (failed) {
			ImGui::Text("Connecting failed");
		}

		static bool scanEnabled = true;
		if (ImGui::MenuItem("Enable scan", nullptr, &scanEnabled))
			Device::SetSearching(scanEnabled);

		if(Device::DeviceNumber() > 0)
		{
			ImGui::Separator();
			for (int i = 0; i < Device::DeviceNumber(); ++i)
			{
				auto name = Device::GetDeviceName(i);
				if (ImGui::RadioButton(name.data(), Device::DeviceSelected() == i)) {
					failed = !Device::DeviceSelect(i);
					if (!failed)
					{
						mySensitivityTab.OnDeviceChanged();
					}

				}
			}
		}
		else
		{
			ImGui::MenuItem("No devices found", nullptr, nullptr, false);
		}

		if(std::getenv("ADP_ADVANCED"))
		{
			ImGui::Separator();
			if(ImGui::MenuItem("Flash device"))
			{
				myDeviceTab.OpenFlashDialog(true);
			}
		}

		ImGui::EndMenu();
	}
}

static void RenderTab(const function<void()>& render, const char* name)
{
	if (ImGui::BeginTabItem(name))
	{
		auto id = fmt::format("{}Content", name);
		ImGui::BeginChild(id.data(), ImVec2(0, 0), false,
			ImGuiWindowFlags_AlwaysUseWindowPadding |
			ImGuiWindowFlags_HorizontalScrollbar);
		render();
		ImGui::EndChild();
		ImGui::EndTabItem();
	}
}

void AdpApplication::RenderCallback()
{
	auto now = Clock::now();

#ifdef __EMSCRIPTEN__
	if (now - myLastUpdateTime > 1000ms)
#else
	if (now - myLastUpdateTime > 10ms)
#endif
	{
		Device::Update();
		myLastUpdateTime = now;
	}

	float statusH = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("StatusRegion", ImVec2(0, -statusH), false, ImGuiWindowFlags_None);
	ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton);
	
	auto pad = Device::Pad();
	if (pad)
	{
		RenderTab(bind(&SensitivityTab::Render, &mySensitivityTab), "Sensitivity");
		RenderTab(bind(&MappingTab::Render, &myMappingTab), "Mapping");
		RenderTab(bind(&DeviceTab::Render, &myDeviceTab), "Device");
		RenderTab(bind(&LightsTab::Render, &myLightsTab), "Lights");
	}
	else
	{
		RenderTab(bind(&AdpApplication::RenderIdleTab, this), "Idle");
	}
	RenderTab(bind(&AdpApplication::RenderAboutTab, this), "About");
	RenderTab(bind(&AdpApplication::RenderLogTab, this), "Log");

	myDeviceTab.RenderFlash();
	ImGui::EndTabBar();
	ImGui::EndChild();

	if (pad)
	{
		auto wp = ImGui::GetWindowPos();
		auto ws = ImGui::GetWindowSize();
		auto wdl = ImGui::GetWindowDrawList();
		wdl->AddRectFilled(
			{ wp.x, wp.y + ws.y - statusH },
			{ wp.x + ws.x, wp.y + ws.y },
			ImGui::GetColorU32(ImGuiCol_FrameBg, 0.25f));
		ImGui::SetCursorPos({ 6, ws.y - statusH + 6 });
		ImGui::Text("Connected to: %s", pad->name.data());

		auto rate = Device::PollingRate();
		if (rate > 0)
		{
			string str = fmt::format("{}Hz", rate);
			auto ts = ImGui::CalcTextSize(str.data());
			ImGui::SetCursorPos({ ws.x - ts.x - 6, ws.y - statusH + 6 });
			ImGui::TextUnformatted(str.data());
		}
	}
};

void AdpApplication::RenderIdleTab()
{
	const char* text = "Connect an ADP enabled device to continue.";
	auto ws = ImGui::GetWindowSize();
	auto ts = ImGui::CalcTextSize(text);
	ImGui::SetCursorPos(ImVec2(ws.x/2 - ts.x/2, ws.y/2 - ts.y/2));
	ImGui::TextUnformatted(text);

	#ifdef __EMSCRIPTEN__
	if(ImGui::Button("Device select"))
		ScanDevices();
	#endif
}

void AdpApplication::RenderAboutTab()
{
	auto ws = ImGui::GetWindowSize();
	const char* lines[] =
	{
		"ADP Tool " ADP_VERSION_STR,
		"(c) Bram van de Wetering 2022",
		"(c) DDR-EXP 2024",
	};
	ImGui::SetCursorPosY(round(ws.y / 2 - 80));
	ImGui::SetCursorPosX(round(ws.x / 2 - 32));
	
	Walnut::Image& icon = *m_Icon64;
	auto size = ImVec2(float(icon.GetWidth()), float(icon.GetHeight()));
	ImGui::Image(icon.GetDescriptorSet(), size);
	
	ImGui::SetCursorPosY(round(ws.y / 2));
	for (int i = 0; i < std::size(lines); ++i)
	{
		auto ts = ImGui::CalcTextSize(lines[i]);
		ImGui::SetCursorPosX(ws.x / 2 - ts.x / 2);
		ImGui::TextUnformatted(lines[i]);
	}
}

void AdpApplication::RenderLogTab()
{
	for (int i = 0; i < Log::NumMessages(); ++i)
		ImGui::TextUnformatted(Log::Message(i).data());
}

AdpApplication app;

void loop()
{
	app.Loop();
}

int Main(int argc, char** argv)
{
	Log::Init();

	auto versionString = fmt::format("{} {}.{}", TOOL_NAME, ADP_VERSION_MAJOR, ADP_VERSION_MINOR);
	auto now = std::time(0);
	std::string datetime = std::ctime(&now);
	Log::Writef("Application started: %s", versionString.data());
	Log::Writef("Starting at: %s", datetime.data());

	Device::Init();

	#ifdef __EMSCRIPTEN__
  		emscripten_set_main_loop(loop, 0, 1);
	#else
		app.Run();
	#endif

	UserConfig::SaveToDisk();
	Device::Shutdown();
	Log::Shutdown();

	return 0;
}

}; // namespace adp.
