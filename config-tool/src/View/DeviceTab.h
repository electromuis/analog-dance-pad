#pragma once

#include "wx/window.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#include "View/BaseTab.h"

namespace adp {

class DeviceTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    DeviceTab(wxWindow* owner);

    void OnRename(wxCommandEvent& event);
    void OnReboot(wxCommandEvent& event);
    void OnFactoryReset(wxCommandEvent& event);
    void OnUploadFirmware(wxCommandEvent& event);

    wxWindow* GetWindow() override { return this; }

    DECLARE_EVENT_TABLE()
};

}; // namespace adp.
