#pragma once

#include "wx/window.h"

#include "Model/Device.h"

namespace adp {

class BaseTab
{
public:
    virtual ~BaseTab() {}

    virtual wxWindow* GetWindow() = 0;

    virtual void HandleChanges(DeviceChanges changes) {}

    virtual void Tick() {}
};

}; // namespace adp.
