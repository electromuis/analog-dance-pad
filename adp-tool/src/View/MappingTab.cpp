#include "Adp.h"

#include "wx/dataview.h"
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/button.h"

#include "Assets/Assets.h"

#include "View/MappingTab.h"

namespace adp {

enum Ids { DIALOG_SAVE_BUTTON = 1, SENSOR_CONFIG_BUTTON = 2 };

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

    auto sizer = new wxGridSizer(pad->numSensors, 4, 4, 4);
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

        auto configButton = new wxButton(this, i, "Config");
        configButton->Bind(wxEVT_BUTTON, &MappingTab::OnSensorConfig, this);
        sizer->Add(configButton, 1, wxEXPAND);
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

void MappingTab::OnSensorConfig(wxCommandEvent& event)
{
    SensorConfigDialog dialog(event.GetId());
    dialog.ShowModal();
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

SensorConfigDialog::SensorConfigDialog(int sensorNumber)
     : wxDialog(NULL, -1, "Sensor configuration", wxDefaultPosition, wxSize(600, 300)),
    sensorNumber(sensorNumber)
{
    auto mainSizer = new wxBoxSizer(wxVERTICAL);
    auto topSizer = new wxBoxSizer(wxVERTICAL);

    sensorBar = new HorizontalSensorBar(this, sensorNumber);
    topSizer->Add(sensorBar, 1, wxEXPAND | wxBOTTOM, 5);

    auto adc = Device::Adc(sensorNumber);

    wxArrayString options;
    options.Add(L"5v");
    options.Add(L"3v");
    arefSelection = new wxComboBox(this, NULL, adc->aref3 ? "3v" : "5v", wxDefaultPosition, wxDefaultSize, options);
    arefSelection->Bind(wxEVT_COMBOBOX, &SensorConfigDialog::Save, this);
    topSizer->Add(arefSelection, 1, wxEXPAND | wxBOTTOM, 5);

    resistorSlider = new wxSlider(this, NULL, adc->resistorValue, 0, 254, wxDefaultPosition, wxDefaultSize);
    resistorSlider->Bind(wxEVT_SLIDER, &SensorConfigDialog::Save, this);
    topSizer->Add(resistorSlider, 1, wxEXPAND | wxBOTTOM, 5);

    auto doneButton = new wxButton(this, wxID_ANY, L"Save", wxDefaultPosition, wxSize(200, -1));
    doneButton->Bind(wxEVT_BUTTON, &SensorConfigDialog::Done, this);
    topSizer->Add(doneButton, 1, wxEXPAND | wxBOTTOM, 5);

    mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 5);
    SetSizerAndFit(mainSizer);

    updateTimer = new wxTimer();
    updateTimer->Bind(wxEVT_TIMER, &SensorConfigDialog::Tick, this);
    updateTimer->Start(10);
}

SensorConfigDialog::~SensorConfigDialog()
{
    updateTimer->Stop();
    delete updateTimer;
}

void SensorConfigDialog::Tick(wxTimerEvent& event)
{
    sensorBar->Refresh(false);
}

void SensorConfigDialog::Done(wxCommandEvent& event)
{
    Save(event);
    Close();
}

void SensorConfigDialog::Save(wxCommandEvent& event)
{
    AdcState adc;

    if (arefSelection->GetValue() == "3v") {
        adc.aref3 = true;
        adc.aref5 = false;
    }
    else {
        adc.aref3 = false;
        adc.aref5 = true;
    }

    adc.resistorValue = resistorSlider->GetValue();
    adc.disabled = false;
    adc.setResistor = true;

    Device::SendAdcConfig(sensorNumber, adc);
}

}; // namespace adp.