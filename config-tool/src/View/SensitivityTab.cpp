#include "wx/dcbuffer.h"
#include "wx/stattext.h"

#include "Assets/Assets.h"

#include "View/SensitivityTab.h"

using namespace std;

namespace adp {

class SensorDisplay : public wxWindow
{
public:
    SensorDisplay(wxWindow* owner)
        : wxWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBG_STYLE_PAINT | wxFULL_REPAINT_ON_RESIZE)
    {
        SetMinSize(wxSize(10, 50));
        SetMaxSize(wxSize(500, 1000));
    }

    void Tick()
    {
        if (myIsBeingDragged)
        {
            auto mouse = wxGetMouseState();
            auto rect = GetScreenRect();
            myThreshold = clamp(1.0 - double(mouse.GetY() - rect.y) / max(1.0, (double)rect.height), 0.0, 1.0);
            if (!mouse.LeftIsDown())
            {
                Device::SetThreshold(mySensor, myThreshold);
                myIsBeingDragged = false;
            }
        }
        else
        {
            auto sensor = Device::Sensor(mySensor);
            myThreshold = sensor ? sensor->threshold : 0.0;
        }
    }

    void OnPaint(wxPaintEvent& evt)
    {
        wxBufferedPaintDC dc(this);
        auto size = GetClientSize();

        auto sensor = Device::Sensor(mySensor);
        int barH = sensor ? (sensor->value * size.y) : 0;
        auto thresholdY = size.y * (1.0 - myThreshold);
        bool pressed = sensor ? sensor->pressed : false;

        dc.SetPen(Pens::Black1px());
        dc.SetBrush(Brushes::SensorBar());
        dc.DrawRectangle(0, 0, size.x, size.y - barH);
        dc.SetBrush(pressed ? Brushes::SensorOn() : Brushes::SensorOff());
        dc.DrawRectangle(0, size.y - barH, size.x, barH);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(0, thresholdY - 1, size.x, 3);

        auto sensitivityText = wxString::Format("%i%%", (int)std::lround(myThreshold * 100.0));
        auto rect = wxRect(size.x / 2 - 20, 5, 40, 20);
        dc.SetBrush(Brushes::DarkGray());
        dc.DrawRectangle(rect);
        dc.SetTextForeground(*wxWHITE);
        dc.DrawLabel(sensitivityText, rect, wxALIGN_CENTER);
    }

    void OnClick(wxMouseEvent& event)
    {
        auto pos = event.GetPosition();
        auto rect = GetClientRect();
        if (rect.Contains(pos))
            myIsBeingDragged = true;
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
    double myThreshold = 0.0;
    bool myIsBeingDragged = false;
};

BEGIN_EVENT_TABLE(SensorDisplay, wxWindow)
    EVT_PAINT(SensorDisplay::OnPaint)
    EVT_LEFT_DOWN(SensorDisplay::OnClick)
    EVT_LEFT_UP(SensorDisplay::OnClick)
END_EVENT_TABLE()

static const wchar_t* ActivationMsg =
    L"Click inside sensor bars to adjust their activation threshold.";

static const wchar_t* ReleaseMsg =
    L"Adjust release threshold (percentage of activation threshold).";

const wchar_t* SensitivityTab::Title = L"Sensitivity";

SensitivityTab::SensitivityTab(wxWindow* owner, const PadState* pad) : BaseTab(owner)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto activationLabel = new wxStaticText(this, wxID_ANY, ActivationMsg);
    sizer->Add(activationLabel, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 4);

    mySensorSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(mySensorSizer, 1, wxEXPAND);
    UpdateDisplays();

    auto releaseLabel = new wxStaticText(this, wxID_ANY, ReleaseMsg);
    sizer->Add(releaseLabel, 0, wxALIGN_CENTER_HORIZONTAL);

    myReleaseThresholdSlider = new wxSlider(this, wxID_ANY, 90, 1, 100,
        wxDefaultPosition, wxSize(300, -1), wxSL_VALUE_LABEL |  wxSL_TOP | wxSL_BOTH);

    myReleaseThresholdSlider->Bind(wxEVT_SLIDER, &SensitivityTab::OnReleaseThresholdChanged, this);
    sizer->Add(myReleaseThresholdSlider, 0, wxALIGN_CENTER_HORIZONTAL);

    SetSizer(sizer);

    myIsUpdatingReleaseThreshold = false;
}

void SensitivityTab::HandleChanges(DeviceChanges changes)
{
    if (changes & DCF_BUTTON_MAPPING)
        UpdateDisplays();
}

void SensitivityTab::Tick()

    auto mouseState = wxGetMouseState();
    for (auto display : mySensorDisplays)
    {
        display->Tick();
        display->Refresh(false);
    }

    if (myIsUpdatingReleaseThreshold && !wxGetMouseState().LeftIsDown())
    {
        auto value = myReleaseThresholdSlider->GetValue();
        Device::SetReleaseThreshold(value * 0.01);
        myIsUpdatingReleaseThreshold = false;
    }
}

void SensitivityTab::OnReleaseThresholdChanged(wxCommandEvent& event)
{
    myIsUpdatingReleaseThreshold = true;
}

void SensitivityTab::UpdateDisplays()
{
    vector<tuple<int, int>> sensors;

    auto pad = Device::Pad();
    if (pad)
    {
        for (int i = 0; i < pad->numSensors; ++i)
        {
            auto sensor = Device::Sensor(i);
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

}; // namespace adp.
