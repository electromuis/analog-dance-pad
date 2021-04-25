#pragma once

#include "View/BaseTab.h"

namespace adp {

class AboutTab : public BaseTab
{
public:
    static const wchar_t* Title;

    AboutTab(wxWindow* owner);
};

}; // namespace adp.