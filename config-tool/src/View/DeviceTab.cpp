#include "wx/dataview.h"
#include "wx/button.h"
#include "wx/generic/textdlgg.h"

#include "View/DeviceTab.h"

#include "Model/Device.h"

#include <string>

using namespace std;

namespace mpc {

static constexpr const wchar_t* RenameMsg =
    L"Rename the pad device. Convenient if you have\nmultiple devices and want to tell them apart.";

static constexpr const wchar_t* ResetMsg =
    L"Perform a hardware reset. This will restart the\ndevice and provide a window to update firmware.";

const wchar_t* DeviceTab::Title = L"Device";

enum Ids { RESET_BUTTON = 1, RENAME_BUTTON = 2 };

DeviceTab::DeviceTab(wxWindow* owner) : BaseTab(owner)
{
    mySizer = new wxBoxSizer(wxVERTICAL);
    myRenameLabel = new wxStaticText(this, wxID_ANY, RenameMsg);
    mySizer->Add(myRenameLabel, 0, wxLEFT | wxTOP, 8);
    mySizer->Add(new wxButton(this, RENAME_BUTTON, L"Rename...", wxDefaultPosition, wxSize(200, -1)),
        0, wxLEFT | wxTOP, 8);
    myResetLabel = new wxStaticText(this, wxID_ANY, ResetMsg);
    mySizer->Add(myResetLabel, 0, wxLEFT | wxTOP, 8);
    mySizer->Add(new wxButton(this, RESET_BUTTON, L"Hardware reset", wxDefaultPosition, wxSize(200, -1)),
        0, wxLEFT | wxTOP, 8);
    SetSizer(mySizer);
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

void DeviceTab::OnReset(wxCommandEvent& event)
{
    Device::SendDeviceReset();
}

BEGIN_EVENT_TABLE(DeviceTab, wxWindow)
    EVT_BUTTON(RENAME_BUTTON, DeviceTab::OnRename)
    EVT_BUTTON(RESET_BUTTON, DeviceTab::OnReset)
END_EVENT_TABLE()

}; // namespace mpc.
