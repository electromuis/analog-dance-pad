#pragma once

#include "wx/window.h"

#include "Model/Device.h"

namespace adp {

class BaseTab : public wxWindow
{
public:
    BaseTab(wxWindow* owner) : wxWindow(owner, wxID_ANY) {}

    virtual ~BaseTab() {}

    virtual void HandleChanges(DeviceChanges changes) {}

    virtual void Tick() {}
};

}; // namespace adp.
