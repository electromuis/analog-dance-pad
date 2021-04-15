#pragma once

#include "wx/textctrl.h"

#include "View/BaseTab.h"

namespace mpc {

class LogTab : public BaseTab
{
public:
    static const wchar_t* Title;

    LogTab(wxWindow* owner);

    void Tick(DeviceChanges changes) override;

private:
    wxTextCtrl* myText;
    int myNumMessages = 0;
};

}; // namespace mpc.
