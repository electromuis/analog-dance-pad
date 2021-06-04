#pragma once

#include "wx/window.h"
#include "wx/textctrl.h"

#include "View/BaseTab.h"

namespace adp {

class LogTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    LogTab(wxWindow* owner);

    void Tick() override;

    wxWindow* GetWindow() override { return this; }

private:
    wxTextCtrl* myText;
    int myNumMessages = 0;
};

}; // namespace adp.
