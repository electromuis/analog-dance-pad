#pragma once

#include "pages/base.h"
#include "devices.h"

#include "wx/dataview.h"

#include <string>

using namespace devices;
using namespace std;

namespace pages {

class DevicePage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Device"; }

    enum Ids { RESET_BUTTON = 1, RENAME_BUTTON = 2 };

    DevicePage(wxWindow* owner) : BasePage(owner)
    {
        mySizer = new wxBoxSizer(wxVERTICAL);
        myRenameLabel = new wxStaticText(this, wxID_ANY, myRenameMsg);
        mySizer->Add(myRenameLabel, 0, wxLEFT | wxTOP, 8);
        mySizer->Add(new wxButton(this, RENAME_BUTTON, L"Rename...", wxDefaultPosition, wxSize(200, -1)),
            0, wxLEFT | wxTOP, 8);
        myResetLabel = new wxStaticText(this, wxID_ANY, myResetMsg);
        mySizer->Add(myResetLabel, 0, wxLEFT | wxTOP, 8);
        mySizer->Add(new wxButton(this, RESET_BUTTON, L"Hardware reset", wxDefaultPosition, wxSize(200, -1)),
            0, wxLEFT | wxTOP, 8);
        SetSizer(mySizer);
    }

    void OnRename(wxCommandEvent& event)
    {
        auto pad = DeviceManager::Pad();
        if (pad == nullptr)
            return;

        wxTextEntryDialog dlg(this, L"Enter a new name for the device", L"Enter name", pad->name.data());
        dlg.SetTextValidator(wxFILTER_ALPHANUMERIC | wxFILTER_SPACE);
        dlg.SetMaxLength(pad->maxNameLength);
        if (dlg.ShowModal() == wxID_OK)
            DeviceManager::SetDeviceName(dlg.GetValue().wc_str());
    }

    void OnReset(wxCommandEvent& event) { DeviceManager::SendDeviceReset(); }

    DECLARE_EVENT_TABLE()

private:
    inline static constexpr const wchar_t* myRenameMsg =
        L"Rename the pad device. Convenient if you have\nmultiple devices and want to tell them apart.";

    inline static constexpr const wchar_t* myResetMsg =
        L"Perform a hardware reset. This will restart the\ndevice and provide a window to update firmware.";

    wxSizer* mySizer;
    wxStaticText* myRenameLabel;
    wxStaticText* myResetLabel;
};

BEGIN_EVENT_TABLE(DevicePage, wxWindow)
    EVT_BUTTON(DevicePage::RENAME_BUTTON, DevicePage::OnRename)
    EVT_BUTTON(DevicePage::RESET_BUTTON, DevicePage::OnReset)
END_EVENT_TABLE()

}; // namespace pages.