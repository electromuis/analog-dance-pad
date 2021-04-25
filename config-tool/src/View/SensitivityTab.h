#pragma once

#include "wx/sizer.h"
#include "wx/slider.h"

#include "View/BaseTab.h"

#include <vector>

using namespace std;

namespace adp {

class SensorDisplay;

class SensitivityTab : public BaseTab
{
public:
    static const wchar_t* Title;

    SensitivityTab(wxWindow* owner, const PadState* pad);

    void HandleChanges(DeviceChanges changes) override;
    void Tick() override;

    void OnReleaseThresholdChanged(wxCommandEvent& event);

private:
    void UpdateDisplays();

    vector<SensorDisplay*> mySensorDisplays;
    wxSlider* myReleaseThresholdSlider;
    wxBoxSizer* mySensorSizer;
    bool myIsUpdatingReleaseThreshold;
};

}; // namespace adp.
