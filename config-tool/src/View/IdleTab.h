#pragma once

#include "View/BaseTab.h"

namespace mpc {

class IdleTab : public BaseTab
{
public:
    static const wchar_t* Title;

    IdleTab(wxWindow* owner);
};

}; // namespace mpc.
