#include <Adp.h>

#include <map>
#include <vector>
#include <iostream>

#include <imgui.h>
#include <fmt/core.h>

#include <Model/Device.h>
#include <View/Colors.h>
#include <View/GraphTab.h>
#include <View/GraphHistory.h>
#include <View/UserConfig.h>

using namespace std;

namespace adp {

static const char* ActivationMsg =
    "Click inside a graph to adjust its activation threshold.";

static constexpr int SENSOR_INDEX_NONE = -1;

GraphTab::GraphTab()
    : myAdjustingSensorIndex(SENSOR_INDEX_NONE)
{
}

void GraphTab::RenderSensor(
    int sensorIndex,
    RgbColorf& graphColor, GraphHistory& history)
{
    auto wdl = ImGui::GetWindowDrawList();
    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();

    // Configured threshold
    auto releaseThreshold = Device::Pad()->releaseThreshold;
    auto sensor = Device::Sensor(sensorIndex);
    if (!sensor)
      return;

    ImGui::InvisibleButton(fmt::format("##GraphSensor{}", sensorIndex).data(), ws);
    static float initialMouseY = 0.0f;
    static float initialSensorThreshold = 0.0f;

    // mouse clicked on this item
    if (ImGui::IsItemActivated()) {
        std::cout << "Item Activated for sensorIndex: " << sensorIndex << std::endl;
        initialMouseY = ImGui::GetIO().MousePos.y;
        initialSensorThreshold = sensor->threshold;
    }

    // mouse held down
    if (ImGui::IsItemActive()) {
        float currentMouseY = ImGui::GetIO().MousePos.y;
        float delta = currentMouseY - initialMouseY;
        std::cout << "initialMouseY: " << initialMouseY << " currentMouseY: " << currentMouseY << " delta: " << delta << std::endl;

        float t = delta / ws.y;
        myAdjustingSensorIndex = sensorIndex;
        myAdjustingSensorThreshold = clamp(sensor->threshold - t, 0.0, 1.0);
    }

    // mouse released
    if (ImGui::IsItemDeactivated()) {
        Device::SetThreshold(sensorIndex, myAdjustingSensorThreshold, myAdjustingSensorThreshold);
    }

    auto pressed = sensor->pressed;

    // Current threshold
    auto threshold = sensor->threshold;

    if (myAdjustingSensorIndex == sensorIndex)
        threshold = myAdjustingSensorThreshold;

    auto fillH = float(sensor->value) * ws.y;
    history.AddValue(sensor->value);
    float thresholdY = float(1 - threshold) * ws.y;
    float normalizer = (ws.y / 2) - thresholdY;

    // background of graph bar.
    wdl->AddRectFilled(
        { wp.x, wp.y },
        { wp.x + ws.x, wp.y + ws.y},
        UserConfig::SensorBar.ToU32());

    // add threshold line
    wdl->AddLine(
        { wp.x, wp.y + thresholdY + normalizer },
        { wp.x + ws.x, wp.y + thresholdY + normalizer},
        graphColor.ToU32(), 3);

    // Draw the graph line showing sensor values over time
    if (history.values.size() >= 2) {
        float stepX = ws.x / float(GraphHistory::MAX_SAMPLES - 1);
        int startIdx = GraphHistory::MAX_SAMPLES - history.values.size();

        for (size_t i = 1; i < history.values.size(); ++i) {
            float x0 = wp.x + (startIdx + i - 1) * stepX;
            float x1 = wp.x + (startIdx + i) * stepX;

            float prevValue = float(history.values[i - 1]) * ws.y ;
            float currValue = float(history.values[i]) * ws.y;
            float y0 = wp.y + ws.y - (prevValue - normalizer);
            float y1 = wp.y + ws.y - (currValue - normalizer);

            y0 = clamp(y0, wp.y, wp.y + ws.y);
            y1 = clamp(y1, wp.y, wp.y + ws.y);

            wdl->AddLine(
                { x0, y0 },
                { x1, y1 },
                history.values[i-1] > threshold ?
                    UserConfig::GraphActive.ToU32() :
                    graphColor.ToU32(),
                3);
        }
    }

    // Small text block at the top displaying sensitivity threshold.
    if (myCheckBox) {
        auto thresholdStr = fmt::format("{}%", (int)std::lround(threshold * 100.0));
        auto ts = ImGui::CalcTextSize(thresholdStr.data());
        ImGui::SetCursorPosX((ws.x - ts.x) / 2);
        ImGui::SetCursorPosY(ImGui::GetStyle().ItemSpacing.y);
        ImGui::TextUnformatted(thresholdStr.data());
    }
}

void GraphTab::Render()
{

    int colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    ImGui::TextUnformatted(ActivationMsg);
    ImGui::ColorEdit3("GraphActive", UserConfig::GraphActive.rgb, colorEditFlags);

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

                // Only create up to GRAPH_COLORS_SIZE color editors.
                // as excess sensors will reuse colors.
                if (numMappedSensors < GRAPH_COLORS_SIZE)
                {
                    ImGui::SameLine();
                    ImGui::ColorEdit3(
                        fmt::format("GraphColorSensor{}Button{}", numMappedSensors, sensor->button).data(),
                        UserConfig::GetGraphColor(numMappedSensors).rgb,
                        colorEditFlags);
                }
                ++numMappedSensors;
            }
        }
    }

    ImGui::Checkbox("Show threshold percent", &myCheckBox);
    auto cra = ImGui::GetContentRegionAvail();

    if (numMappedSensors != myGraphHistories.size())
    {
        myGraphHistories.clear();
        myGraphHistories.resize(numMappedSensors);
    }

    float colW = (cra.x - 10);
    float colH = (cra.y  / max(numMappedSensors, 1)) - ImGui::GetStyle().ItemSpacing.y;

    int colorIndex = 0;
    for (auto& button : mappings)
    {
        for (auto& sensor : button.second)
        {
            ImGui::BeginChild(
                fmt::format("GraphSensorChild{}", sensor).data(),
                ImVec2(colW, colH));
            RenderSensor(
                sensor,
                UserConfig::GetGraphColor(colorIndex),
                myGraphHistories[colorIndex]);
            ImGui::EndChild();
            colorIndex++;
        }
    }
}

}; // namespace adp.
