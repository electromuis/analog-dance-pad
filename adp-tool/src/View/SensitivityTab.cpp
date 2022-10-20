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
    "Click inside a sensor bar to adjust its activation threshold.";

static const char* ReleaseMsg =
    "Adjust release threshold (percentage of activation threshold).";

static constexpr int SENSOR_INDEX_NONE = -1;

SensitivityTab::SensitivityTab()
    : myAdjustingSensorIndex(SENSOR_INDEX_NONE)
{
}

void SensitivityTab::RenderSensor(int sensorIndex, float colX, float colY, float colW, float colH)
{
    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();
    auto wdl = ImGui::GetWindowDrawList();

    auto releaseThreshold = Device::Pad()->releaseThreshold;
    auto sensor = Device::Sensor(sensorIndex);
    auto pressed = sensor ? sensor->pressed : false;

    auto threshold = sensor ? sensor->threshold : 0.0;
    if (myAdjustingSensorIndex == sensorIndex)
        threshold = myAdjustingSensorThreshold;

    auto fillH = sensor ? float(sensor->value) * colH : 0.f;
    float thresholdY = colY + float(1 - threshold) * colH;

    // Full bar.
    wdl->AddRectFilled(
        { wp.x + colX, wp.y + colY },
        { wp.x + colX + colW, wp.y + colY + colH },
        RgbColorf::SensorBar.ToU32());

    // Filled bar that indicates current sensor reading.
    wdl->AddRectFilled(
        { wp.x + colX, wp.y + colY + colH - fillH },
        { wp.x + colX + colW, wp.y + colY + colH },
        pressed ? RgbColorf::SensorOn.ToU32() : RgbColorf::SensorOff.ToU32());

    // Line representing where the release threshold would be for the current sensor.
    if (releaseThreshold < 1.0)
    {
        float releaseY = colY + float(1 - releaseThreshold * threshold) * colH;
        wdl->AddRectFilled(
            { wp.x + colX, wp.y + thresholdY },
            { wp.x + colX + colW, wp.y + thresholdY + max(1.f, releaseY - thresholdY) },
            IM_COL32(50, 50, 50, 100));
    }

    // Line representing the sensitivity threshold.
    wdl->AddRectFilled(
        { wp.x + colX, wp.y + thresholdY - 2 },
        { wp.x + colX + colW, wp.y + thresholdY + 1 },
        IM_COL32_BLACK);
    wdl->AddRectFilled(
        { wp.x + colX, wp.y + thresholdY - 1 },
        { wp.x + colX + colW, wp.y + thresholdY },
        IM_COL32_WHITE);

    // Small text block at the top displaying sensitivity threshold.
    auto thresholdStr = fmt::format("{}%%", (int)std::lround(threshold * 100.0));
    auto ts = ImGui::CalcTextSize(thresholdStr.data());
    ImGui::SetCursorPos({ colX + (colW - ts.x) / 2, colY + 10 });
    ImGui::Text(thresholdStr.data());

    // Start/finish sensor threshold adjusting based on LMB click/release.
    if (myAdjustingSensorIndex == SENSOR_INDEX_NONE)
    {
        if (ImGui::IsMouseClicked(ImGuiPopupFlags_MouseButtonLeft) && ImGui::IsMousePosValid())
        {
            auto pos = ImGui::GetMousePos();
            if (pos.x >= wp.x + colX && pos.x < wp.x + colX + colW &&
                pos.y >= wp.y + colY && pos.y < wp.y + colY + colH)
            {
                myAdjustingSensorIndex = sensorIndex;
                myAdjustingSensorThreshold = 0.0;
                ImGui::CaptureMouseFromApp(true);
            }
        }
    }
    else if (!ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonLeft))
    {
        Device::SetThreshold(myAdjustingSensorIndex, myAdjustingSensorThreshold);
        myAdjustingSensorIndex = SENSOR_INDEX_NONE;
    }

    // If sensor threshold adjusting is active, update threshold based on mouse position.
    if (myAdjustingSensorIndex != SENSOR_INDEX_NONE)
    {
        double value = double(ImGui::GetMousePos().y) - (wp.y + colY);
        double range = max(1.0, double(colH));
        myAdjustingSensorThreshold = clamp(1.0 - (value / range), 0.0, 1.0);
    }
}

void SensitivityTab::Render()
{
	auto ws = ImGui::GetWindowSize();

    int colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    ImGui::TextUnformatted(ActivationMsg);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorBar", RgbColorf::SensorBar.rgb, colorEditFlags);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorOn", RgbColorf::SensorOn.rgb, colorEditFlags);
    ImGui::SameLine();
    ImGui::ColorEdit3("SensorOff", RgbColorf::SensorOff.rgb, colorEditFlags);

    map<int, vector<int>> mappings; // button -> sensors[]
    int numMappedSensors = 0;

    auto pad = Device::Pad();
    if (pad)
    {
        for (int i = 0; i < pad->numSensors; ++i)
        {
            auto sensor = Device::Sensor(i);
            if (sensor->button != 0)
            {
                mappings[sensor->button].push_back(i);
                ++numMappedSensors;
            }
        }
    }

    float colX = 10;
    float colY = ImGui::GetCursorPosY() + 10;
    float colW = (ws.x - 10) / max(numMappedSensors, 1) - 10;
    float colH = ws.y - ImGui::GetCursorPosY() - 50;

    for (auto& button : mappings)
    {
        for (auto& sensor : button.second)
        {
            RenderSensor(sensor, colX, colY, colW, colH);
            colX += colW + 10;
        }
    }
}

}; // namespace adp.
