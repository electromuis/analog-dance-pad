#include "Adp.h"

#include "wx/dataview.h"
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#include "Assets/Assets.h"

#include "View/MappingTab.h"

namespace adp {

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
        GetClientSize(&w, &h);

        auto sensor = Device::Sensor(mySensor);
        int barW = sensor ? (sensor->value * w) : 0;

        dc.SetPen(Pens::Black1px());
        dc.SetBrush(Brushes::SensorBar());
        dc.DrawRectangle(barW, 0, w - barW, h);
        dc.SetBrush(Brushes::SensorOn());
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

const wchar_t* MappingTab::Title = L"Mapping";

MappingTab::MappingTab(wxWindow* owner, const PadState* pad)
    : wxWindow(owner, wxID_ANY)
{
    wxArrayString options;
    options.Add(L"-");
    for (int i = 1; i <= pad->numButtons; ++i)
        options.Add(wxString::Format("Button %i", i));

    auto sizer = new wxGridSizer(pad->numSensors, 3, 4, 4);
    for (int i = 0; i < pad->numSensors; ++i)
    {
        auto text = new wxStaticText(this, wxID_ANY, wxString::Format("Sensor %i", i + 1),
            wxDefaultPosition, wxDefaultSize);
        sizer->Add(text, 1, wxLEFT | wxTOP | wxRIGHT | wxEXPAND, 4);

        auto box = new wxComboBox(this, i, options[0],
            wxDefaultPosition, wxDefaultSize, options, wxCB_READONLY);
        box->Bind(wxEVT_COMBOBOX, &MappingTab::OnButtonChanged, this);
        sizer->Add(box, 1, wxEXPAND);
        myButtonBoxes.push_back(box);

        auto bar = new HorizontalSensorBar(this, i);
        sizer->Add(bar, 1, wxEXPAND);
        mySensorBars.push_back(bar);
    }
    UpdateButtonMapping();

    auto outerSizer = new wxBoxSizer(wxVERTICAL | wxALIGN_TOP);
    outerSizer->Add(sizer, 0, wxALL | wxEXPAND, 4);
    SetSizer(outerSizer);
}

void MappingTab::HandleChanges(DeviceChanges changes)
{
    if (changes & DCF_BUTTON_MAPPING)
        UpdateButtonMapping();
}

void MappingTab::Tick()
{
    for (auto bar : mySensorBars)
        bar->Refresh(false);
}

void MappingTab::OnButtonChanged(wxCommandEvent& event)
{
    int sensorIndex = event.GetId();
    auto selectedButton = myButtonBoxes[sensorIndex]->GetSelection();
    Device::SetButtonMapping(sensorIndex, selectedButton);
}

void MappingTab::UpdateButtonMapping()
{
    for (uint32_t i = 0; i < myButtonBoxes.size(); ++i)
    {
        auto sensor = Device::Sensor(i);
        auto button = sensor ? sensor->button : 0;
        myButtonBoxes[i]->SetSelection(button);
    }
}

void MappingTab::SaveToProfile(json& j)
{
    j["mapping"] = json::array();
	for (int i = 0; i < myButtonBoxes.size(); ++i)
    {
		j["mapping"][i] = myButtonBoxes[i]->GetSelection();
	}
}

void MappingTab::LoadFromProfile(json& j)
{
    //string name = j["name"];
	//Device::SetDeviceName(widen(name.c_str(), name.length()).c_str());
}

}; // namespace adp.