#include "Adp.h"

#include <string>

#include "wx/msgdlg.h"
#include "wx/sizer.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include <wx/stdpaths.h>
#include "wx/app.h"

#include "Main.h"
#include "Model/Updater.h"
#include "Model/Device.h"
#include "View/UpdaterView.h"

using namespace std;

namespace adp {

enum Ids { STOP_BUTTON = 1 };

static constexpr const wchar_t* UpdateMsg =
L"An update has been found for %s, you are currently running v%i.%i. The new version is v%i.%i. Would you like to install this update?";

AdpUpdateDialog::AdpUpdateDialog(SoftwareUpdate& update)
    :update(update)
    ,wxDialog(NULL, -1, "Update availible", wxDefaultPosition, wxDefaultSize)
{
    wxPanel* panel = new wxPanel(this, -1);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    wxString text;
    VersionType currentVersion;
    VersionType newVersion;
    auto pad = Device::Pad();

    switch (update.GetSoftwareType()) {
    case SoftwareType::SW_TYPE_ADP_TOOL:
        currentVersion = Updater::AdpVersion();
        newVersion = update.GetVersion();

        text = wxString::Format(UpdateMsg, "ADP tool", currentVersion.major, currentVersion.minor, newVersion.major, newVersion.minor);
        break;
    case SoftwareType::SW_TYPE_ADP_FIRMWARE:
        
        
        if (pad == NULL) {
            text = "Unknown update";
            break;
        }
        currentVersion = pad->firmwareVersion;
        newVersion = update.GetVersion();

        text = wxString::Format(UpdateMsg, BoardTypeToString(pad->boardType), currentVersion.major, currentVersion.minor, newVersion.major, newVersion.minor);
        break;

    default:
        text = "Unknown update";
        break;
    }

    auto lUpdate = new wxStaticText(this, wxID_ANY, text,
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

    wxButton* yesButton = new wxButton(this, wxID_YES, wxT("Yes"),
        wxDefaultPosition, wxSize(70, 30));
    wxButton* noButton = new wxButton(this, wxID_NO, wxT("No"),
        wxDefaultPosition, wxSize(70, 30));
    wxButton* stopButton = new wxButton(this, STOP_BUTTON, wxT("No, don't ask again"),
        wxDefaultPosition, wxSize(200, 30));

    hbox->Add(yesButton, 1);
    hbox->Add(noButton, 1, wxLEFT, 5);
    hbox->Add(stopButton, 1, wxLEFT, 5);

    vbox->Add(lUpdate);
    vbox->Add(panel, 1);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

    SetSizer(vbox);

    Centre();
}

void AdpUpdateDialog::OnStop(wxCommandEvent& event)
{
    EndModal(STOP_BUTTON);
}

void AdpUpdateDialog::OnYes(wxCommandEvent& event)
{
    EndModal(wxID_YES);
}

void AdpUpdateDialog::OnNo(wxCommandEvent& event)
{
    EndModal(wxID_NO);
}

BEGIN_EVENT_TABLE(AdpUpdateDialog, wxDialog)
    EVT_BUTTON(wxID_YES, AdpUpdateDialog::OnYes)
    EVT_BUTTON(wxID_NO, AdpUpdateDialog::OnNo)
    EVT_BUTTON(STOP_BUTTON, AdpUpdateDialog::OnStop)
END_EVENT_TABLE()
    
void ShowUpdateDialog(SoftwareUpdate update)
{
    AdpUpdateDialog dialog(update);
    int ret = dialog.ShowModal();
    dialog.Destroy();

    if (ret == wxID_YES) {

        update.Install([](bool succes) {
            if (succes) {
                int result = wxMessageBox("The update has installed succesfully, restart now?", L"Update result", wxICON_INFORMATION | wxYES_NO);
                if (result == wxYES) {
                    wxGetApp().Restart();

                }
            }
            else {
                wxMessageBox(L"The update has failed installing", L"Update result", wxICON_ERROR);
            }
        });
    }

}

}; // namespace adp.
