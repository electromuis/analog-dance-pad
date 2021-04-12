#pragma once

#include "View/BaseTab.h"

namespace mpc {

class AboutPage : public BaseTab
{
public:
    static const wchar_t* Title;

    AboutPage(wxWindow* owner);
};

}; // namespace mpc.