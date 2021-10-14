#pragma once

#include "wx/dialog.h"

#include "Model/Updater.h"

using namespace std;

namespace adp {

class Application;

class AdpUpdateDialog : public wxDialog
{
public:
    AdpUpdateDialog(SoftwareUpdate& update);

    void OnStop(wxCommandEvent& event);
    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);

private:
    SoftwareUpdate& update;

    DECLARE_EVENT_TABLE()
};


void ShowUpdateDialog(SoftwareUpdate update);

}; // namespace adp.
