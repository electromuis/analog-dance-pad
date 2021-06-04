#include "wx/mstream.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/generic/statbmpg.h"

#include "Assets/Assets.h"

#include "View/AboutTab.h"

namespace adp {

const wchar_t* AboutTab::Title = L"About";

AboutTab::AboutTab(wxWindow* owner, const wchar_t* versionString)
    : wxWindow(owner, wxID_ANY)
{
    //todo fix linux

#ifdef _MSC_VER
    wxImage logo(Files::Icon64(), wxBITMAP_TYPE_PNG);
    auto logoBitmap = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(logo));
#endif

    auto topText = new wxStaticText(this, wxID_ANY, versionString);
    auto bottomText = new wxStaticText(this, wxID_ANY, L"\xA9 Bram van de Wetering 2021");
    auto underBottomText = new wxStaticText(this, wxID_ANY, L"\xA9 DDR-EXP");

    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();
#ifdef _MSC_VER
    sizer->Add(logoBitmap, 0, wxALIGN_CENTER_HORIZONTAL);
#endif
    sizer->Add(topText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 10);
    sizer->Add(bottomText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 5);
    sizer->Add(underBottomText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 5);
    sizer->AddStretchSpacer();

    SetSizer(sizer);


}

}; // namespace adp.
