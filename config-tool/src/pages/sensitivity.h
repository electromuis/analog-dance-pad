#pragma once

#include "pages/base.h"
#include "devices.h"

#include <vector>

using namespace devices;
using namespace std;

namespace pages {

class SensorDisplay : public wxWindow
{
public:
    SensorDisplay(wxWindow* owner, int sensor) : wxWindow(owner, wxID_ANY)
    {
    }

    void OnPaint(wxPaintEvent& evt)
    {
        auto sensor = DeviceManager::Sensor(mySensor);
        if (!sensor)
            return;

        int w, h;
        wxPaintDC dc(this);
        dc.GetSize(&w, &h);

        int barH = sensor->value * h;
        int thresholdH = sensor->threshold * h;

        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(0, 0, 0, 100 - barH);
        dc.SetBrush(sensor->pressed ? *wxLIGHT_GREY_BRUSH : *wxYELLOW_BRUSH);
        dc.DrawRectangle(0, 0, 100 - barH, barH);
        dc.SetBrush(*wxRED);
        dc.DrawRectangle(0, 0, 100 - thresholdH, 1);
    }

    DECLARE_EVENT_TABLE()

private:
    int mySensor;
};

BEGIN_EVENT_TABLE(SensorDisplay, wxWindow)
    EVT_PAINT(SensorDisplay::OnPaint)
END_EVENT_TABLE()

class SensitivityPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Sensitivity"; }

    SensitivityPage(wxWindow* owner, const PadState* pad) : BasePage(owner)
    {
        for (int i = 0; i < pad->numSensors; ++i)
        {
        }
    }

    void Tick(devices::DeviceChanges changes)
    {
        Refresh();
    }

    DECLARE_EVENT_TABLE()

private:
    vector<SensorDisplay*> mySensors;
};

BEGIN_EVENT_TABLE(SensitivityPage, wxWindow)
    EVT_PAINT(SensitivityPage::OnPaint)
END_EVENT_TABLE()

}; // namespace pages.