#pragma once

#include <string>
#include <vector>

#include "wx/window.h"
#include "wx/dataview.h"
#include "wx/combobox.h"

#include "View/BaseTab.h"

namespace adp {

class HorizontalSensorBar;

class MappingTab : public BaseTab, public wxWindow
{
public:
    static const wchar_t* Title;

    MappingTab(wxWindow* owner, const PadState* pad);

    void HandleChanges(DeviceChanges changes) override;
    void Tick() override;

    wxWindow* GetWindow() override { return this; }

private:
    std::vector<wxComboBox*> myButtonBoxes;
    std::vector<HorizontalSensorBar*> mySensorBars;

    void OnButtonChanged(wxCommandEvent&);
    void UpdateButtonMapping();
};

}; // namespace adp.
