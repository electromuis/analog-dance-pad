#pragma once

#include <string>
#include <vector>

#include "wx/window.h"
#include "wx/dataview.h"
#include "wx/combobox.h"
#include "wx/dialog.h"
#include "wx/slider.h"
#include "wx/timer.h"

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
    void OnSensorConfig(wxCommandEvent&);
    void UpdateButtonMapping();
};

class SensorConfigDialog : public wxDialog
{
public:
    SensorConfigDialog(int sensorNumber);
    ~SensorConfigDialog();

    void Save(wxCommandEvent& event);
    void Done(wxCommandEvent& event);
    void Tick(wxTimerEvent& event);

private:
    HorizontalSensorBar* sensorBar;
    wxComboBox* arefSelection;
    wxSlider* resistorSlider;
    int sensorNumber;
    wxTimer* updateTimer;
};

}; // namespace adp.
