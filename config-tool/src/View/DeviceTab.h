#pragma once

#include "wx/sizer.h"
#include "wx/stattext.h"

#include "View/BaseTab.h"

namespace mpc {

class DevicePage : public BaseTab
{
public:
    static const wchar_t* Title;

    DevicePage(wxWindow* owner);

    void OnRename(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()

private:
    wxSizer* mySizer;
    wxStaticText* myRenameLabel;
    wxStaticText* myResetLabel;
};

}; // namespace mpc.
