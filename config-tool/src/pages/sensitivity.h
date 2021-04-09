#pragma once

#include "pages/base.h"
#include "devices.h"

#include "wx/dcbuffer.h"

#include <vector>

using namespace devices;
using namespace std;

namespace pages {

class SensorDisplay : public wxWindow
{
public:
    SensorDisplay(wxWindow* owner)
        : wxWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBG_STYLE_PAINT | wxFULL_REPAINT_ON_RESIZE)
    {
        SetMinSize(wxSize(10, 50));
        SetMaxSize(wxSize(500, 1000));
    }

    void OnPaint(wxPaintEvent& evt)
    {
        int w, h;
        wxBufferedPaintDC dc(this);
        dc.GetSize(&w, &h);

        auto sensor = DeviceManager::Sensor(mySensor);
        int barH = sensor ? (sensor->value * h) : 0;
        int thresholdH = sensor ? (sensor->threshold * h) : 0;
        bool pressed = sensor ? sensor->pressed : false;

        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(0, 0, w, h - barH);
        dc.SetBrush(pressed ? *wxCYAN_BRUSH : *wxYELLOW_BRUSH);
        dc.DrawRectangle(0, h - barH, w, barH);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(0, h - thresholdH - 1, w, 3);
    }

    void SetSensor(int index, int button)
    {
        mySensor = index;
        myButton = button;
    }

    int button() const { return myButton; }

    DECLARE_EVENT_TABLE()

protected:
    wxSize DoGetBestSize() const override { return wxSize(20, 100); }

private:
    int mySensor = 0;
    int myButton = 0;
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
        UpdateDisplays();
    }

    void Tick(devices::DeviceChanges changes) override
    {
        if (changes & DCF_BUTTON_MAPPING)
            UpdateDisplays();

        for (auto display : mySensorDisplays)
            display->Refresh(false);
    }

private:
    void UpdateDisplays()
    {
        vector<tuple<int, int>> sensors;

        auto pad = DeviceManager::Pad();
        if (pad)
        {
            for (int i = 0; i < pad->numSensors; ++i)
            {
                auto sensor = DeviceManager::Sensor(i);
                if (sensor->button != 0)
                    sensors.push_back(make_tuple(i, sensor->button));
            }
        }

        std::stable_sort(sensors.begin(), sensors.end(), [](tuple<int, int> l, tuple<int, int> r) {
            return get<1>(l) < get<1>(r);
        });

        if (sensors.size() != mySensorDisplays.size())
        {
            auto sizer = new wxBoxSizer(wxHORIZONTAL);
            mySensorDisplays.clear();
            for (auto sensor : sensors)
            {
                auto display = new SensorDisplay(this);
                mySensorDisplays.push_back(display);
                sizer->Add(display, 1, wxALL | wxEXPAND, 4);
            }
            auto vsizer = new wxBoxSizer(wxVERTICAL);
            vsizer->Add(sizer, 1, wxALL, 4);
            SetSizer(vsizer);
        }
        
        for (size_t i = 0; i < sensors.size(); ++i)
        {
            auto [sensor, button] = sensors[i];
            mySensorDisplays[i]->SetSensor(sensor, button);
        }
    }

    vector<SensorDisplay*> mySensorDisplays;
};

}; // namespace pages.