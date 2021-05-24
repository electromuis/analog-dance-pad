#pragma once

#include "wx/window.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/gauge.h"

#include "View/BaseTab.h"

using namespace std;

namespace adp {

class FirmwareDialog : public wxDialog
{
public:
    FirmwareDialog(const wxString& title);

    void OnOpen(wxCommandEvent& event);
    void OnGo(wxCommandEvent& event);
    void BindFile(wstring file);

private:
    wxStaticText* tStatus;
    wxGauge* progressBar;
    wstring boundFile;

    DECLARE_EVENT_TABLE()
};

class DeviceTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;
    FirmwareDialog* firmwareDialog;

    DeviceTab(wxWindow* owner);

    void OnRename(wxCommandEvent& event);
    void OnReboot(wxCommandEvent& event);
    void OnFactoryReset(wxCommandEvent& event);
    void OnUploadFirmware(wxCommandEvent& event);

    wxWindow* GetWindow() override { return this; }

    DECLARE_EVENT_TABLE()
};

}; // namespace adp.
