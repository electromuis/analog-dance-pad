#include "wx/sizer.h"
#include "wx/stattext.h"

#include "View/IdleTab.h"

namespace mpc {

const wchar_t* IdlePage::Title = L"Idle";

IdlePage::IdlePage(wxWindow* owner) : BaseTab(owner)
{
    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    auto text = new wxStaticText(
        this,
        wxID_ANY,
        "Connect an FSR Mini Pad to continue.",
        wxDefaultPosition,
        wxDefaultSize,
        wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    sizer->Add(text, 1, wxCENTER);
    SetSizer(sizer);
}

}; // namespace mpc.
