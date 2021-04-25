#pragma once

#include "View/BaseTab.h"

namespace adp {

class IdleTab : public BaseTab
{
public:
    static const wchar_t* Title;

    IdleTab(wxWindow* owner);
};

}; // namespace adp.
