#pragma once

#include "wx/scrolwin.h"

#include "View/BaseTab.h"

namespace adp {

class LightsTab : public BaseTab, public wxScrolledWindow
{
public:
    static const wchar_t* Title;

    LightsTab(wxWindow* owner);

    void DeleteLightSettings(int index);

    void OnAddLightSettings(wxCommandEvent& event);

    wxWindow* GetWindow() override { return this; }

    DECLARE_EVENT_TABLE()

private:
    int myNumLightSettings;
};

}; // namespace adp.
