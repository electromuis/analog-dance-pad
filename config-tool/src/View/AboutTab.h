#pragma once

#include "wx/window.h"

#include "View/BaseTab.h"

namespace adp {

class AboutTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    AboutTab(wxWindow* owner);

    wxWindow* GetWindow() override { return this; }
};

}; // namespace adp.