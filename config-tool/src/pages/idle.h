#pragma once

#include "pages/base.h"

namespace pages {

class IdlePage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Idle"; }

    IdlePage(wxWindow* owner) : BasePage(owner)
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
};

}; // namespace pages.