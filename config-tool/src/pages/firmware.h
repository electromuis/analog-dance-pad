#pragma once

#include "pages/base.h"
#include "devices.h"

#include "wx/dataview.h"

#include <string>

using namespace devices;
using namespace std;

namespace pages {

class FirmwarePage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Firmware"; }

    enum Ids { RESET_BUTTON = 1 };

    FirmwarePage(wxWindow* owner) : BasePage(owner)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new wxButton(this, RESET_BUTTON, L"Reset"));
        SetSizer(sizer);
    }

    void OnReset(wxCommandEvent& event) { DeviceManager::SendDeviceReset(); }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(FirmwarePage, wxWindow)
    EVT_BUTTON(FirmwarePage::RESET_BUTTON, FirmwarePage::OnReset)
END_EVENT_TABLE()

}; // namespace pages.