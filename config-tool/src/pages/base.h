#pragma once

#include "wx/wxprec.h"

namespace pages {

class BasePage : public wxWindow
{
public:
    BasePage(wxWindow* owner) : wxWindow(owner, wxID_ANY) {}

    virtual ~BasePage() {}

    virtual void Tick() {}
};

}; // namespace pages.