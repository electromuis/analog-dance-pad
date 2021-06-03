#pragma once

#include "wx/window.h"
#include "wx/sizer.h"
#include "wx/slider.h"

#include "View/BaseTab.h"

#include <vector>

using namespace std;

namespace adp {

class SensorDisplay;

class SensitivityTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    SensitivityTab(wxWindow* owner, const PadState* pad);

    void HandleChanges(DeviceChanges changes) override;
    void Tick() override;

    double ReleaseThreshold() const;

    void OnReleaseThresholdChanged(wxCommandEvent& event);

    wxWindow* GetWindow() override { return this; }

private:
    void UpdateDisplays();

    vector<SensorDisplay*> mySensorDisplays;
    wxSlider* myReleaseThresholdSlider;
    wxBoxSizer* mySensorSizer;
    double myReleaseThreshold = 1.0;
    bool myIsAdjustingReleaseThreshold = false;
};

}; // namespace adp.
