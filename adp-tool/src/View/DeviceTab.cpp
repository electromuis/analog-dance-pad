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

class UpdateTask {
public:
    UpdateTask(std::string firmwareFile)
        :firmwareFile(firmwareFile)
    {

    }

    void FirmwareCallback(AvrdudeEvent event, std::string message, int p)
    {
        switch(event) {
            case AE_START:
                status = "Starting";
                break;

            case AE_PROGRESS:
                if(message != lastTask)
                {
                    lastTask = message;
                    tasksCompleted ++;
                }

                progress = tasksCompleted * 100 + p;
                break;
        }
    }

    FlashResult Start()
    {
        uploader.SetEventHandler(std::bind(&UpdateTask::FirmwareCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        return uploader.UpdateFirmware(firmwareFile);
    }

    void Done()
    {

    }

    void Render()
    {
        if (ImGui::BeginPopupModal("Updating", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Updating firmware ...");

            ImGui::LabelText("Status", status.c_str());
            float progressFloat = (float)progress / 300;
            ImGui::ProgressBar(progressFloat);

            ImGui::EndPopup();
        }

        bool close = false;
        if (ImGui::BeginPopupModal("Update error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(uploader.GetErrorMessage().c_str());

            ImGui::EndPopup();
        }
    }

    std::string GetFirmwareFile() { return firmwareFile; }

protected:
    std::string firmwareFile;
    FirmwareUploader uploader;
    std::string status;
    std::string lastTask;
    int tasksCompleted = 0;
    int progress = 0;
};

std::unique_ptr<UpdateTask> updateTask;

static void OnUploadFirmware()
{
    nfdchar_t* rawOutPath = nullptr;
	const nfdchar_t* defaultPath = (updateTask == nullptr ? nullptr : updateTask->GetFirmwareFile().data());
	auto nfdResult = NFD_OpenDialog("hex", nullptr, &rawOutPath);
	if (nfdResult != NFD_OKAY)
		return;
		
	string outPath(rawOutPath ? rawOutPath : "");
	free(rawOutPath);

    if(updateTask)
        updateTask.reset();

    updateTask = std::make_unique<UpdateTask>(outPath);
    updateTask->Render();

    auto updateResult = updateTask->Start();
    switch(updateResult) {
        case FLASHRESULT_NOTHING:
            ImGui::OpenPopup("Updating");
            break;
        case FLASHRESULT_FAILURE_BOARDTYPE:
            ImGui::OpenPopup("Update board type error");
            break;
        default:
            ImGui::OpenPopup("Update error");
            break;
    }

    if(updateTask->Start() != FLASHRESULT_NOTHING) {
        ImGui::OpenPopup("Update error");
    } else {
        ImGui::OpenPopup("Updating");
    }
}

#endif

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
        OnUploadFirmware();
#endif // __EMSCRIPTEN__

    ImGui::PopStyleVar();

    RenamePopup();
    
#ifndef __EMSCRIPTEN__
    if(updateTask)
        updateTask->Render();
#endif
}

}; // namespace adp.
