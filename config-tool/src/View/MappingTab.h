#pragma once

#include "View/BaseTab.h"

#include "wx/dataview.h"
#include "wx/combobox.h"

#include "View/BaseTab.h"

#include <string>
#include <vector>

namespace mpc {

class HorizontalSensorBar;

class MappingPage : public BaseTab
{
public:
    static const wchar_t* Title;

    MappingPage(wxWindow* owner, const PadState* pad);

    void Tick(DeviceChanges changes) override;

private:
    std::vector<wxComboBox*> myButtonBoxes;
    std::vector<HorizontalSensorBar*> mySensorBars;

    void OnButtonChanged(wxCommandEvent&);
    void UpdateButtonMapping();
};

}; // namespace mpc.
