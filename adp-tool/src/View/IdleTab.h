#pragma once

#include "View/BaseTab.h"

namespace adp {

class IdleTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    IdleTab(wxWindow* owner);

    wxWindow* GetWindow() override { return this; }
};

}; // namespace adp.
