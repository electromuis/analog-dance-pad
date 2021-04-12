#pragma once

#include "View/BaseTab.h"

namespace mpc {

class IdlePage : public BaseTab
{
public:
    static const wchar_t* Title;

    IdlePage(wxWindow* owner);
};

}; // namespace mpc.
