#include "wx/sizer.h"
#include "wx/panel.h"
#include "wx/button.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/dcbuffer.h"
#include "wx/checkbox.h"
#include "wx/colordlg.h"

#include "View/LightsTab.h"
#include "Assets/Assets.h"

#include "Model/Reporter.h"

namespace adp {

static wxColour ToWx(color24 color)
{
    return wxColour(color.red, color.green, color.blue);
}

enum Ids { ADD_LIGHT_SETTINGS_BUTTON = 1, DELETE_LIGHT_SETTINGS_BUTTON = 2 };

// ====================================================================================================================
// Color setting gradient.
// ====================================================================================================================

class ColorSettingGradient : public wxWindow
{
public:
    ColorSettingGradient(wxWindow* owner, wxPoint position, wxSize size, wxString label, color24 c1, color24 c2)
        : wxWindow(owner, wxID_ANY, position, size)
        , myLabel(label)
        , myStartColor(c1)
        , myEndColor(c2)
        , myIsFadeEnabled(false)
    {
    }

    void EnableFade(bool enabled)
    {
        myIsFadeEnabled = enabled;
        Refresh();
    }

    void OnPaint(wxPaintEvent& evt)
    {
        auto size = GetClientSize();
        wxBufferedPaintDC dc(this);

        auto startColor = ToWx(myStartColor);
        auto centerColor = startColor;
        if (myIsFadeEnabled)
        {
            auto endColor = ToWx(myEndColor);
            centerColor = wxColour(
                (startColor.Red()   + endColor.Red())   / 2,
                (startColor.Green() + endColor.Green()) / 2,
                (startColor.Blue()  + endColor.Blue())  / 2);

            // Gradient.
            dc.SetPen(wxPen(wxColour(100, 100, 100), 1));
            auto rect = wxRect(30, 1, size.x - 60, 28);
            dc.GradientFillLinear(rect, startColor, endColor, wxRIGHT);

            // Start color.
            rect = wxRect(0, 0, 30, size.y);
            dc.SetPen(Pens::Black1px());
            dc.DrawRectangle(rect);
            dc.SetPen(Pens::White1px());
            dc.SetBrush(wxBrush(startColor, wxBRUSHSTYLE_SOLID));
            rect.Deflate(1);
            dc.DrawRectangle(rect);

            // End color.
            rect = wxRect(size.x - 30, 0, 30, size.y);
            dc.SetPen(Pens::Black1px());
            dc.DrawRectangle(rect);
            dc.SetPen(Pens::White1px());
            dc.SetBrush(wxBrush(endColor, wxBRUSHSTYLE_SOLID));
            rect.Deflate(1);
            dc.DrawRectangle(rect);
        }
        else
        {
            // Start color (end color is not used).
            auto rect = wxRect(0, 0, size.x, size.y);
            dc.SetPen(Pens::Black1px());
            dc.DrawRectangle(rect);
            dc.SetPen(Pens::White1px());
            dc.SetBrush(wxBrush(startColor, wxBRUSHSTYLE_SOLID));
            rect.Deflate(1);
            dc.DrawRectangle(rect);
        }

        // Label text.
        auto rect = wxRect(size.x / 2 - 20, 5, 40, 20);
        bool isBright = centerColor.GetLuminance() > 0.5;
        dc.SetTextForeground(isBright ? *wxBLACK : *wxWHITE);
        dc.DrawLabel(myLabel, rect, wxALIGN_CENTER);
    }

    void OnClick(wxMouseEvent& event)
    {
        auto pos = event.GetPosition();
        auto rect = GetClientRect();
        if (rect.Contains(pos))
        {
            if (pos.x < rect.x + 30 || !myIsFadeEnabled)
            {
                ShowPicker(myStartColor);
            }
            else if (pos.x > rect.x + rect.width - 30)
            {
                ShowPicker(myEndColor);
            }
        }
    }

    bool ShowPicker(color24& color)
    {
        wxColourData data;
        data.SetColour(ToWx(color));
        wxColourDialog dlg(this, &data);
        if (dlg.ShowModal() != wxID_OK)
            return false;

        auto result = dlg.GetColourData().GetColour();
        color.red = result.Red();
        color.green = result.Green();
        color.blue = result.Blue();
        Refresh();

        return true;
    }

    DECLARE_EVENT_TABLE()

private:
    bool myIsFadeEnabled;
    color24 myStartColor;
    color24 myEndColor;
    wxString myLabel;
};

BEGIN_EVENT_TABLE(ColorSettingGradient, wxWindow)
    EVT_PAINT(ColorSettingGradient::OnPaint)
    EVT_LEFT_DOWN(ColorSettingGradient::OnClick)
END_EVENT_TABLE()

// ====================================================================================================================
// Light settings panel.
// ====================================================================================================================

class LightSettingsPanel : public wxPanel
{
public:
    LightSettingsPanel(LightsTab* owner, int index)
        : wxPanel(owner, wxID_ANY, wxDefaultPosition, wxSize(310, 100))
        , myOwner(owner)
        , myIndex(index)
    {
        auto sGrad = wxSize(120, 30);
        myOnSetting = new ColorSettingGradient(this, wxPoint(10, 10), sGrad, L"ON", { 20, 100, 20 }, { 40, 200, 40 });
        myOffSetting = new ColorSettingGradient(this, wxPoint(140, 10), sGrad, L"OFF", { 80, 80, 80 }, { 20, 0, 0 });

        myFadeOnBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(45, 45), wxSize(50, 20));
        myFadeOnBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOnToggled, this);

        myFadeOffBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(175, 45), wxSize(50, 20));
        myFadeOffBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOffToggled, this);

        myDeleteButton = new wxButton(this, DELETE_LIGHT_SETTINGS_BUTTON, L"\u2715", wxPoint(270, 10), wxSize(30, 30));
    }

    void OnFadeOnToggled(wxCommandEvent& event)
    {
        myOnSetting->EnableFade(myFadeOnBox->IsChecked());
    }

    void OnFadeOffToggled(wxCommandEvent& event)
    {
        myOffSetting->EnableFade(myFadeOffBox->IsChecked());
    }

    void OnDelete(wxCommandEvent& event)
    {
        myOwner->DeleteLightSettings(myIndex);
    }

    DECLARE_EVENT_TABLE()

private:
    int myIndex;
    LightsTab* myOwner;
    ColorSettingGradient* myOnSetting;
    ColorSettingGradient* myOffSetting;
    wxCheckBox* myFadeOnBox;
    wxCheckBox* myFadeOffBox;
    wxButton* myDeleteButton;
};

BEGIN_EVENT_TABLE(LightSettingsPanel, wxWindow)
    EVT_BUTTON(DELETE_LIGHT_SETTINGS_BUTTON, LightSettingsPanel::OnDelete)
END_EVENT_TABLE()

// ====================================================================================================================
// Lights tab.
// ====================================================================================================================

const wchar_t* LightsTab::Title = L"Lights";

LightsTab::LightsTab(wxWindow* owner)
    : wxScrolledWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
    , myNumLightSettings(0)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto bRule = new wxButton(this, ADD_LIGHT_SETTINGS_BUTTON, L"Add light setting", wxDefaultPosition, wxSize(200, 30));
    sizer->Add(bRule, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 5);

    SetSizer(sizer);

    for (int i = 0; i < 4; ++i)
        OnAddLightSettings(wxCommandEvent());

    SetScrollRate(5, 5);
}

void LightsTab::OnAddLightSettings(wxCommandEvent& event)
{
    auto sizer = GetSizer();
    auto settings = new LightSettingsPanel(this, myNumLightSettings);
    sizer->Insert(myNumLightSettings, settings);
    ++myNumLightSettings;
    FitInside();
}

void LightsTab::DeleteLightSettings(int index)
{
}

BEGIN_EVENT_TABLE(LightsTab, wxWindow)
    EVT_BUTTON(ADD_LIGHT_SETTINGS_BUTTON, LightsTab::OnAddLightSettings)
END_EVENT_TABLE()

}; // namespace adp.
