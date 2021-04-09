#pragma once

#include "pages/base.h"
#include "devices.h"

#include "wx/dataview.h"
#include "wx/dcbuffer.h"

#include <string>

using namespace devices;
using namespace std;

namespace pages {

class HorizontalSensorBar : public wxWindow
{
public:
    HorizontalSensorBar(wxWindow* owner, int sensor)
        : wxWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBG_STYLE_PAINT)
        , mySensor(sensor)
    {
    }

    void OnPaint(wxPaintEvent& evt)
    {
        int w, h;
        wxBufferedPaintDC dc(this);
        dc.GetSize(&w, &h);

        auto sensor = DeviceManager::Sensor(mySensor);
        int barW = sensor ? (sensor->value * w) : 0;

        dc.SetPen(wxPen(*wxBLACK, 0));
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(barW, 0, w - barW, h);
        dc.SetBrush(*wxYELLOW_BRUSH);
        dc.DrawRectangle(0, 0, barW, h);
    }

    DECLARE_EVENT_TABLE()

protected:
    wxSize DoGetBestSize() const override { return wxSize(100, 24); }

private:
    int mySensor;
};

BEGIN_EVENT_TABLE(HorizontalSensorBar, wxWindow)
    EVT_PAINT(HorizontalSensorBar::OnPaint)
END_EVENT_TABLE()

class MappingPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Mapping"; }

    MappingPage(wxWindow* owner, const PadState* pad) : BasePage(owner)
    {
        wxArrayString options;
        options.Add(L"-");
        for (int i = 1; i <= pad->numButtons; ++i)
            options.Add(wxString::Format("Button %i", i));

        auto view = new wxScrolledWindow(this);

        auto sizer = new wxFlexGridSizer(pad->numSensors, 3, 4, 4);
        sizer->SetFlexibleDirection(wxHORIZONTAL);
        sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE);

        for (int i = 0; i < pad->numSensors; ++i)
        {
            auto text = new wxStaticText(view, wxID_ANY, wxString::Format("Sensor %i", i + 1));
            sizer->Add(text, 0, wxLEFT | wxTOP | wxRIGHT, 4);

            auto box = new wxComboBox(view, i, options[0], wxDefaultPosition, wxSize(100, 24), options, wxCB_READONLY);
            box->Bind(wxEVT_COMBOBOX, &MappingPage::OnButtonChanged, this);
            sizer->Add(box);
            myButtonBoxes.push_back(box);

            auto bar = new HorizontalSensorBar(view, i);
            sizer->Add(bar);
            mySensorBars.push_back(bar);
        }
        view->SetSizer(sizer);
        view->FitInside();
        view->SetScrollRate(5, 5);

        UpdateButtonMapping();

        auto outerSizer = new wxBoxSizer(wxVERTICAL);
        outerSizer->Add(view, 1, wxTOP | wxLEFT | wxEXPAND, 4);
        SetSizer(outerSizer);
    }

    void Tick(devices::DeviceChanges changes) override
    {
        for (auto bar : mySensorBars)
            bar->Refresh(false);
    }

private:
    vector<wxComboBox*> myButtonBoxes;
    vector<HorizontalSensorBar*> mySensorBars;

    void OnButtonChanged(wxCommandEvent& event)
    {
        int sensorIndex = event.GetId();
        auto selectedButton = myButtonBoxes[sensorIndex]->GetSelection();
        DeviceManager::SetButtonMapping(sensorIndex, selectedButton);
        UpdateButtonMapping();
    }

    void UpdateButtonMapping()
    {
        for (uint32_t i = 0; i < myButtonBoxes.size(); ++i)
        {
            auto sensor = DeviceManager::Sensor(i);
            auto button = sensor ? sensor->button : 0;
            myButtonBoxes[i]->SetSelection(button);
        }
    }
};

}; // namespace pages.