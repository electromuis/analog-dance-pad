#pragma once

#include "wx/window.h"

#include "Model/Devices.h"

namespace mpc {

class BaseTab : public wxWindow
{
public:
    BaseTab(wxWindow* owner) : wxWindow(owner, wxID_ANY) {}

    virtual ~BaseTab() {}

    virtual void Tick(DeviceChanges changes) {}
};

}; // namespace mpc.
