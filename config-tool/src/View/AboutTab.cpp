#include "wx/mstream.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/generic/statbmpg.h"

#include "Assets/Assets.h"

#include "View/AboutTab.h"

namespace adp {

const wchar_t* AboutTab::Title = L"About";

AboutTab::AboutTab(wxWindow* owner)
    : wxWindow(owner, wxID_ANY)
{
    wxImage logo(Files::Icon64(), wxBITMAP_TYPE_PNG);

    auto logoBitmap = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(logo));
    auto topText = new wxStaticText(this, wxID_ANY, L"FSR Mini Pad Config Tool");
    auto bottomText = new wxStaticText(this, wxID_ANY, L"\xA9 Bram van de Wetering 2021");

    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();
    sizer->Add(logoBitmap, 0, wxALIGN_CENTER_HORIZONTAL);
    sizer->Add(topText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 10);
    sizer->Add(bottomText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 5);
    sizer->AddStretchSpacer();

    SetSizer(sizer);
}

}; // namespace adp.