#pragma once

#include "pages/base.h"
#include "logging.h"

using namespace logging;

namespace pages {

class LogPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Log"; }

    LogPage(wxWindow* owner) : BasePage(owner)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        myText = new wxTextCtrl(
            this,
            wxID_ANY,
            wxEmptyString,
            wxDefaultPosition,
            wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE);
        wxFont font = wxFont(9, wxFontFamily::wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        myText->SetFont(font);
        sizer->Add(myText, 1, wxEXPAND);
        SetSizer(sizer);
    }

    void Tick(devices::DeviceChanges changes) override
    {
        for (int numMessages = Log::NumMessages(); myNumMessages < numMessages; ++myNumMessages)
        {
            if (myNumMessages > 0)
                myText->AppendText(L"\n");
            myText->AppendText(Log::Message(myNumMessages));
        }
    }

private:
    wxTextCtrl* myText;
    int myNumMessages = 0;
};

}; // namespace pages.