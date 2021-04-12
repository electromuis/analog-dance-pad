#include "wx/wxprec.h"
#include "wx/setup.h"
#include "wx/timer.h"
#include "wx/notebook.h"

#include "View/IdleTab.h"
#include "View/SensitivityTab.h"
#include "View/MappingTab.h"
#include "View/DeviceTab.h"
#include "View/AboutTab.h"
#include "View/LogTab.h"
#include "View/Style.h"

#include "Model/Logging.h"

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
        myTabs->AddPage(new AboutPage(myTabs), AboutPage::Title);
        myTabs->AddPage(new LogPage(myTabs), LogPage::Title);
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
        auto changes = DeviceManager::Update();

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

        auto pad = DeviceManager::Pad();
        if (pad)
        {
            myTabs->InsertPage(0, new SensitivityPage(myTabs, pad), SensitivityPage::Title, true);
            myTabs->InsertPage(1, new MappingPage(myTabs, pad), MappingPage::Title);
            myTabs->InsertPage(2, new DevicePage(myTabs), DevicePage::Title);
        }
        else
        {
            myTabs->InsertPage(0, new IdlePage(myTabs), IdlePage::Title, true);
        }
    }

    void UpdateStatusText()
    {
        auto pad = DeviceManager::Pad();
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

        DeviceManager::Init();
        Style::Init();

        wxImage::AddHandler(new wxPNGHandler());

        myWindow = new MainWindow();
        myWindow->Show();
        return true;
    }

    int OnExit() override
    {
        Style::Shutdown();
        DeviceManager::Shutdown();
        Log::Shutdown();

        return wxApp::OnExit();
    }

private:
    MainWindow* myWindow;
};
wxIMPLEMENT_APP(Application);

}; // namespace mpc.