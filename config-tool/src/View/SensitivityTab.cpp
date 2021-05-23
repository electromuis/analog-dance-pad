#include <map>

#include "wx/dcbuffer.h"
#include "wx/stattext.h"

#include "Assets/Assets.h"

#include "View/SensitivityTab.h"

using namespace std;

namespace adp {

static constexpr int SENSOR_INDEX_NONE = -1;

class SensorDisplay : public wxWindow
{
public:
    SensorDisplay(SensitivityTab* owner)
        : wxWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBG_STYLE_PAINT | wxFULL_REPAINT_ON_RESIZE)
        , myOwner(owner)
    {
        SetMinSize(wxSize(10, 50));
        SetMaxSize(wxSize(500, 1000));
    }

    void Tick()
    {
        if (myAdjustingSensorIndex != SENSOR_INDEX_NONE)
        {
            auto mouse = wxGetMouseState();
            auto rect = GetScreenRect();
            double value = double(mouse.GetY() - rect.y);
            double range = max(1.0, (double)rect.height);
            myAdjustingSensorThreshold = clamp(1.0 - (value / range), 0.0, 1.0);
            if (!mouse.LeftIsDown())
            {
                Device::SetThreshold(myAdjustingSensorIndex, myAdjustingSensorThreshold);
                myAdjustingSensorIndex = SENSOR_INDEX_NONE;
            }
        }
    }

    void OnPaint(wxPaintEvent& evt)
    {
        wxBufferedPaintDC dc(this);
        auto size = GetClientSize();

        int x = 0;
        for (size_t i = 0; i < mySensorIndices.size(); ++i)
        {
            int barW = (i < mySensorIndices.size() - 1)
                ? size.x * (i + 1) / mySensorIndices.size() - x
                : size.x - x;

            auto sensor = Device::Sensor(mySensorIndices[i]);
            auto pressed = sensor ? sensor->pressed : false;

            auto threshold = sensor ? sensor->threshold : 0.0;
            if (myAdjustingSensorIndex == mySensorIndices[i])
                threshold = myAdjustingSensorThreshold;

            int barH = sensor ? (sensor->value * size.y) : 0;
            int thresholdY = size.y - (threshold * size.y);

            // Empty region above the current sensor value.
            dc.SetPen(Pens::Black1px());
            dc.SetBrush(Brushes::SensorBar());
            dc.DrawRectangle(x, 0, barW, size.y - barH);

            // Line representing where the release threshold would be for the current sensor.
            auto releaseThreshold = myOwner->ReleaseThreshold();
            if (releaseThreshold < 1.0)
            {
                dc.SetBrush(Brushes::ReleaseMargin());
                int releaseY = size.y - (releaseThreshold * threshold * size.y);
                dc.DrawRectangle(x, thresholdY, barW, max(1, releaseY - thresholdY));
            }

            // Coloured region below the current sensor value.
            dc.SetBrush(pressed ? Brushes::SensorOn() : Brushes::SensorOff());
            dc.DrawRectangle(x, size.y - barH, barW, barH);

            // Line representing the sensitivity threshold.
            dc.SetBrush(*wxWHITE_BRUSH);
            dc.DrawRectangle(x, thresholdY - 1, barW, 3);

            // Small text block at the top displaying sensitivity threshold.
            auto sensitivityText = wxString::Format("%i%%", (int)std::lround(threshold * 100.0));
            auto rect = wxRect(x + barW / 2 - 20, 5, 40, 20);
            dc.SetBrush(Brushes::DarkGray());
            dc.DrawRectangle(rect);
            dc.SetTextForeground(*wxWHITE);
            dc.DrawLabel(sensitivityText, rect, wxALIGN_CENTER);

            x += barW;
        }
    }

    void OnClick(wxMouseEvent& event)
    {
        auto pos = event.GetPosition();
        auto rect = GetClientRect();
        if (rect.Contains(pos))
        {
            int barIndex = (pos.x - rect.x) * mySensorIndices.size() / max(1, rect.width);
            if (barIndex >= 0 && barIndex < (int)mySensorIndices.size())
                myAdjustingSensorIndex = mySensorIndices[barIndex];
        }
    }

    void SetTarget(int button, const vector<int>& sensorIndices)
    {
        myButton = button;
        mySensorIndices = sensorIndices;
    }

    int button() const { return myButton; }

    DECLARE_EVENT_TABLE()

protected:
    wxSize DoGetBestSize() const override { return wxSize(20, 100); }

private:
    SensitivityTab* myOwner;
    int myButton = 0;
    vector<int> mySensorIndices;
    int myAdjustingSensorIndex = SENSOR_INDEX_NONE;
    double myAdjustingSensorThreshold = 0.0;
};

BEGIN_EVENT_TABLE(SensorDisplay, wxWindow)
    EVT_PAINT(SensorDisplay::OnPaint)
    EVT_LEFT_DOWN(SensorDisplay::OnClick)
END_EVENT_TABLE()

static const wchar_t* ActivationMsg =
    L"Click inside sensor bars to adjust their activation threshold.";

static const wchar_t* ReleaseMsg =
    L"Adjust release threshold (percentage of activation threshold).";

const wchar_t* SensitivityTab::Title = L"Sensitivity";

SensitivityTab::SensitivityTab(wxWindow* owner, const PadState* pad)
    : wxWindow(owner, wxID_ANY)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto activationLabel = new wxStaticText(this, wxID_ANY, ActivationMsg);
    sizer->Add(activationLabel, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 4);

    mySensorSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(mySensorSizer, 1, wxEXPAND);
    UpdateDisplays();

    auto releaseLabel = new wxStaticText(this, wxID_ANY, ReleaseMsg);
    sizer->Add(releaseLabel, 0, wxALIGN_CENTER_HORIZONTAL);

    myReleaseThresholdSlider = new wxSlider(this, wxID_ANY, 100, 1, 100,
        wxDefaultPosition, wxSize(300, -1), wxSL_VALUE_LABEL |  wxSL_TOP | wxSL_BOTH);

    myReleaseThresholdSlider->Bind(wxEVT_SLIDER, &SensitivityTab::OnReleaseThresholdChanged, this);
    sizer->Add(myReleaseThresholdSlider, 0, wxALIGN_CENTER_HORIZONTAL);

    SetSizer(sizer);

    myIsAdjustingReleaseThreshold = false;
}

void SensitivityTab::HandleChanges(DeviceChanges changes)
{
    if (changes & DCF_BUTTON_MAPPING)
        UpdateDisplays();
}

void SensitivityTab::Tick()
{
    // Determine the current release threshold. While the user is dragging the slider, the slider value is visualized
    // instead of the pad value. When the user releases the left mouse button, the slider value is applied to the pad.

    if (myIsAdjustingReleaseThreshold)
    {
        myReleaseThreshold = myReleaseThresholdSlider->GetValue() * 0.01;
        if (!wxGetMouseState().LeftIsDown())
        {
            Device::SetReleaseThreshold(myReleaseThreshold);
            myIsAdjustingReleaseThreshold = false;
        }
    }
    else
    {
        auto pad = Device::Pad();
        auto threshold = pad ? pad->releaseThreshold : 1.0;
        if (myReleaseThreshold != threshold)
        {
            myReleaseThreshold = threshold;
            auto sliderValue = (int)lround(threshold * 100);
            myReleaseThresholdSlider->SetValue(sliderValue);
        }
    }

    for (auto display : mySensorDisplays)
    {
        display->Tick();
        display->Refresh(false);
    }
}

double SensitivityTab::ReleaseThreshold() const
{
    return myReleaseThreshold;
}

void SensitivityTab::OnReleaseThresholdChanged(wxCommandEvent& event)
{
    myIsAdjustingReleaseThreshold = true;
}

void SensitivityTab::UpdateDisplays()
{
    map<int, vector<int>> mapping; // button -> sensors[]

    auto pad = Device::Pad();
    if (pad)
    {
        for (int i = 0; i < pad->numSensors; ++i)
        {
            auto sensor = Device::Sensor(i);
            if (sensor->button != 0)
                mapping[sensor->button].push_back(i);
        }
    }

    if (mapping.size() != mySensorDisplays.size())
    {
        mySensorSizer->Clear(true);
        mySensorDisplays.clear();
        for (auto mapping : mapping)
        {
            auto display = new SensorDisplay(this);
            mySensorDisplays.push_back(display);
            mySensorSizer->Add(display, 1, wxALL | wxEXPAND, 4);
        }
        mySensorSizer->Layout();
    }
    
    int index = 0;
    for (auto mapping : mapping)
        mySensorDisplays[index++]->SetTarget(mapping.first, mapping.second);
}

}; // namespace adp.
