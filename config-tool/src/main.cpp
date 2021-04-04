#include "wx/wxprec.h"
#include "wx/setup.h"
#include "wx/timer.h"
#include "wx/notebook.h"
#include "wx/object.h"
#include "wx/generic/statbmpg.h"
#include "wx/mstream.h"

#include <memory>

#include "devices.h"
#include "logging.h"
#include "logo.h"

using namespace std;
using namespace devices;
using namespace logging;

class BasePage : public wxWindow
{
public:
    BasePage(wxWindow* owner) : wxWindow(owner, wxID_ANY) {}

    virtual void Tick() {}
};

class SensorPage : public BasePage
{
public:
    SensorPage(wxWindow* owner)
        : BasePage(owner)
    {
        SetDoubleBuffered(true);
    }

    void Tick() { Refresh(); }

    void OnPaint(wxPaintEvent& evt)
    {
        auto device = DeviceManager::Device();
        if (!device)
            return;

        wxPaintDC dc(this);
        int w, h;
        dc.GetSize(&w, &h);
        dc.SetUserScale(w * 0.006, h * 0.006);

        int x = 10;
        for (int i = 0; i < device->numSensors; ++i)
        {
            auto sensor = DeviceManager::Sensor(i);
            if (!sensor)
                continue;

            if (sensor->button == SensorState::UNMAPPED_BUTTON)
                continue;

            int barHeight = sensor->value * 100;

            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawRectangle(x, 10, 20, 100 - barHeight);
            dc.SetBrush(*wxYELLOW_BRUSH);
            dc.DrawRectangle(x, 110 - barHeight, 20, barHeight);

            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawRectangle(x, 120, 20, 20);

            if (sensor->pressed)
            {
                dc.SetBrush(*wxGREEN_BRUSH);
                dc.DrawRectangle(x + 2, 122, 16, 16);
            }

            x += 30;
        }
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SensorPage, wxPanel)
    EVT_PAINT(SensorPage::OnPaint)
END_EVENT_TABLE()

// ====================================================================================================================
// No device page.
// ====================================================================================================================

class NoDevicePage : public BasePage
{
public:
    NoDevicePage(wxWindow* owner) : BasePage(owner)
    {
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        auto text = new wxStaticText(this, wxID_ANY, "Connect an FSR Mini Pad to continue.",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
        sizer->Add(text, 1, wxCENTER);
        SetSizer(sizer);
    }
};

// ====================================================================================================================
// Log page.
// ====================================================================================================================

class LogPage : public BasePage
{
public:
    LogPage(wxWindow* owner) : BasePage(owner)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        myText = new wxTextCtrl(this, wxID_ANY, L"", wxDefaultPosition, wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE);
        sizer->Add(myText, 1, wxEXPAND);
        SetSizer(sizer);
    }
    void Tick() override
    {
        for (int numMessages = Log::NumMessages(); myNumMessages < numMessages; ++myNumMessages)
        {
            if (myNumMessages > 0)
                myText->AppendText(L"\n");
            myText->AppendText(Log::Message(myNumMessages));
        }
    }
private:
    wxTextCtrl* myText;
    int myNumMessages = 0;
};

// ====================================================================================================================
// About page.
// ====================================================================================================================

class AboutPage : public BasePage
{
public:
    AboutPage(wxWindow* owner) : BasePage(owner)
    {
        wxMemoryInputStream stream(LOGO_PNG, LOGO_PNG_SIZE);
        wxImage logo(stream, wxBITMAP_TYPE_PNG);
        auto sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(new wxStaticText(
            this, wxID_ANY, L"FSR Mini Pad Config Tool"), 0, wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);
        sizer->Add(new wxGenericStaticBitmap(this, wxID_ANY, wxBitmap(logo)), 0, wxALIGN_CENTER_HORIZONTAL);
        sizer->Add(new wxStaticText(
            this, wxID_ANY, L"\xA9 Bram van de Wetering 2021"), 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, 10);
        sizer->AddStretchSpacer();
        SetSizer(sizer);
    }
};

// ====================================================================================================================
// Main window.
// ====================================================================================================================

class MainWindow : public wxFrame
{
public:
    MainWindow() : wxFrame(nullptr, wxID_ANY, "FSR Mini Pad Config", wxDefaultPosition, wxSize(500, 500))
    {
        SetStatusBar(CreateStatusBar(1));

        auto sizer = new wxBoxSizer(wxVERTICAL);
        myTabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_NOPAGETHEME);
        myTabs->AddPage(new AboutPage(myTabs), L"About");
        myTabs->AddPage(new LogPage(myTabs), L"Log");
        sizer->Add(myTabs, 1, wxEXPAND);
        SetSizer(sizer);

        UpdateDevice();

        myUpdateTimer = make_unique<UpdateTimer>(this);
        myUpdateTimer->Start(10);
    }

    ~MainWindow()
    {
        myUpdateTimer->Stop();
    }

    void Tick()
    {
        auto result = DeviceManager::Update();

        if (result.deviceChanged)
            UpdateDevice();

        auto page = (BasePage*)myTabs->GetCurrentPage();
        if (page)
            page->Tick();
    }

private:
    void UpdateDevice()
    {
        // [dynamic pages] | About | Log
        while (myTabs->GetPageCount() > 2)
            myTabs->DeletePage(0);

        auto device = DeviceManager::Device();
        if (device)
        {
            UpdateConnectedToStatus(device);
            myTabs->InsertPage(0, new SensorPage(myTabs), L"Sensors", true);
        }
        else
        {
            SetStatusText(wxEmptyString);
            myTabs->InsertPage(0, new NoDevicePage(myTabs), L"Device", true);
        }
    }

    void UpdateConnectedToStatus(const DeviceState* device)
    {
        SetStatusText(L"Connected to: " + device->name);
    }

    struct UpdateTimer : public wxTimer
    {
        UpdateTimer(MainWindow* owner) : owner(owner) {}
        void Notify() override { owner->Tick(); }
        MainWindow* owner;
    };

    // BasicDrawPane* myDrawPane;
    wxNotebook* myTabs;
    unique_ptr<wxTimer> myUpdateTimer;
};

// ====================================================================================================================
// Application.
// ====================================================================================================================

class Application : public wxApp
{
public:
    bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        Log::Init();

        auto now = wxDateTime::Now().FormatISOCombined(' ');
        Log::Writef(L"Application started: %s", now.wc_str());

        DeviceManager::Init();

        wxImage::AddHandler(new wxPNGHandler());

        myWindow = new MainWindow();
        myWindow->Show();
        return true;
    }

    int OnExit() override
    {
        DeviceManager::Shutdown();
        Log::Shutdown();

        return wxApp::OnExit();
    }

private:
    MainWindow* myWindow;
};
wxIMPLEMENT_APP(Application);