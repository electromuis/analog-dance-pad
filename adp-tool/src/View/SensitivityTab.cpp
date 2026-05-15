#include <Adp.h>

#include <map>
#include <vector>

#include <imgui.h>
#include <fmt/core.h>

#include <Model/Device.h>
#include <View/Colors.h>
#include <View/SensitivityTab.h>

using namespace std;

namespace adp {

static const char* ActivationMsg =
    "Left click inside a sensor bar to adjust its activation threshold.";

static const char* ReleaseMsg =
    "Right click to adjust release threshold.";

static constexpr int SENSOR_INDEX_NONE = -1;
static float GLOBAL_RELEASE_THRESHOLD = 0.0;
static bool INITIALIZED = false;
static ReleaseMode RELEASE_MODE;

SensitivityTab::SensitivityTab()
    : myAdjustingSensorIndex(SENSOR_INDEX_NONE)
{
}

void SensitivityTab::OnDeviceChanged()
{
    INITIALIZED = false;
}

void SensitivityTab::RenderSensor(int sensorIndex)
{
    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();
    auto wdl = ImGui::GetWindowDrawList();

    auto sensor = Device::Sensor(sensorIndex);
    if (!sensor)
        return;

    auto pressed = sensor->pressed;
    auto threshold = sensor->threshold;
    auto releaseThreshold = sensor->releaseThreshold;

    ImGui::InvisibleButton(fmt::format("##GraphSensor{}", sensorIndex).data(), ws);

    // While mouse clicked
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        myAdjustingSensorIndex = sensorIndex;
        myAdjustingSensorThreshold = threshold;
        myAdjustingSensorReleaseThreshold = releaseThreshold;
        myAdjustingRelativeReleaseThreshold = releaseThreshold / threshold;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        myAdjustingSensorIndex = sensorIndex;
        myAdjustingSensorThreshold = threshold;
        myAdjustingSensorReleaseThreshold = releaseThreshold;
    }

    // During left mouse drag
    if (myAdjustingSensorIndex == sensorIndex && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        auto currentMouseY = ImGui::GetIO().MousePos.y;
        double t = (wp.y + ws.y - currentMouseY) / ws.y;
        myAdjustingSensorThreshold = clamp(t, 0.0, 1.0);

        // update visual threshold during adjustment
        threshold = myAdjustingSensorThreshold;
        releaseThreshold = myAdjustingSensorThreshold * myAdjustingRelativeReleaseThreshold;
    }

    // Only allow adjusting release threshold if in individual release mode.
    if (myAdjustingSensorIndex == sensorIndex && ImGui::IsMouseDown(ImGuiMouseButton_Right) && RELEASE_MODE == RELEASE_INDIVIDUAL)
    {
        // assume this box is 100px tall, and starts on (0,10)
        // (0,10) ┌─────┐
        // (0,30) ├─────┤ <- threshold line (0.8 of total box height for example)
        //        |     |
        // (0,70) ├─────┤ <- release threshold (0.4 of total box height for example)
        //        |     |
        //        |     |
        // (0,110)└─────┘
        auto currentMouseY = ImGui::GetIO().MousePos.y;

        // current mouse y is where we want to set our release threshold
        // 1 - ((70 - 10) / (100)) = 1 - 0.6 = 0.4
        double currentMousePercent = 1.0 - ((currentMouseY - wp.y) / ws.y);
        myAdjustingSensorReleaseThreshold = clamp(currentMousePercent, 0.0, myAdjustingSensorThreshold);

        // update visual release threshold during adjustment
        releaseThreshold = myAdjustingSensorReleaseThreshold;
    }

    // LEFT RELEASE
    if (myAdjustingSensorIndex == sensorIndex && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        // Set the threshold and preserve the relative scaling of the release
        // threshold.
        Device::SetThreshold(
            sensorIndex,
            myAdjustingSensorThreshold,
            myAdjustingSensorThreshold * myAdjustingRelativeReleaseThreshold);
        myAdjustingSensorIndex = SENSOR_INDEX_NONE;
    }

    // RIGHT RELEASE
    if (myAdjustingSensorIndex == sensorIndex && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        Device::SetThreshold(
            sensorIndex,
            myAdjustingSensorThreshold,
            clamp(myAdjustingSensorReleaseThreshold, 0.0, myAdjustingSensorThreshold * 0.99));
        myAdjustingSensorIndex = SENSOR_INDEX_NONE;
    }

    if(RELEASE_MODE == RELEASE_NONE) {
        releaseThreshold = threshold;
    }
    else if(RELEASE_MODE == RELEASE_GLOBAL) {
        releaseThreshold = threshold * GLOBAL_RELEASE_THRESHOLD;
    }

    auto fillH = float(sensor->value) * ws.y;
    float thresholdY = float(1 - threshold) * ws.y;

    // Full bar.
    wdl->AddRectFilled(
        { wp.x, wp.y },
        { wp.x + ws.x, wp.y + ws.y},
        RgbColorf::SensorBar.ToU32());

    // Filled bar that indicates current sensor reading.
    wdl->AddRectFilled(
        { wp.x , wp.y + ws.y - fillH },
        { wp.x + ws.x, wp.y + ws.y },
        pressed ? RgbColorf::SensorOn.ToU32() : RgbColorf::SensorOff.ToU32());

    // Line representing where the release threshold would be for the current sensor.
    if (releaseThreshold < 1.0)
    {
        float releaseY = float(1 - releaseThreshold) * ws.y;
        wdl->AddRectFilled(
            { wp.x , wp.y + thresholdY },
            { wp.x + ws.x, wp.y + thresholdY + max(1.f, releaseY - thresholdY) },
            IM_COL32(50, 50, 50, 100));
    }

    // Line representing the sensitivity threshold.
    wdl->AddRectFilled(
        { wp.x, wp.y + thresholdY - 2 },
        { wp.x + ws.x, wp.y + thresholdY + 1 },
        IM_COL32_BLACK);
    wdl->AddRectFilled(
        { wp.x , wp.y + thresholdY - 1 },
        { wp.x + ws.x, wp.y + thresholdY },
        IM_COL32_WHITE);

    // Small text block at the top displaying sensitivity threshold.
    auto thresholdStr = fmt::format("{}%", (int)std::lround(threshold * 100.0));
    auto ts = ImGui::CalcTextSize(thresholdStr.data());
    ImGui::SetCursorPos({ (ws.x - ts.x) / 2, 10 });
    ImGui::TextUnformatted(thresholdStr.data());
}

void SensitivityTab::Render()
{
    auto pad = Device::Pad();
    if (!pad)
    {
        ImGui::TextUnformatted("No device connected.");
        return;
    }

    // On first startup, pull the mode and threshold from the device
    if (!INITIALIZED)
    {
        GLOBAL_RELEASE_THRESHOLD = pad->releaseThreshold;
        RELEASE_MODE = pad->releaseMode;
        INITIALIZED = true;
    }

    auto ws = ImGui::GetWindowSize();

    int colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    ImGui::TextUnformatted(ActivationMsg);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorBar", RgbColorf::SensorBar.rgb, colorEditFlags);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorOn", RgbColorf::SensorOn.rgb, colorEditFlags);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorOff", RgbColorf::SensorOff.rgb, colorEditFlags);

    if(RELEASE_MODE == RELEASE_INDIVIDUAL) {
        ImGui::TextUnformatted(ReleaseMsg);
    }

    map<int, vector<int>> mappings; // button -> sensors[]
    int numMappedSensors = 0;

    for (int i = 0; i < pad->numSensors; ++i)
    {
        auto sensor = Device::Sensor(i);
        if (sensor->button != 0)
        {
            mappings[sensor->button].push_back(i);
            ++numMappedSensors;
        }

    }


    float colW = (ws.x - 10) / max(numMappedSensors, 1) - 10;
    float colH = ws.y - ImGui::GetCursorPosY() - 100;

    ImGui::BeginGroup();
    for (auto& button : mappings)
    {
        for (auto& sensor : button.second)
        {
            ImGui::BeginChild(
                fmt::format("SensorGroup{}", sensor).data(),
                { colW, colH });
            RenderSensor(sensor);
            ImGui::EndChild();
            ImGui::SameLine();
        }
    }
    ImGui::EndGroup();

    ImGui::TextUnformatted("Release Mode:");
    ImGui::SameLine();

    if(ImGui::RadioButton("None", RELEASE_MODE == RELEASE_NONE)) {
        Device::SetReleaseMode(ReleaseMode::RELEASE_NONE);
        RELEASE_MODE = RELEASE_NONE;

        for (int i = 0; i < pad->numSensors; ++i)
        {
            auto sensor = Device::Sensor(i);
            if(sensor) {
                Device::SetThreshold(i, sensor->threshold, sensor->threshold * 0.99);
            }
        }
    }

    ImGui::SameLine();
    if(ImGui::RadioButton("Global", RELEASE_MODE == RELEASE_GLOBAL)) {
        Device::SetReleaseMode(RELEASE_GLOBAL);
        RELEASE_MODE = RELEASE_GLOBAL;
        Device::SetReleaseThreshold(GLOBAL_RELEASE_THRESHOLD);
    }

    ImGui::SameLine();
    if(ImGui::RadioButton("Individial", RELEASE_MODE == RELEASE_INDIVIDUAL)) {
        Device::SetReleaseMode(RELEASE_INDIVIDUAL);
        RELEASE_MODE = RELEASE_INDIVIDUAL;
    }

    if(RELEASE_MODE == RELEASE_GLOBAL) {
        ImGui::SliderFloat("##", &GLOBAL_RELEASE_THRESHOLD, 0.0, 1.0, "%.2f");

        if (ImGui::IsItemDeactivatedAfterEdit())
            Device::SetReleaseThreshold(GLOBAL_RELEASE_THRESHOLD);
    }
}

}; // namespace adp.
