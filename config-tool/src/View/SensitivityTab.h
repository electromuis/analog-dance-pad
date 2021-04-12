#pragma once

#include "wx/sizer.h"
#include "wx/slider.h"

#include "View/BaseTab.h"

#include <vector>

using namespace std;

namespace mpc {

class SensorDisplay;

class SensitivityPage : public BaseTab
{
public:
    static const wchar_t* Title;

    SensitivityPage(wxWindow* owner, const PadState* pad);

    void Tick(DeviceChanges changes) override;

    void OnReleaseThresholdChanged(wxCommandEvent& event);

private:
    void UpdateDisplays();

    vector<SensorDisplay*> mySensorDisplays;
    wxSlider* myReleaseThresholdSlider;
    wxBoxSizer* mySensorSizer;
};

}; // namespace mpc.
