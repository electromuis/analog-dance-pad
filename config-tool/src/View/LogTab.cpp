#include "wx/sizer.h"

#include "View/LogTab.h"

#include "Model/Log.h"

namespace adp {

const wchar_t* LogTab::Title = L"Log";

LogTab::LogTab(wxWindow* owner) : BaseTab(owner)
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

void LogTab::Tick()
{
    for (int numMessages = Log::NumMessages(); myNumMessages < numMessages; ++myNumMessages)
    {
        if (myNumMessages > 0)
            myText->AppendText(L"\n");
        myText->AppendText(Log::Message(myNumMessages));
    }
}

}; // namespace adp.
