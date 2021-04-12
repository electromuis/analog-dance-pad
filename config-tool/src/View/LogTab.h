#pragma once

#include "wx/textctrl.h"

#include "View/BaseTab.h"

namespace mpc {

class LogPage : public BaseTab
{
public:
    static const wchar_t* Title;

    LogPage(wxWindow* owner);

    void Tick(DeviceChanges changes) override;

private:
    wxTextCtrl* myText;
    int myNumMessages = 0;
};

}; // namespace mpc.
