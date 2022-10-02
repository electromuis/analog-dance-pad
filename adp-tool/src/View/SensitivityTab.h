#pragma once

#include <Adp.h>

namespace adp {

class SensitivityTab
{
public:
    SensitivityTab();
    void Render();

private:
    void RenderSensor(int, float, float, float, float);
    int myAdjustingSensorIndex;
    double myAdjustingSensorThreshold;
};

}; // namespace adp.
