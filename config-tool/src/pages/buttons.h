#pragma once

#include "pages/base.h"
#include "devices.h"

#include "wx/dataview.h"

#include <string>

using namespace devices;
using namespace std;

namespace pages {

class ButtonsPage : public BasePage
{
public:
    static constexpr const wchar_t* Title() { return L"Buttons"; }

    enum Ids { RESET_BUTTON = 1 };

    ButtonsPage(wxWindow* owner, const PadState* pad) : BasePage(owner)
    {
        myModel = new Model(pad);
        mySensorGrid = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES);
        mySensorGrid->AssociateModel(myModel);
        mySensorGrid->AppendTextColumn("Sensor", 0, wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT);
        mySensorGrid->AppendTextColumn("Button", 1, wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT);
        mySensorGrid->AppendColumn(new wxDataViewColumn("Reading", new ReadingRenderer(), 2, -1, wxALIGN_LEFT));
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(mySensorGrid, 1, wxEXPAND);

        wxArrayString options;
        options.Add("None");
        for (int i = 1; i <= pad->numButtons; ++i)
            options.Add(wxString::Format("Button %i", i));

        auto buttons = new wxBoxSizer(wxHORIZONTAL);
        myButtonBox = new wxComboBox(this, wxID_ANY, options[0],
            wxDefaultPosition, wxSize(120, 25), options, wxCB_READONLY);
        buttons->Add(myButtonBox);
        buttons->AddStretchSpacer(1);
        auto clearButton = new wxButton(this, wxID_ANY, L"Clear all", wxDefaultPosition, wxSize(120, 25));
        buttons->Add(clearButton);
        sizer->Add(buttons, 0, wxEXPAND | wxTOP, 4);

        SetSizer(sizer);

        mySensorGrid->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ButtonsPage::OnSelectedSensorChanged, this);
        myButtonBox->Bind(wxEVT_COMBOBOX, &ButtonsPage::OnSelectedButtonChanged, this);
        clearButton->Bind(wxEVT_BUTTON, &ButtonsPage::OnClearPressed, this);

        UpdateButtonBox(nullptr);
    }

    ~ButtonsPage() { myModel->DecRef(); }

    void Tick() override { myModel->Tick(); }

private:
    void OnSelectedSensorChanged(wxDataViewEvent& event)
    {
        auto item = mySensorGrid->GetSelection();
        if (!item)
        {
            UpdateButtonBox(nullptr);
        }
        else
        {
            int sensorIndex = myModel->GetRow(item);
            auto sensor = DeviceManager::Sensor(sensorIndex);
            UpdateButtonBox(&sensor->button);
        }
    }

    void OnSelectedButtonChanged(wxCommandEvent& event)
    {
        auto item = mySensorGrid->GetSelection();
        if (item)
        {
            int sensorIndex = myModel->GetRow(item);
            int button = myButtonBox->GetSelection();
            DeviceManager::SetButtonMapping(sensorIndex, button);
            UpdateButtonMapping();
        }
    }

    void OnClearPressed(wxCommandEvent& event)
    {
        DeviceManager::ClearButtonMapping();
        UpdateButtonMapping();
    }

    void UpdateButtonBox(const int* button)
    {
        myButtonBox->Enable(button != nullptr);
        myButtonBox->SetSelection(button ? *button : 0);
    }

    void UpdateButtonMapping()
    {
        for (uint32_t i = 0; i < myModel->GetRowCount(); ++i)
            myModel->RowValueChanged(i, 1);
    }

    class ReadingRenderer : public wxDataViewCustomRenderer
    {
    public:
        ReadingRenderer() : wxDataViewCustomRenderer("double")
        {
        }

        bool SetValue(const wxVariant& value) override
        {
            myValue = value.IsNull() ? 0.0 : value.GetDouble();
            return true;
        }

        bool GetValue(wxVariant& value) const { return false; }

        wxSize GetSize() const override { return wxSize(100, 20); }

        bool Render(wxRect cell, wxDC* dc, int state)
        {
            int barW = myValue * cell.width;
            dc->SetPen(wxNullPen);
            dc->SetBrush(*wxBLACK_BRUSH);
            dc->DrawRectangle(cell.x + barW, cell.y, cell.width - barW, cell.height);
            dc->SetBrush(*wxYELLOW_BRUSH);
            dc->DrawRectangle(cell.x, cell.y, barW, cell.height);
            return true;
        }

    private:
        double myValue;
    };

    class Model : public wxDataViewVirtualListModel
    {
    public:
        Model(const PadState* pad)
            : wxDataViewVirtualListModel(pad->numSensors)
            , myNumSensors(pad->numSensors)
        {
        }

        void Tick()
        {
            for (int i = 0; i < myNumSensors; ++i)
                RowValueChanged(i, 2);
        }

        void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override
        {
            auto sensor = DeviceManager::Sensor(row);
            switch (col)
            {
            case 0:
                variant = to_string(row);
                break;
            case 1:
                variant = (sensor->button == 0) ? string("-") : to_string(sensor->button);
                break;
            case 2:
                variant = sensor->value;
                break;
            }
        }

        bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override
        {
            return false;
        }

        wxString GetColumnType(unsigned int col) const override
        {
            switch (col)
            {
                case 0: return L"string";
                case 1: return L"string";
                case 2: return L"double";
            }
            return wxEmptyString;
        }

        uint32_t GetRowCount() const { return myNumSensors; }

        uint32_t GetColumnCount() const override { return 3; }

    private:
        int myNumSensors;
    };

    Model* myModel;
    wxDataViewCtrl* mySensorGrid;
    wxComboBox* myButtonBox;
};

}; // namespace pages.