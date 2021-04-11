#pragma once

#include "pages/base.h"
#include "devices.h"
#include "style.h"

#include "wx/dcbuffer.h"
#include "wx/utils.h"

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
        wxBufferedPaintDC dc(this);
        auto size = GetClientSize();

        auto sensor = DeviceManager::Sensor(mySensor);
        int barH = sensor ? (sensor->value * size.y) : 0;
        auto threshold = sensor ? sensor->threshold : 0;
        auto thresholdY = size.y * (1.0 - threshold);
        bool pressed = sensor ? sensor->pressed : false;

        dc.SetPen(style::Pens::Black1px());
        dc.SetBrush(style::Brushes::SensorBar());
        dc.DrawRectangle(0, 0, size.x, size.y - barH);
        dc.SetBrush(pressed ? style::Brushes::SensorOn() : style::Brushes::SensorOff());
        dc.DrawRectangle(0, size.y - barH, size.x, barH);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(0, thresholdY - 1, size.x, 3);

        auto sensitivityText = wxString::Format("%i%%", (int)std::lround(threshold * 100.0));
        auto rect = wxRect(size.x / 2 - 20, 5, 40, 20);
        dc.SetBrush(style::Brushes::DarkGray());
        dc.DrawRectangle(rect);
        dc.SetTextForeground(*wxWHITE);
        dc.DrawLabel(sensitivityText, rect, wxALIGN_CENTER);
    }

    void OnClick(wxMouseEvent& event)
    {
        auto pos = event.GetPosition();
        auto rect = GetClientRect();
        if (rect.Contains(pos))
        {
            double threshold = 1.0 - double(pos.y - rect.y) / max(1.0, (double)rect.height);
            DeviceManager::SetThreshold(mySensor, threshold);
        }
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
    EVT_LEFT_DOWN(SensorDisplay::OnClick)
END_EVENT_TABLE()

class SensitivityPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Sensitivity"; }

    SensitivityPage(wxWindow* owner, const PadState* pad) : BasePage(owner)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        auto topLabel = new wxStaticText(this, wxID_ANY, myInstructionMsg);
        sizer->Add(topLabel, 0, wxLEFT | wxRIGHT | wxTOP, 4);

        mySensorSizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(mySensorSizer, 1, wxEXPAND);
        UpdateDisplays();

        SetSizer(sizer);
    }

    void Tick(devices::DeviceChanges changes) override
    {
        if (changes & DCF_BUTTON_MAPPING)
            UpdateDisplays();

        auto mouseState = wxGetMouseState();
        for (auto display : mySensorDisplays)
            display->Refresh(false);
    }

private:
    inline static constexpr const wchar_t* myInstructionMsg =
        L"Click inside a sensor bar to adjust its sensitivity threshold.";

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
            mySensorSizer->Clear(true);
            mySensorDisplays.clear();
            for (auto sensor : sensors)
            {
                auto display = new SensorDisplay(this);
                mySensorDisplays.push_back(display);
                mySensorSizer->Add(display, 1, wxALL | wxEXPAND, 4);
            }
        }
        
        for (size_t i = 0; i < sensors.size(); ++i)
        {
            auto [sensor, button] = sensors[i];
            mySensorDisplays[i]->SetSensor(sensor, button);
        }
    }

    vector<SensorDisplay*> mySensorDisplays;
    wxBoxSizer* mySensorSizer;
};

}; // namespace pages.