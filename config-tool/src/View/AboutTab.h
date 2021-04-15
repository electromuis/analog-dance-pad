#pragma once

#include "View/BaseTab.h"

namespace mpc {

class AboutTab : public BaseTab
{
public:
    static const wchar_t* Title;

    AboutTab(wxWindow* owner);
};

}; // namespace mpc.