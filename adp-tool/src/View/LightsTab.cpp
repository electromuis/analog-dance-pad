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
    ADD_LIGHT_RULE     = 1,
    DELETE_LIGHT_RULE  = 2,
    ADD_LED_MAPPING    = 3,
    DELETE_LED_MAPPING = 4,
};

struct LightsTab::Change
{
    enum Type { ADD_LIGHT_RULE, REMOVE_LIGHT_RULE, ADD_LED_MAPPING, REMOVE_LED_MAPPING };
    Type type;
    int lightRuleIndex = -1;  // Applies to REMOVE_LIGHT_RULE, ADD_LED_MAPPING, REMOVE_LED_MAPPING.
    int ledMappingIndex = -1; // Applies to REMOVE_LED_MAPPING.
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
        auto luminance = (0.299*centerColor.Red() + 0.587*centerColor.Green() + 0.114*centerColor.Blue()) / 255.0;
        bool isBright = luminance > 0.5;
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
// LED mapping panel.
// ====================================================================================================================

static LedMapping NewLedMapping(int lightRuleIndex)
{
    LedMapping mapping;
    mapping.lightRuleIndex = lightRuleIndex;
    mapping.sensorIndex = 0;
    mapping.ledIndexBegin = 0;
    mapping.ledIndexEnd = 0;
    return mapping;
}

class LedMappingPanel : public wxPanel
{
public:
    LedMappingPanel(LightRulePanel* owner, wxWindow* ownerAsWindow, int ledMappingIndex, const LedMapping& mapping)
        : wxPanel(ownerAsWindow, wxID_ANY, wxDefaultPosition, wxSize(400, 25))
        , myOwner(owner)
        , myLedMappingIndex(ledMappingIndex)
        , myLightRuleIndex(mapping.lightRuleIndex)
    {
        auto pad = Device::Pad();

        wxArrayString options;
        for (int i = 1; i <= pad->numSensors; ++i)
            options.Add(wxString::Format("Sensor %i", i));

        auto sizer = new wxBoxSizer(wxHORIZONTAL);

        mySensorSelect = new wxComboBox(this, 0, options[0],
            wxPoint(0, 0), wxSize(115, 25), options, wxCB_READONLY);
        mySensorSelect->Bind(wxEVT_COMBOBOX, &LedMappingPanel::OnSensorChanged, this);
        sizer->Add(mySensorSelect);

        auto lfrom = new wxStaticText(this, wxID_ANY, L"LED");
        mySensorFrom = new wxSpinCtrl(
            this, wxID_ANY, L"", wxDefaultPosition, wxSize(115, 25), wxSP_ARROW_KEYS, 0, 100, 0);
        auto lto = new wxStaticText(this, wxID_ANY, L"to");
        mySensorTo = new wxSpinCtrl(
            this, wxID_ANY, L"", wxDefaultPosition, wxSize(115, 25), wxSP_ARROW_KEYS, 0, 100, 0);

        mySensorFrom->Bind(wxEVT_SPINCTRL, &LedMappingPanel::OnLedIndexChanged, this);
        mySensorTo->Bind(wxEVT_SPINCTRL, &LedMappingPanel::OnLedIndexChanged, this);

        sizer->Add(lfrom, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
        sizer->Add(mySensorFrom, 0, wxLEFT, 5);
        sizer->Add(lto, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
        sizer->Add(mySensorTo, 0, wxLEFT, 5);

        mySensorFrom->SetValue(mapping.ledIndexBegin);
        mySensorTo->SetValue(mapping.ledIndexEnd);
        mySensorSelect->SetSelection(mapping.sensorIndex);

        myHLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        myDeleteButton = new wxButton(this, DELETE_LED_MAPPING, L"\u2715", wxDefaultPosition, wxSize(35, 25));

        sizer->Add(myHLine, 1, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5);
        sizer->Add(myDeleteButton, 0, wxLEFT, 5);

        SetSizer(sizer);
    }
    
    void OnSensorChanged(wxCommandEvent& event)
    {
        SendToDevice();
    }

    void OnLedIndexChanged(wxCommandEvent& event)
    {
        SendToDevice();
    }

    void OnDeleteRequested(wxCommandEvent& event);

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
    int GetLightRuleIndex() const { return myLightRuleIndex; }

    DECLARE_EVENT_TABLE()

private:
    LightRulePanel* myOwner;
    wxComboBox* mySensorSelect;
    wxSpinCtrl* mySensorFrom;
    wxSpinCtrl* mySensorTo;
    wxStaticLine* myHLine;
    wxButton* myDeleteButton;
    int myLedMappingIndex;
    int myLightRuleIndex;
};

BEGIN_EVENT_TABLE(LedMappingPanel, wxWindow)
    EVT_BUTTON(DELETE_LED_MAPPING, LedMappingPanel::OnDeleteRequested)
END_EVENT_TABLE()

// ====================================================================================================================
// Light rule panel.
// ====================================================================================================================

static LightRule NewLightRule()
{
    LightRule rule;
    rule.fadeOn = false;
    rule.fadeOff = false;
    rule.onColor = { 150, 150, 150 };
    rule.offColor = { 0, 0, 0 };
    rule.onFadeColor = { 0, 0, 0 };
    rule.offFadeColor = { 0, 0, 0 };
    return rule;
}

class LightRulePanel : public wxPanel
{
public:
    LightRulePanel(LightsTab* owner, int lightRuleIndex, const LightRule& rule, const LightsState* state = nullptr)
        : wxPanel(owner, wxID_ANY, wxDefaultPosition, wxSize(310, 100))
        , myOwner(owner)
        , myLightRuleIndex(lightRuleIndex)
    {
        // Color settings and delete.
        myOnSetting = new ColorSettingGradient(
            this, wxPoint(0, 0), wxSize(140, 30), L"ON", rule.onColor, rule.onFadeColor, rule.fadeOn);
        myOffSetting = new ColorSettingGradient(
            this, wxPoint(150, 0), wxSize(140, 30), L"OFF", rule.offColor, rule.offFadeColor, rule.fadeOff);

        myOnSetting->Bind(EVT_COLOR_CHANGED, &LightRulePanel::OnColorChanged, this);
        myOffSetting->Bind(EVT_COLOR_CHANGED, &LightRulePanel::OnColorChanged, this);

        myFadeOnBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(0, 35), wxSize(70, 20));
        myFadeOnBox->Bind(wxEVT_CHECKBOX, &LightRulePanel::OnFadeOnToggled, this);
        myFadeOnBox->SetValue(rule.fadeOn);

        myFadeOffBox = new wxCheckBox(this, wxID_ANY, L"Fade", wxPoint(150, 35), wxSize(70, 20));
        myFadeOffBox->Bind(wxEVT_CHECKBOX, &LightRulePanel::OnFadeOffToggled, this);
        myFadeOffBox->SetValue(rule.fadeOff);

        myHLine = new wxStaticLine(this, wxID_ANY, wxPoint(300, 14), wxDefaultSize);
        myDeleteButton = new wxButton(this, DELETE_LIGHT_RULE, L"\u2715", wxDefaultPosition, wxSize(35, 35));

        if (state)
        {
            for (auto& mapping : state->ledMappings)
            {
                if (mapping.second.lightRuleIndex == lightRuleIndex)
                {
                    auto item = new LedMappingPanel(this, this, mapping.first, mapping.second);
                    myLedMappings.push_back(item);
                }
            }
        }

        myAddLedMappingButton =
            new wxButton(this, ADD_LED_MAPPING, L"+", wxDefaultPosition, wxSize(100, 25));
    }

    int GetContentHeight() const { return myLedMappings.size() * 30 + 100; }

    void RecomputeLayout()
    {
        auto size = GetSize();
        int contentH = 60;

        for (auto item : myLedMappings)
        {
            int itemH = 25;
            item->SetPosition(wxPoint(0, contentH));
            item->SetSize(wxSize(size.x, itemH));
            contentH += itemH + 5;
        }

        myDeleteButton->SetPosition(wxPoint(size.x - 35, 0));
        myHLine->SetSize(size.x - 345, wxDefaultCoord);
        myAddLedMappingButton->SetPosition(wxPoint(0, contentH));

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

    void OnAddLedMappingRequested(wxCommandEvent& event)
    {
        LightsTab::Change change;
        change.type = LightsTab::Change::ADD_LED_MAPPING;
        change.lightRuleIndex = myLightRuleIndex;
        myOwner->ProcessChange(change);
    }

    void OnDeleteLedMappingRequested(int ledMappingIndex)
    {
        LightsTab::Change change;
        change.type = LightsTab::Change::REMOVE_LED_MAPPING;
        change.lightRuleIndex = myLightRuleIndex;
        change.ledMappingIndex = ledMappingIndex;
        myOwner->ProcessChange(change);
    }

    void OnDeleteRequested(wxCommandEvent& event)
    {
        LightsTab::Change change;
        change.type = LightsTab::Change::REMOVE_LIGHT_RULE;
        change.lightRuleIndex = myLightRuleIndex;
        myOwner->ProcessChange(change);
    }

    void AddLedMapping(int ledMappingIndex, const LedMapping& mapping)
    {
        auto item = new LedMappingPanel(this, this, ledMappingIndex, mapping);
        myLedMappings.push_back(item);
        item->SendToDevice();
    }

    void RemoveLedMapping(int ledMappingIndex)
    {
        auto it = std::find_if(myLedMappings.begin(), myLedMappings.end(), [=](const LedMappingPanel* m) {
            return m->GetLedMappingIndex() == ledMappingIndex;
        });

        if (it != myLedMappings.end())
        {
            (*it)->Destroy();
            myLedMappings.erase(it);
        }
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

    const std::vector<LedMappingPanel*>& GetLedMappings() { return myLedMappings; }

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
    wxButton* myAddLedMappingButton;
    std::vector<LedMappingPanel*> myLedMappings;
    int myLightRuleIndex;
};

BEGIN_EVENT_TABLE(LightRulePanel, wxWindow)
    EVT_BUTTON(ADD_LED_MAPPING, LightRulePanel::OnAddLedMappingRequested)
    EVT_BUTTON(DELETE_LIGHT_RULE, LightRulePanel::OnDeleteRequested)
END_EVENT_TABLE()

// NOTE: defined here because the implementation needs to know RequestDeleteLedMapping.
void LedMappingPanel::OnDeleteRequested(wxCommandEvent& event)
{
    myOwner->OnDeleteLedMappingRequested(myLedMappingIndex);
}

// ====================================================================================================================
// Lights tab.
// ====================================================================================================================

const wchar_t* LightsTab::Title = L"Lights";

LightsTab::LightsTab(wxWindow* owner, const LightsState* state)
    : wxScrolledWindow(owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL)
{
    myAddSettingButton = new wxButton(this, ADD_LIGHT_RULE, L"Add light setting", wxDefaultPosition, wxSize(200, 30));

    UpdateSettings(state);
    SetScrollRate(5, 5);
}

void LightsTab::UpdateSettings(const LightsState* state)
{
    for (auto setting : myLightRules)
        setting->Destroy();

    myLightRules.clear();

    for (auto& rule : state->lightRules)
    {
        auto item = new LightRulePanel(this, rule.first, rule.second, state);
        myLightRules.push_back(item);
    }

    RecomputeLayout();
}

void LightsTab::HandleChanges(DeviceChanges changes)
{
    if (changes & DCF_LIGHTS)
    {
        auto lights = Device::Lights();
        if (lights)
            UpdateSettings(lights);
    }
}

void LightsTab::OnAddLightRuleRequested(wxCommandEvent& event)
{
    LightsTab::Change change;
    change.type = LightsTab::Change::ADD_LIGHT_RULE;
    ProcessChange(change);
}

void LightsTab::OnResize(wxSizeEvent& event)
{
    RecomputeLayout();
}

void LightsTab::RecomputeLayout()
{
    auto size = GetClientSize();
    int margin = 5;
    int contentH = margin;

    for (auto item : myLightRules)
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

void LightsTab::ProcessChange(const Change& change)
{
    // Aggregate indices of settings that were previously used.

    std::set<int> previousLightRules;
    std::set<int> previousLedMappings;

    for (auto rule : myLightRules)
    {
        previousLightRules.insert(rule->GetLightRuleIndex());
        for (auto mapping : rule->GetLedMappings())
            previousLedMappings.insert(mapping->GetLedMappingIndex());
    }

    // Assign new indices to the settings in incremental order without gaps.

    int lightRuleIndex = 0;
    int ledMappingIndex = 0;

    if (change.type == Change::REMOVE_LIGHT_RULE)
    {
        auto it = std::find_if(myLightRules.begin(), myLightRules.end(), [=](const LightRulePanel* r) {
            return r->GetLightRuleIndex() == change.lightRuleIndex;
        });

        if (it != myLightRules.end())
        {
            (*it)->Destroy();
            myLightRules.erase(it);
        }
    }

    for (auto rule : myLightRules)
    {
        bool isTargetRule = change.lightRuleIndex == rule->GetLightRuleIndex();

        rule->UpdateIndex(lightRuleIndex);

        if (change.type == Change::REMOVE_LED_MAPPING && isTargetRule)
        {
            rule->RemoveLedMapping(change.ledMappingIndex);
        }

        for (auto mapping : rule->GetLedMappings())
        {
            mapping->UpdateIndices(lightRuleIndex, ledMappingIndex);
            ++ledMappingIndex;
        }

        if (change.type == Change::ADD_LED_MAPPING && isTargetRule)
        {
            rule->AddLedMapping(ledMappingIndex, NewLedMapping(lightRuleIndex));
            ++ledMappingIndex;
        }

        ++lightRuleIndex;
    }

    if (change.type == Change::ADD_LIGHT_RULE)
    {
        auto item = new LightRulePanel(this, lightRuleIndex, NewLightRule());
        myLightRules.push_back(item);
        item->SendToDevice();
        ++lightRuleIndex;
    }

    // Previously used indices that were not reassigned to new settings should be disabled.

    for (auto index : previousLightRules)
    {
        if (index >= lightRuleIndex)
            Device::DisableLightRule(index);
    }
    for (auto index : previousLedMappings)
    {
        if (index >= ledMappingIndex)
            Device::DisableLedMapping(index);
    }

    RecomputeLayout();
}

BEGIN_EVENT_TABLE(LightsTab, wxWindow)
    EVT_BUTTON(ADD_LIGHT_RULE, LightsTab::OnAddLightRuleRequested)
    EVT_SIZE(LightsTab::OnResize)
END_EVENT_TABLE()

}; // namespace adp.
