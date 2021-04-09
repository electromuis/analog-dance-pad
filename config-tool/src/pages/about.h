#pragma once

#include "pages/base.h"
#include "logo.h"

#include "wx/mstream.h"
#include "wx/generic/statbmpg.h"

namespace pages {

class AboutPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"About"; }

    AboutPage(wxWindow* owner) : BasePage(owner)
    {
        wxMemoryInputStream stream(LOGO_PNG, LOGO_PNG_SIZE);
        wxImage logo(stream, wxBITMAP_TYPE_PNG);

        auto topText = new wxStaticText(this, wxID_ANY, L"FSR Mini Pad Config Tool");
        auto bottomText = new wxStaticText(this, wxID_ANY, L"\xA9 Bram van de Wetering 2021");
        auto logoBitmap = new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(logo));

        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(topText, 0, wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);
        sizer->Add(logoBitmap, 0, wxALIGN_CENTER_HORIZONTAL);
        sizer->Add(bottomText, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 10);
        sizer->AddStretchSpacer();

        SetSizer(sizer);
    }
};

}; // namespace pages.