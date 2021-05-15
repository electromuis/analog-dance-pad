#include "wx/dataview.h"
#include "wx/button.h"
#include "wx/generic/textdlgg.h"

#include "View/DeviceTab.h"

#include "Model/Device.h"

#include <string>

using namespace std;

namespace adp {

static constexpr const wchar_t* RenameMsg =
    L"Rename the pad device. Convenient if you have\nmultiple devices and want to tell them apart.";

static constexpr const wchar_t* RebootMsg =
    L"Reboot to bootloader. This will restart the\ndevice and provide a window to update firmware.";

static constexpr const wchar_t* FactoryResetMsg =
    L"Perform a factory reset. This will load the\nfirmware defaults ans the current configuration.";

const wchar_t* DeviceTab::Title = L"Device";

enum Ids { RENAME_BUTTON = 1, FACTORY_RESET_BUTTON = 2, REBOOT_BUTTON = 3};

DeviceTab::DeviceTab(wxWindow* owner) : BaseTab(owner)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();

    auto lRename = new wxStaticText(this, wxID_ANY, RenameMsg,
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(lRename, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    auto bRename = new wxButton(this, RENAME_BUTTON, L"Rename...", wxDefaultPosition, wxSize(200, -1));
    sizer->Add(bRename, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);

    auto lReset = new wxStaticText(this, wxID_ANY, FactoryResetMsg,
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(lReset, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
    auto bReset = new wxButton(this, FACTORY_RESET_BUTTON, L"Factory reset", wxDefaultPosition, wxSize(200, -1));
    sizer->Add(bReset, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);

    auto lReboot = new wxStaticText(this, wxID_ANY, RebootMsg,
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(lReboot, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
    auto bReboot = new wxButton(this, REBOOT_BUTTON, L"Bootloader mode", wxDefaultPosition, wxSize(200, -1));
    sizer->Add(bReboot, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);

    sizer->AddStretchSpacer();
    SetSizer(sizer);
}

void DeviceTab::OnRename(wxCommandEvent& event)
{
    auto pad = Device::Pad();
    if (pad == nullptr)
        return;

    wxTextEntryDialog dlg(this, L"Enter a new name for the device", L"Enter name", pad->name.data());
    dlg.SetTextValidator(wxFILTER_ALPHANUMERIC | wxFILTER_SPACE);
    dlg.SetMaxLength(pad->maxNameLength);
    if (dlg.ShowModal() == wxID_OK)
        Device::SetDeviceName(dlg.GetValue().wc_str());
}

void DeviceTab::OnFactoryReset(wxCommandEvent& event)
{
    Device::SendFactoryReset();
}

void DeviceTab::OnReboot(wxCommandEvent& event)
{
    Device::SendDeviceReset();
}

BEGIN_EVENT_TABLE(DeviceTab, wxWindow)
    EVT_BUTTON(RENAME_BUTTON, DeviceTab::OnRename)
    EVT_BUTTON(FACTORY_RESET_BUTTON, DeviceTab::OnFactoryReset)
    EVT_BUTTON(REBOOT_BUTTON, DeviceTab::OnReboot)
END_EVENT_TABLE()

}; // namespace adp.
