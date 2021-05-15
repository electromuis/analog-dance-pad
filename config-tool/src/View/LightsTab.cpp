#include "wx/sizer.h"
#include "wx/panel.h"
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

class ColorSettingGradient : public wxWindow
{
public:
    ColorSettingGradient(wxWindow* owner, wxPoint position, color24 c1, color24 c2)
        : wxWindow(owner, wxID_ANY, position, wxSize(100, 30), wxBG_STYLE_PAINT)
    {
        myStartColor = c1;
        myEndColor = c2;
        myIsFadeEnabled = false;
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
        if (myIsFadeEnabled)
        {
            auto endColor = ToWx(myEndColor);

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
};

BEGIN_EVENT_TABLE(ColorSettingGradient, wxWindow)
    EVT_PAINT(ColorSettingGradient::OnPaint)
    EVT_LEFT_DOWN(ColorSettingGradient::OnClick)
END_EVENT_TABLE()

class LightSettingsPanel : public wxPanel
{
public:
    LightSettingsPanel(wxWindow* owner)
        : wxPanel(owner, wxID_ANY, wxDefaultPosition, wxSize(300, 100))
    {
        auto box = new wxStaticBox(this, wxID_ANY, wxEmptyString, wxPoint(0, 0), wxSize(300, 100));
        myOnSetting = new ColorSettingGradient(box, wxPoint(10, 10), { 20, 100, 20 }, { 40, 200, 40 });
        myOffSetting = new ColorSettingGradient(box, wxPoint(120, 10), { 80, 80, 80 }, { 20, 0, 0 });

        myFadeOnBox = new wxCheckBox(this, wxID_ANY, L"Fade on", wxPoint(10, 45), wxSize(100, 20));
        myFadeOnBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOnToggled, this);

        myFadeOffBox = new wxCheckBox(this, wxID_ANY, L"Fade off", wxPoint(120, 45), wxSize(100, 20));
        myFadeOffBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOffToggled, this);
    }

    void OnFadeOnToggled(wxCommandEvent& event);
    void OnFadeOffToggled(wxCommandEvent& event);

private:
    ColorSettingGradient* myOnSetting;
    ColorSettingGradient* myOffSetting;
    wxCheckBox* myFadeOnBox;
    wxCheckBox* myFadeOffBox;
};

void LightSettingsPanel::OnFadeOnToggled(wxCommandEvent& event)
{
    myOnSetting->EnableFade(myFadeOnBox->IsChecked());
}

void LightSettingsPanel::OnFadeOffToggled(wxCommandEvent& event)
{
    myOffSetting->EnableFade(myFadeOffBox->IsChecked());
}

const wchar_t* LightsTab::Title = L"Lights";

LightsTab::LightsTab(wxWindow* owner) : BaseTab(owner)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    for (int i = 0; i < 3; ++i)
    {
        auto item = new LightSettingsPanel(this);
        sizer->Add(item, 0, wxEXPAND, 5);
    }

    SetSizer(sizer);
}

}; // namespace adp.
