#include "wx/sizer.h"
#include "wx/stattext.h"

#include "View/LightsTab.h"

namespace adp {

const wchar_t* LightsTab::Title = L"Lights";

LightsTab::LightsTab(wxWindow* owner) : BaseTab(owner)
{
    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    auto text = new wxStaticText(
        this,
        wxID_ANY,
        "Lights",
        wxDefaultPosition,
        wxDefaultSize,
        wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    sizer->Add(text, 1, wxCENTER);
    SetSizer(sizer);
}

}; // namespace adp.
