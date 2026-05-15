#pragma once

#include <Adp.h>

namespace adp {

class SensitivityTab
{
public:
    SensitivityTab();
    void Render();
    void OnDeviceChanged();

private:
    void RenderSensor(int);
    int myAdjustingSensorIndex;
    double myAdjustingSensorThreshold;
    double myAdjustingSensorReleaseThreshold;
    double myAdjustingRelativeReleaseThreshold;
};

}; // namespace adp.
