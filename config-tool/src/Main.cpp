#include "wx/wx.h"
#include "wx/notebook.h"

#include "Assets/Assets.h"

#include "View/IdleTab.h"
#include "View/SensitivityTab.h"
#include "View/MappingTab.h"
#include "View/LightsTab.h"
#include "View/DeviceTab.h"
#include "View/AboutTab.h"
#include "View/LogTab.h"

#include "Model/Log.h"

namespace mpc {

// ====================================================================================================================
// Main window.
// ====================================================================================================================

class MainWindow : public wxFrame
{
public:
    MainWindow() : wxFrame(nullptr, wxID_ANY, "FSR Mini Pad Config", wxDefaultPosition, wxSize(500, 500))
    {
        SetMinClientSize(wxSize(400, 400));

        SetStatusBar(CreateStatusBar(1));

        auto sizer = new wxBoxSizer(wxVERTICAL);
        myTabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_NOPAGETHEME);
        myTabs->AddPage(new AboutTab(myTabs), AboutTab::Title);
        myTabs->AddPage(new LogTab(myTabs), LogTab::Title);
        sizer->Add(myTabs, 1, wxEXPAND);
        SetSizer(sizer);

        UpdatePages();

        myUpdateTimer = make_unique<UpdateTimer>(this);
        myUpdateTimer->Start(10);
    }

    ~MainWindow()
    {
        myUpdateTimer->Stop();
    }

    void Tick()
    {
        auto changes = Device::Update();

        if (changes & DCF_DEVICE)
            UpdatePages();

        if (changes & (DCF_DEVICE | DCF_NAME))
            UpdateStatusText();

        auto page = (BaseTab*)myTabs->GetCurrentPage();
        if (page)
            page->Tick(changes);
    }

private:
    void UpdatePages()
    {
        // Delete all pages except "About" and "Log" at the end.
        while (myTabs->GetPageCount() > 2)
            myTabs->DeletePage(0);

        auto pad = Device::Pad();
        if (pad)
        {
            myTabs->InsertPage(0, new SensitivityTab(myTabs, pad), SensitivityTab::Title, true);
            myTabs->InsertPage(1, new MappingTab(myTabs, pad), MappingTab::Title);
            myTabs->InsertPage(2, new LightsTab(myTabs), LightsTab::Title);
            myTabs->InsertPage(3, new DeviceTab(myTabs), DeviceTab::Title);
        }
        else
        {
            myTabs->InsertPage(0, new IdleTab(myTabs), IdleTab::Title, true);
        }
    }

    void UpdateStatusText()
    {
        auto pad = Device::Pad();
        if (pad)
            SetStatusText(L"Connected to: " + pad->name);
        else
            SetStatusText(wxEmptyString);
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

        Assets::Init();
        Device::Init();

        wxImage::AddHandler(new wxPNGHandler());

        wxIconBundle icons;
        icons.AddIcon(Files::Icon16(), wxBITMAP_TYPE_PNG);
        icons.AddIcon(Files::Icon32(), wxBITMAP_TYPE_PNG);
        icons.AddIcon(Files::Icon64(), wxBITMAP_TYPE_PNG);

        myWindow = new MainWindow();
        myWindow->SetIcons(icons);
        myWindow->Show();


        return true;
    }

    int OnExit() override
    {
        Device::Shutdown();
        Assets::Shutdown();
        Log::Shutdown();

        return wxApp::OnExit();
    }

private:
    MainWindow* myWindow;
};
wxIMPLEMENT_APP(Application);

}; // namespace mpc.