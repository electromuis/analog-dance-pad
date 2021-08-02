#pragma once

#include "wx/scrolwin.h"

#include "View/BaseTab.h"

namespace adp {

class LightRulePanel;

class LightsTab : public BaseTab, public wxScrolledWindow
{
public:
    struct Change;

    static const wchar_t* Title;

    LightsTab(wxWindow* owner, const LightsState* state);

    void UpdateSettings(const LightsState* state);
    void HandleChanges(DeviceChanges changes) override;
    void OnAddLightRuleRequested(wxCommandEvent& event);
    void OnResize(wxSizeEvent& event);

    void ProcessChange(const Change& change);

    wxWindow* GetWindow() override { return this; }

    void RecomputeLayout();

    DECLARE_EVENT_TABLE()

private:
    wxButton* myAddSettingButton;
    std::vector<LightRulePanel*> myLightRules;
};

}; // namespace adp.
