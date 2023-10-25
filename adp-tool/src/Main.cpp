#include <Adp.h>

#define _HAS_STD_BYTE 0

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

#include <Model/Device.h>
#include <Model/Log.h>
#include <Model/Utils.h>

#include <View/Application.h>
#include <View/Image.h>
#include <View/DeviceTab.h>
#include <View/LightsTab.h>
#include <View/MappingTab.h>
#include <View/SensitivityTab.h>

#ifndef __EMSCRIPTEN__
	#include <nfd.h>
#else
	#include <emscripten.h>

	EM_JS(void, device_scan_js, (), {
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
	: Walnut::Application(800, 800, TOOL_NAME)
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
	string outPath(rawOutPath ? rawOutPath : "");
	free(rawOutPath);

	if (result != NFD_OKAY)
		return;

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
	string outPath(rawOutPath);
	free(rawOutPath);

	if (result != NFD_OKAY)
		return;

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
#ifndef __EMSCRIPTEN__
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Connect remote"))
			Device::Connect("192.168.2.32", "1234");

		if (ImGui::MenuItem("Load profile"))
			LoadProfile();

		if (ImGui::MenuItem("Save profile"))
			SaveProfile();

		if (ImGui::MenuItem("Exit"))
			Close();

		ImGui::EndMenu();
	}
#else
	if(ImGui::Button("Device select"))
		device_scan_js();
#endif
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
	if (now - myLastUpdateTime > 10ms)
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
	ImGui::Text(text);

	#ifdef __EMSCRIPTEN__
	if(ImGui::Button("Device select"))
		device_scan_js();
	#endif
}

void AdpApplication::RenderAboutTab()
{
	auto ws = ImGui::GetWindowSize();
	const char* lines[] =
	{
		"ADP Tool " ADP_VERSION_STR,
		"(c) Bram van de Wetering 2022",
		"(c) DDR-EXP",
	};
	ImGui::SetCursorPosY(round(ws.y / 2 - 80));
	ImGui::SetCursorPosX(round(ws.x / 2 - 32));
	auto ico = m_Icon64.get();
	auto size = ImVec2(float(ico->GetWidth()), float(ico->GetHeight()));
	ImGui::Image(ico->GetDescriptorSet(), size);
	ImGui::SetCursorPosY(round(ws.y / 2));
	for (int i = 0; i < std::size(lines); ++i)
	{
		auto ts = ImGui::CalcTextSize(lines[i]);
		ImGui::SetCursorPosX(ws.x / 2 - ts.x / 2);
		ImGui::Text(lines[i]);
	}
}

void AdpApplication::RenderLogTab()
{
	for (int i = 0; i < Log::NumMessages(); ++i)
		ImGui::TextUnformatted(Log::Message(i).data());
}

#ifdef __EMSCRIPTEN__
AdpApplication app;
void main_loop() { app.RunOnce(); }

int Main(int argc, char** argv)
{
	Log::Init();
	auto versionString = fmt::format("{} {}.{}", TOOL_NAME, ADP_VERSION_MAJOR, ADP_VERSION_MINOR);

	Device::Init();

	emscripten_set_main_loop(main_loop, 0, 1);

	Device::Shutdown();
	Log::Shutdown();

	return 0;
}
#else
int Main(int argc, char** argv)
{
	Log::Init();

	auto versionString = fmt::format("{} {}.{}", TOOL_NAME, ADP_VERSION_MAJOR, ADP_VERSION_MINOR);
	//auto now = fmt::format("{}", chrono::system_clock::now());
	//Log::Writef("Application started: %s - %s", versionString.data(), now.data());

	Device::Init();

	AdpApplication app;
	app.Run();

	Device::Shutdown();
	Log::Shutdown();

	return 0;
}
#endif

}; // namespace adp.