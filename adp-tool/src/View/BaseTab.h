#pragma once

#include "wx/window.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Model/Device.h"

namespace adp {

class BaseTab
{
public:
    virtual ~BaseTab() {}

    virtual wxWindow* GetWindow() = 0;

    virtual void HandleChanges(DeviceChanges changes) {}

    virtual void Tick() {}

    virtual void SaveToProfile(json& j) {}

    virtual void LoadFromProfile(json& j) {}
};

}; // namespace adp.
