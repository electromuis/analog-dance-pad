#pragma once

#include "View/BaseTab.h"

namespace adp {

class LightsTab : public BaseTab
{
public:
    static const wchar_t* Title;

    LightsTab(wxWindow* owner);
};

}; // namespace adp.
