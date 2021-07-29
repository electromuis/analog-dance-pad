#pragma once

#include "wx/scrolwin.h"

#include "View/BaseTab.h"

namespace adp {

class LightSettingsPanel;

class LightsTab : public BaseTab, public wxScrolledWindow
{
public:
    static const wchar_t* Title;

    LightsTab(wxWindow* owner, const LightsState* lights);

    void UpdateSettings(const LightsState* lights);
    void HandleChanges(DeviceChanges changes) override;
    void DeleteLightSetting(LightSettingsPanel* panel);
    void OnAddLightSetting(wxCommandEvent& event);
    void OnResize(wxSizeEvent& event);

    void ReindexAll();

    wxWindow* GetWindow() override { return this; }

    void RecomputeLayout();

    DECLARE_EVENT_TABLE()

private:
    wxButton* myAddSettingButton;
    std::vector<LightSettingsPanel*> myLightSettings;
    bool myTemporarilyIgnoreChanges = false;
};

}; // namespace adp.
