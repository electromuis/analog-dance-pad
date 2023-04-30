#include <Adp.h>

#include <string>

#include <imgui.h>

#include <Model/Device.h>
#include <Model/Firmware.h>
#include <Model/Log.h>
#include <Model/Utils.h>

#include <View/DeviceTab.h>

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

static void OnUploadFirmware()
{
    /*
    wxFileDialog dlg(this, L"Open XYZ file", L"", L"", L"ADP firmware (*.hex)|*.hex",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dlg.ShowModal() == wxID_CANCEL)
        return;

    firmwareDialog->UpdateFirmware((dlg.GetPath().ToStdWstring()));
    */
}

static void OnRename()
{
    /*
    auto pad = Device::Pad();
    if (pad == nullptr)
        return;

    wxTextEntryDialog dlg(this, L"Enter a new name for the device", L"Enter name", pad->name.data());

    wxTextValidator validator = wxTextValidator(wxFILTER_INCLUDE_CHAR_LIST);
    validator.SetCharIncludes("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_ ()");
    dlg.SetTextValidator(validator);

    dlg.SetMaxLength(pad->maxNameLength);
    if (dlg.ShowModal() == wxID_OK)
        Device::SetDeviceName(dlg.GetValue());
    */
}

void DeviceTab::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

    ImGui::TextUnformatted(RenameMsg);
    if (ImGui::Button("Rename...", { 200, 30 }))
        OnRename();

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
}

}; // namespace adp.
