#pragma once

#include "View/BaseTab.h"

#include "wx/dataview.h"
#include "wx/combobox.h"

#include "View/BaseTab.h"

#include <string>
#include <vector>

namespace adp {

class HorizontalSensorBar;

class MappingTab : public BaseTab
{
public:
    static const wchar_t* Title;

    MappingTab(wxWindow* owner, const PadState* pad);

    void HandleChanges(DeviceChanges changes) override;
    void Tick() override;

private:
    std::vector<wxComboBox*> myButtonBoxes;
    std::vector<HorizontalSensorBar*> mySensorBars;

    void OnButtonChanged(wxCommandEvent&);
    void UpdateButtonMapping();
};

}; // namespace adp.
