#include "Adp.h"

#include <set>

#include "wx/sizer.h"
#include "wx/panel.h"
#include "wx/button.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/combobox.h"
#include "wx/dcbuffer.h"
#include "wx/checkbox.h"
#include "wx/colordlg.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"

#include "Model/Reporter.h"
#include "Model/Device.h"

#include "Assets/Assets.h"

#include "View/LightsTab.h"


namespace adp {

wxDEFINE_EVENT(EVT_COLOR_CHANGED, wxCommandEvent);

static wxColour ToWx(RgbColor color)
{
    return wxColour(color.red, color.green, color.blue);
}

enum Ids
{
    ADD_LIGHT_SETTING    = 1,
    DELETE_LIGHT_SETTING = 2,
    ADD_LED_SETTING      = 3,
    DELETE_LED_SETTING   = 4,
};

// ====================================================================================================================
// Color setting gradient.
// ====================================================================================================================

class ColorSettingGradient : public wxWindow
{
public:
    ColorSettingGradient(
        wxWindow* owner,
        wxPoint position,
        wxSize size,
        wxString label,
        RgbColor c1,
        RgbColor c2,
        bool fade)
        : wxWindow(owner, wxID_ANY, position, size)
        , myLabel(label)
        , myStartColor(c1)
        , myEndColor(c2)
        , myIsFadeEnabled(fade)
    {
    }

    void EnableFade(bool enabled)
    {
        myIsFadeEnabled = enabled;
        Refresh();
    }

    bool IsFadeEnabled()
    {
        return myIsFadeEnabled;
    }

    RgbColor GetStartColor()
    {
        return myStartColor;
    }

    RgbColor GetEndColor()
    {
        return myEndColor;
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

    bool ShowPicker(RgbColor& color)
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

        wxCommandEvent event(EVT_COLOR_CHANGED);
        wxPostEvent(this, event);

        return true;
    }

    DECLARE_EVENT_TABLE()

private:
    bool myIsFadeEnabled;
    RgbColor myStartColor;
    RgbColor myEndColor;
    wxString myLabel;
};

BEGIN_EVENT_TABLE(ColorSettingGradient, wxWindow)
    EVT_PAINT(ColorSettingGradient::OnPaint)
    EVT_LEFT_DOWN(ColorSettingGradient::OnClick)
END_EVENT_TABLE()

// ====================================================================================================================
// LED settings panel.
// ====================================================================================================================

class LedSettingsPanel : public wxPanel
{
public:
    // Settings for a newly created led mapping.
    LedSettingsPanel(LightSettingsPanel* owner, wxWindow* ownerAsWindow, const LedMapping& mapping)
        : LedSettingsPanel(owner, ownerAsWindow, -1 /* will be assigned via reindex */, mapping)
    {
    }

    // Settings for an existing led mapping.
    LedSettingsPanel(LightSettingsPanel* owner, wxWindow* ownerAsWindow, int ledMappingIndex, const LedMapping& mapping)
        : wxPanel(ownerAsWindow, wxID_ANY, wxDefaultPosition, wxSize(400, 25))
        , myOwner(owner)
        , myLedMappingIndex(ledMappingIndex)
        , myLightRuleIndex(mapping.lightRuleIndex)
    {
        auto pad = Device::Pad();

        wxArrayString options;
        for (int i = 1; i <= pad->numSensors; ++i)
            options.Add(wxString::Format("Sensor %i", i));

        mySensorSelect = new wxComboBox(this, 0, options[0],
            wxPoint(0, 0), wxSize(100, 25), options, wxCB_READONLY);
        mySensorSelect->Bind(wxEVT_COMBOBOX, &LedSettingsPanel::OnSensorChanged, this);

        auto lfrom = new wxStaticText(
            this, wxID_ANY, L"LED", wxPoint(105, 5), wxSize(30, 25), wxALIGN_CENTRE_HORIZONTAL);
        mySensorFrom = new wxSpinCtrl(
            this, wxID_ANY, L"", wxPoint(140, 0), wxSize(80, 25), wxSP_ARROW_KEYS, 0, 100, 0);
        auto lto = new wxStaticText(
            this, wxID_ANY, L"to", wxPoint(225, 5), wxSize(30, 25), wxALIGN_CENTRE_HORIZONTAL);
        mySensorTo = new wxSpinCtrl(
            this, wxID_ANY, L"", wxPoint(260, 0), wxSize(80, 25), wxSP_ARROW_KEYS, 0, 100, 0);

        mySensorFrom->Bind(wxEVT_SPINCTRL, &LedSettingsPanel::OnLedIndexChanged, this);
        mySensorTo->Bind(wxEVT_SPINCTRL, &LedSettingsPanel::OnLedIndexChanged, this);

        mySensorFrom->SetValue(mapping.ledIndexBegin);
        mySensorTo->SetValue(mapping.ledIndexEnd);
        mySensorSelect->SetSelection(mapping.sensorIndex);

        myHLine = new wxStaticLine(this, wxID_ANY, wxPoint(350, 12), wxDefaultSize);
        myDeleteButton = new wxButton(this, DELETE_LED_SETTING, L"\u2715", wxDefaultPosition, wxSize(25, 25));
    }

    void RecomputeLayout()
    {
        auto size = GetSize();
        myDeleteButton->SetPosition(wxPoint(size.x - 25, 0));
        myHLine->SetSize(size.x - 385, wxDefaultCoord);
    }

    void OnSensorChanged(wxCommandEvent& event)
    {
        SendToDevice();
    }

    void OnLedIndexChanged(wxCommandEvent& event)
    {
        SendToDevice();
    }

    void OnDelete(wxCommandEvent& event);

    void UpdateIndices(int lightRuleIndex, int ledMappingIndex)
    {
        if (myLightRuleIndex == lightRuleIndex && myLedMappingIndex == ledMappingIndex)
            return;

        myLightRuleIndex = lightRuleIndex;
        myLedMappingIndex = ledMappingIndex;
        SendToDevice();
    }

    bool SendToDevice()
    {
        LedMapping mapping;
        mapping.lightRuleIndex = myLightRuleIndex;
        mapping.sensorIndex = mySensorSelect->GetSelection();
        mapping.ledIndexBegin = mySensorFrom->GetValue();
        mapping.ledIndexEnd = mySensorTo->GetValue();
        return Device::SendLedMapping(myLedMappingIndex, mapping);
    }

    int GetLedMappingIndex() const { return myLedMappingIndex; }

    DECLARE_EVENT_TABLE()

private:
    LightSettingsPanel* myOwner;
    wxComboBox* mySensorSelect;
    wxSpinCtrl* mySensorFrom;
    wxSpinCtrl* mySensorTo;
    wxStaticLine* myHLine;
    wxButton* myDeleteButton;
    int myLedMappingIndex;
    int myLightRuleIndex;
};

BEGIN_EVENT_TABLE(LedSettingsPanel, wxWindow)
    EVT_BUTTON(DELETE_LED_SETTING, LedSettingsPanel::OnDelete)
END_EVENT_TABLE()

// ====================================================================================================================
// Light settings panel.
// ====================================================================================================================

static LightRule DefaultLightSettings()
{
    LightRule rule;
}

class LightSettingsPanel : public wxPanel
{
public:
    // Settings for a newly created light rule.
    LightSettingsPanel(LightsTab* owner, const LightRule& rule)
        : LightSettingsPanel(owner, -1 /* will be assigned via reindex */, rule, nullptr)
    {
    }

    // Settings for an existing light rule.
    LightSettingsPanel(LightsTab* owner, int lightRuleIndex, const LightRule& rule, const LightsState* lights)
        : wxPanel(owner, wxID_ANY, wxDefaultPosition, wxSize(310, 100))
        , myOwner(owner)
        , myLightRuleIndex(lightRuleIndex)
    {
        // Color settings and delete.
        myOnSetting = new ColorSettingGradient(
            this, wxPoint(0, 0), wxSize(140, 30), L"ON", rule.onColor, rule.onFadeColor, rule.fadeOn);
        myOffSetting = new ColorSettingGradient(
            this, wxPoint(150, 0), wxSize(140, 30), L"OFF", rule.offColor, rule.offFadeColor, rule.fadeOff);

        myOnSetting->Bind(EVT_COLOR_CHANGED, &LightSettingsPanel::OnColorChanged, this);
        myOffSetting->Bind(EVT_COLOR_CHANGED, &LightSettingsPanel::OnColorChanged, this);

        myFadeOnBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(0, 35), wxSize(50, 20));
        myFadeOnBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOnToggled, this);
        myFadeOnBox->SetValue(rule.fadeOn);

        myFadeOffBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(150, 35), wxSize(50, 20));
        myFadeOffBox->Bind(wxEVT_CHECKBOX, &LightSettingsPanel::OnFadeOffToggled, this);
        myFadeOffBox->SetValue(rule.fadeOff);

        myHLine = new wxStaticLine(this, wxID_ANY, wxPoint(300, 14), wxDefaultSize);

        myDeleteButton =
            new wxButton(this, DELETE_LIGHT_SETTING, L"\u2715", wxDefaultPosition, wxSize(30, 30));

        // LED settings.
        if (lights)
        {
            for (auto& mapping : lights->ledMappings)
            {
                if (mapping.second.lightRuleIndex == lightRuleIndex)
                {
                    auto item = new LedSettingsPanel(this, this, mapping.first, mapping.second);
                    myLedSettings.push_back(item);
                }
            }
        }

        myAddLedSettingButton =
            new wxButton(this, ADD_LED_SETTING, L"+", wxDefaultPosition, wxSize(100, 25));
    }

    void DeleteLedSetting(LedSettingsPanel* item)
    {
        auto it = std::find(myLedSettings.begin(), myLedSettings.end(), item);
        if (it == myLedSettings.end())
            return;

        item->Destroy();
        myLedSettings.erase(it);
        myOwner->ReindexAll();
    }

    int GetContentHeight() const { return myLedSettings.size() * 30 + 100; }

    void RecomputeLayout()
    {
        auto size = GetSize();
        int contentH = 60;

        for (auto item : myLedSettings)
        {
            int itemH = 25;
            item->SetPosition(wxPoint(0, contentH));
            item->SetSize(wxSize(size.x, itemH));
            item->RecomputeLayout();
            contentH += itemH + 5;
        }

        myDeleteButton->SetPosition(wxPoint(size.x - 30, 0));
        myHLine->SetSize(size.x - 340, wxDefaultCoord);

        myAddLedSettingButton->SetPosition(wxPoint(0, contentH));

        Refresh();
    }

    void OnFadeOnToggled(wxCommandEvent& event)
    {
        myOnSetting->EnableFade(myFadeOnBox->IsChecked());
        SendToDevice();
    }

    void OnFadeOffToggled(wxCommandEvent& event)
    {
        myOffSetting->EnableFade(myFadeOffBox->IsChecked());
        SendToDevice();
    }

    void OnColorChanged(wxCommandEvent& event)
    {
        SendToDevice();
    }

    void OnAddLedSetting(wxCommandEvent& event)
    {
        // Note: indices will be udated by ReindexAll.
        LedMapping mapping;
        mapping.lightRuleIndex = -1;
        mapping.sensorIndex = 0;
        mapping.ledIndexBegin = 0;
        mapping.ledIndexEnd = 0;
        auto item = new LedSettingsPanel(this, this, -1, mapping);
        myLedSettings.push_back(item);
        myOwner->ReindexAll();
    }

    void OnDelete(wxCommandEvent& event)
    {
        myOwner->DeleteLightSetting(this);
    }

    void UpdateIndex(int lightRuleIndex)
    {
        if (myLightRuleIndex == lightRuleIndex)
            return;

        myLightRuleIndex = lightRuleIndex;
        SendToDevice();
    }

    bool SendToDevice()
    {
        LightRule rule;
        rule.fadeOn = myOnSetting->IsFadeEnabled();
        rule.fadeOff = myOffSetting->IsFadeEnabled();
        rule.onColor = myOnSetting->GetStartColor();
        rule.offColor = myOffSetting->GetStartColor();
        rule.onFadeColor = myOnSetting->GetEndColor();
        rule.offFadeColor = myOffSetting->GetEndColor();
        return Device::SendLightRule(myLightRuleIndex, rule);
    }

    const std::vector<LedSettingsPanel*>& GetLedSettings()
    {
        return myLedSettings;
    }

    int GetLightRuleIndex() const { return myLightRuleIndex; }

    DECLARE_EVENT_TABLE()

private:
    LightsTab* myOwner;
    ColorSettingGradient* myOnSetting;
    ColorSettingGradient* myOffSetting;
    wxCheckBox* myFadeOnBox;
    wxCheckBox* myFadeOffBox;
    wxStaticLine* myHLine;
    wxButton* myDeleteButton;
    wxButton* myAddLedSettingButton;
    std::vector<LedSettingsPanel*> myLedSettings;
    int myLightRuleIndex;
};

BEGIN_EVENT_TABLE(LightSettingsPanel, wxWindow)
    EVT_BUTTON(ADD_LED_SETTING, LightSettingsPanel::OnAddLedSetting)
    EVT_BUTTON(DELETE_LIGHT_SETTING, LightSettingsPanel::OnDelete)
END_EVENT_TABLE()

// NOTE: defined here due to DeleteLedSetting.
void LedSettingsPanel::OnDelete(wxCommandEvent& event)
{
    myOwner->DeleteLedSetting(this);
}

// ====================================================================================================================
// Lights tab.
// ====================================================================================================================

const wchar_t* LightsTab::Title = L"Lights";

LightsTab::LightsTab(wxWindow* owner, const LightsState* lights)
    : wxScrolledWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
{
    myAddSettingButton = new wxButton(this, ADD_LIGHT_SETTING, L"Add light setting", wxDefaultPosition, wxSize(200, 30));

    //todo fix linux
#ifdef _MSC_VER
    UpdateSettings(lights);
#endif

    SetScrollRate(5, 5);
}

void LightsTab::UpdateSettings(const LightsState* lights)
{
    for (auto setting : myLightSettings)
        setting->Destroy();
    myLightSettings.clear();

    for (auto& rule : lights->lightRules)
    {
        auto item = new LightSettingsPanel(this, rule.first, rule.second, lights);
        myLightSettings.push_back(item);
    }
    RecomputeLayout();
}

void LightsTab::HandleChanges(DeviceChanges changes)
{
    if ((changes & DCF_LIGHTS) && !myTemporarilyIgnoreChanges)
    {
        // Updating on every change causes the UI to flicker. Since all changes should originate from the tab itself,
        // let's assume that updating is not necessary since the controls should already be in sync with the device.

        auto lights = Device::Lights();
        if (lights)
            UpdateSettings(lights);
    }
}

void LightsTab::OnAddLightSetting(wxCommandEvent& event)
{
    LightRule rule;
    rule.fadeOn = false;
    rule.fadeOff = false;
    rule.onColor = { 150, 150, 150 };
    rule.offColor = { 0, 0, 0 };
    rule.onFadeColor = { 0, 0, 0 };
    rule.offFadeColor = { 0, 0, 0 };
    auto item = new LightSettingsPanel(this, rule);
    myLightSettings.push_back(item);
    ReindexAll();
}

void LightsTab::OnResize(wxSizeEvent& event)
{
    RecomputeLayout();
}

void LightsTab::DeleteLightSetting(LightSettingsPanel* item)
{
    auto it = std::find(myLightSettings.begin(), myLightSettings.end(), item);
    if (it == myLightSettings.end())
        return;

    item->Destroy();
    myLightSettings.erase(it);
    ReindexAll();
}

void LightsTab::RecomputeLayout()
{
    auto size = GetClientSize();
    int margin = 5;
    int contentH = margin;

    for (auto item : myLightSettings)
    {
        int itemH = item->GetContentHeight();
        item->SetPosition(wxPoint(margin, contentH));
        item->SetSize(wxSize(size.x - margin * 2, itemH));
        item->RecomputeLayout();
        contentH += itemH + margin * 2;
    }

    myAddSettingButton->SetPosition(wxPoint(margin, contentH));
    myAddSettingButton->SetSize(wxSize(size.x - margin * 2, 30));
    contentH += 30 + margin * 2;

    SetVirtualSize(size.x, contentH);
}

void LightsTab::ReindexAll()
{
    myTemporarilyIgnoreChanges = true;

    // Aggregate indices of settings that are currently used.

    std::set<int> usedLightRules;
    std::set<int> usedLedMappings;

    for (auto setting : myLightSettings)
    {
        usedLightRules.insert(setting->GetLightRuleIndex());
        for (auto mapping : setting->GetLedSettings())
            usedLedMappings.insert(mapping->GetLedMappingIndex());
    }

    // Reassign indices to ordered values without gaps.

    int lightRuleIndex = 0;
    int ledMappingIndex = 0;

    for (auto setting : myLightSettings)
    {
        setting->UpdateIndex(lightRuleIndex);
        for (auto mapping : setting->GetLedSettings())
        {
            mapping->UpdateIndices(lightRuleIndex, ledMappingIndex);
            ++ledMappingIndex;
        }
        ++lightRuleIndex;
    }

    // Disable settings that are no longer used.

    for (auto index : usedLightRules)
    {
        if (index >= lightRuleIndex)
            Device::DisableLightRule(index);
    }
    for (auto index : usedLedMappings)
    {
        if (index >= ledMappingIndex)
            Device::DisableLedMapping(index);
    }

    myTemporarilyIgnoreChanges = false;
    UpdateSettings(Device::Lights());
}

BEGIN_EVENT_TABLE(LightsTab, wxWindow)
    EVT_BUTTON(ADD_LIGHT_SETTING, LightsTab::OnAddLightSetting)
    EVT_SIZE(LightsTab::OnResize)
END_EVENT_TABLE()

}; // namespace adp.
