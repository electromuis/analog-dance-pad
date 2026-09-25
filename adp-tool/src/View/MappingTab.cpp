#include <Adp.h>

#include <imgui.h>
#include <fmt/core.h>

#include <Model/Device.h>
#include <View/Colors.h>
#include <View/MappingTab.h>
#include <View/UserConfig.h>

using namespace std;

namespace adp {

static string ButtonIndexToText(int index)
{
    return (index == 0) ? "-" : fmt::format("Button {}", index);
}

static void RenderSensorBar(int sensorIndex)
{
    auto sensorBarId = fmt::format("##SensorMappingBar{}", sensorIndex);

    ImGui::BeginChild(sensorBarId.data(), ImVec2(200, ImGui::GetFrameHeight()));

    auto wp = ImGui::GetWindowPos();
    auto ws = ImGui::GetWindowSize();
    auto wdl = ImGui::GetWindowDrawList();

    auto sensor = Device::Sensor(sensorIndex);
    auto fillW = sensor ? float(sensor->value) * ws.x : 0.f;

    wdl->AddRectFilled(
        { wp.x, wp.y },
        { wp.x + ws.x, wp.y + ws.y },
        UserConfig::SensorBar.ToU32());

    wdl->AddRectFilled(
        { wp.x, wp.y },
        { wp.x + fillW, wp.y + ws.y },
        UserConfig::SensorOff.ToU32());

    ImGui::EndChild();
}

static int selectedSensor = -1;

static void ConfigModal()
{
    auto pad = Device::Pad();
    auto sensor = Device::Sensor(selectedSensor);
    
    if (ImGui::BeginPopupModal("Sensor config", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto sensorText = fmt::format("Sensor {}", selectedSensor+1);
        ImGui::TextUnformatted(sensorText.data());

        RenderSensorBar(selectedSensor);
        ImGui::SameLine();
        ImGui::TextUnformatted("Reading");

        int resistorValue = 255 - sensor->resistorValue;
        ImGui::PushItemWidth(200);
        if(ImGui::SliderInt("Gain", &resistorValue, 1, 254))
        {
            Device::SetAdcConfig(selectedSensor, 255 - resistorValue);
        }

        if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

void MappingTab::Render()
{
    auto pad = Device::Pad();
    bool configButton = Device::Pad()->featureDigipot;

    string buttonOptions("-\0", 2);
    for (int i = 1; i <= pad->numButtons; ++i)
        buttonOptions.append(fmt::format("Button {}{}", i, '\0'));

    for (int i = 0; i < pad->numSensors; ++i)
    {
        auto sensorText = fmt::format("Sensor {}", i+1);
        auto itemId = ImGui::GetID(sensorText.c_str());
        ImGui::PushID(itemId);

        int selected = Device::Sensor(i)->button;
        ImGui::TextUnformatted(sensorText.data());
        ImGui::SameLine();
        ImGui::SetCursorPosX(100);

        auto sensorId = fmt::format("##SensorMapping{}", i);
        ImGui::PushItemWidth(200);
        if (ImGui::Combo(sensorId.data(), &selected, buttonOptions.data()))
            Device::SetButtonMapping(i, selected);

        ImGui::SameLine();
        RenderSensorBar(i);

        if (configButton)
        {
            ImGui::SameLine();
            if (ImGui::Button("Config"))
            {
                selectedSensor = i;
                ImGui::OpenPopup("Sensor config");
            }
        }


        if(selectedSensor == i)
            ConfigModal();

        ImGui::PopID();
    }
}

}; // namespace adp.
