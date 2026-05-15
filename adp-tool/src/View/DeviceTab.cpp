#include <Adp.h>

#include <string>
#include <memory>

#include <imgui.h>

#include <Model/Device.h>
#include <Model/Firmware.h>
#include <Model/Log.h>
#include <Model/Utils.h>

#include <View/DeviceTab.h>

#ifndef __EMSCRIPTEN__
	#include <nfd.h>
#endif

using namespace std;

namespace adp {

static constexpr const char* RenameMsg =
    "Rename the pad device. Convenient if you have\nmultiple devices and want to tell them apart.";

static constexpr const char* RebootMsg =
    "Reboot to bootloader. This will restart the\ndevice and provide a window to update firmware.";

static constexpr const char* FactoryResetMsg =
    "Perform a factory reset. This will load the\nfirmware default configuration.";

static constexpr const char* UpdateFirmwareMsg =
    "Upload a firmware file to the pad device.";

#ifndef __EMSCRIPTEN__
class FlashInstance {
public:
    FlashInstance()
    {
        uploader.Reset();
    }
    
    void Render()
    {
        if(shoudlOpen)
        {
            ImGui::OpenPopup("Flash");
            shoudlOpen = false;
        }

        if (!ImGui::BeginPopupModal("Flash", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::InputText("Firmware file", firmwareFile, 255);
        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(120, 0)))
        {
            nfdchar_t* rawOutPath = nullptr;
            const nfdchar_t* defaultPath = firmwareFile;
            auto nfdResult = NFD_OpenDialog("adpf", nullptr, &rawOutPath);
            if (nfdResult == NFD_OKAY)
            {   
                string outPath(rawOutPath ? rawOutPath : "");
                free(rawOutPath);
                outPath.copy(firmwareFile, 255);
            }
        }

        static char selectedPortStr[255] = "Select port";
        if(selectedPort != -1)
        {
            strcpy(selectedPortStr, serialPorts[selectedPort].c_str());
        }

        if(advancedMode)
        {
            ImGui::Checkbox("Ignore board type", &ignoreBoardType);

            if(ImGui::BeginCombo("Serial port", selectedPortStr))
            {
                for(int i=0; i<serialPorts.size(); i++)
                {
                    if(ImGui::Selectable(serialPorts[i].c_str(), i == selectedPort))
                        selectedPort = i;
                }
                ImGui::EndCombo();
            }
        }

        ImGui::TextUnformatted("Status:");
        ImGui::SameLine();
        ImGui::TextUnformatted(message.c_str());

        ImGui::ProgressBar(uploader.GetProgress());

        bool disabled = !CanFlash();

        if(disabled)
            ImGui::BeginDisabled();

        if(ImGui::Button("Flash", ImVec2(120, 0)))
        {
            Start();
        }

        if(disabled)
            ImGui::EndDisabled();

        disabled = IsWorking();

        if(disabled)
            ImGui::BeginDisabled();
        ImGui::SameLine();
        if(ImGui::Button("Close", ImVec2(120, 0)) && !IsWorking())
        {
            Close();
        }
        if(disabled)
            ImGui::EndDisabled();

        ImGui::EndPopup();

        PollUploader();
    }

    void Close()
    {
        ImGui::CloseCurrentPopup();
        firmware = nullptr;
        Device::SetSearching(true);
    }

    void Open(bool advanced = false)
    {
        advancedMode = advanced;
        if(advancedMode)
        {
            serialPorts.clear();
            ListSerialPorts(serialPorts);
            selectedPort = -1;
        }

        shoudlOpen = true;
    }

    void FirmwareCallback(FlashResult event, std::string m, int progress)
    {
        if(event == FLASHRESULT_MESSAGE)
        {
            message = m;
        }
    }

protected:
    void PollUploader()
    {
        switch(uploader.GetFlashResult())
        {
            case FLASHRESULT_NOTHING:
                break;
            case FLASHRESULT_SUCCESS:
                message = "Success";
                uploader.Reset();
                Device::SetSearching(true);
                break;
            case FLASHRESULT_FAILURE:
                message = uploader.GetErrorMessage();
                uploader.Reset();
                Device::SetSearching(true);
                break;
        }
    }

    bool IsWorking()
    {
        if(uploader.GetFlashResult() != FLASHRESULT_NOTHING)
            return true;

        return false;
    }

    void Start()
    {
        if(!CanFlash())
            return;

        try {
            firmware = FirmwarePackageRead(firmwareFile);
        } catch (std::exception& e) {
            Log::Writef("Error: %s", e.what());
            message = e.what();
            return;
        }

        uploader.SetEventHandler([this](FlashResult event, std::string message, int progress)
        {
            FirmwareCallback(event, message, progress);
        });

        if(advancedMode)
        {
            uploader.SetIgnoreBoardType(ignoreBoardType);
            uploader.SetPort(serialPorts[selectedPort]);
            if(ignoreBoardType)
            {
                uploader.SetArchtType(firmware->GetBoardType().archType);
            }
        } else {
            uploader.SetDoDeviceConnect(true);
        }

        try {
            Device::SetSearching(false);
            uploader.WriteFirmwareThreaded(firmware);
        } catch (std::exception& e) {
            Log::Writef("Error: %s", e.what());
            message = e.what();
        }
    }

    bool CanFlash()
    {
        if(advancedMode)
        {
            if(selectedPort == -1 || selectedPort >= serialPorts.size())
                return false;
        }

        if(strlen(firmwareFile) == 0)
            return false;

        if(IsWorking())
            return false;

        return true;
    }

    bool ignoreBoardType = false;
    bool advancedMode = false;
    bool shoudlOpen = false;
    char firmwareFile[255];
    std::vector<std::string> serialPorts;
    int selectedPort = -1;
    std::string message = "IDLE";

    FirmwarePackagePtr firmware;
    FirmwareUploader uploader;
};

FlashInstance flashInstance;
#endif // __EMSCRIPTEN__

void DeviceTab::RenderFlash()
{
    flashInstance.Render();
}

void DeviceTab::OpenFlashDialog(bool advanced)
{
    flashInstance.Open(advanced);
}

static char padName[255] = "";

static void RenamePopup()
{
    auto pad = Device::Pad();
    if (pad == nullptr)
        return;

    if (ImGui::BeginPopupModal("Rename", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Pad name", padName, pad->maxNameLength);

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            Device::SetDeviceName(padName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }

        ImGui::EndPopup();
    }
}

void DeviceTab::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

    ImGui::TextUnformatted(RenameMsg);
    if (ImGui::Button("Rename...", { 200, 30 }))
    {
        auto pad = Device::Pad();
        if (pad == nullptr)
            return;
        
        //strcpy_s(padName, pad->maxNameLength, pad->name.c_str());
        strcpy(padName, pad->name.c_str());
        
        ImGui::OpenPopup("Rename");
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::TextUnformatted(FactoryResetMsg);
    if (ImGui::Button("Factory reset", { 200, 30 }))
        Device::SendFactoryReset();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::TextUnformatted(RebootMsg);
    if (ImGui::Button("Bootloader mode", { 200, 30 }))
        Device::SendDeviceReset();

#ifndef __EMSCRIPTEN__
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::TextUnformatted(UpdateFirmwareMsg);
    if (ImGui::Button("Update firmware...", { 200, 30 }))
        OpenFlashDialog(false);
#endif // __EMSCRIPTEN__

    ImGui::PopStyleVar();

    RenamePopup();
    
// #ifndef __EMSCRIPTEN__
//     if(updateTask)
//         updateTask->Render();
// #endif
}

}; // namespace adp.
