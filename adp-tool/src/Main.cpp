#include "Adp.h"

#include <iostream>
#include <vector>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "wx/wx.h"
#include "wx/notebook.h"
#include "wx/wfstream.h"
#include "wx/sstream.h"

#include "Assets/Assets.h"

#include "View/BaseTab.h"
#include "View/IdleTab.h"
#include "View/SensitivityTab.h"
#include "View/MappingTab.h"
#include "View/LightsTab.h"
#include "View/DeviceTab.h"
#include "View/AboutTab.h"
#include "View/LogTab.h"

#include "Model/Log.h"

namespace adp {
	
static const wchar_t* TOOL_NAME = L"ADP Tool";

enum Ids { PROFILE_LOAD = 1, PROFILE_SAVE = 2, MENU_EXIT = 3};


// ====================================================================================================================
// Main window.
// ====================================================================================================================

class MainWindow : public wxFrame
{
public:
    MainWindow(wxApp* app, const wchar_t* versionString)
        : wxFrame(nullptr, wxID_ANY, TOOL_NAME, wxDefaultPosition, wxSize(500, 500))
        , myApp(app)
    {
        SetMinClientSize(wxSize(400, 400));
        SetStatusBar(CreateStatusBar(2));

        wxMenuBar* menuBar = new wxMenuBar();
        wxMenu* fileMenu = new wxMenu();

        menuBar->Append(fileMenu, wxT("File"));
        
        fileMenu->Append(PROFILE_LOAD, wxT("Load profile"));
        fileMenu->Append(PROFILE_SAVE, wxT("Save profile"));
        fileMenu->Append(MENU_EXIT, wxT("Exit"));
        
        SetMenuBar(menuBar);

        auto sizer = new wxBoxSizer(wxVERTICAL);
        myTabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_NOPAGETHEME);
        AddTab(0, new AboutTab(myTabs, versionString), AboutTab::Title);
        AddTab(1, new LogTab(myTabs), LogTab::Title);
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

    void ProfileLoad(wxCommandEvent & event)
    {
		if(!Device::Pad()) {
			return;
		}
		
        wxFileDialog dlg(this, L"Open XYZ file", L"", L"", L"ADP profile (*.json)|*.json",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (dlg.ShowModal() == wxID_CANCEL)
            return;
	
		ifstream fileStream;
		fileStream.open((std::string)dlg.GetPath());
		if(!fileStream.is_open())
		{
			Log::Writef(L"Could not read profile: %s", dlg.GetPath());
            return;
        }


		json j;
		
		fileStream >> j;
		fileStream.close();
		
		Device::LoadProfile(j, DGP_ALL);
    }

    void ProfileSave(wxCommandEvent & event)
    {
		if(!Device::Pad()) {
			return;
		}
		
        wxFileDialog dlg(this, L"Save XYZ file", L"", L"", L"ADP profile (*.json)|*.json",
        wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (dlg.ShowModal() == wxID_CANCEL)
            return;

        wxFileOutputStream output_stream(dlg.GetPath());
        if (!output_stream.IsOk())
        {
			Log::Writef(L"Could not save profile: %s", dlg.GetPath());
            return;
        }

        json j;
		
        Device::SaveProfile(j, DGP_ALL);

        wxStringInputStream input_stream(wxString(j.dump()));
        output_stream.Write(input_stream);
        output_stream.Close();
    }

    void Tick()
    {
        auto changes = Device::Update();

        if (changes & DCF_DEVICE)
            UpdatePages();

        if (changes & (DCF_DEVICE | DCF_NAME))
            UpdateStatusText();

        wstring debugMessage = Device::ReadDebug();

        if (!debugMessage.empty()) {
            Log::Writef(L"\\/ \\/ \\/ Debug \\/ \\/ \\/\n%ls", debugMessage.c_str());
        }

        UpdatePollingRate();

        if (changes)
        {
            for (auto tab : myTabList)
                tab->HandleChanges(changes);
        }

        auto activeTab = GetActiveTab();
        if (activeTab)
            activeTab->Tick();
    }

    void CloseApp(wxCommandEvent & event)
    {
        myUpdateTimer->Stop();
        myApp->ExitMainLoop();
        event.Skip(); // Default handler will close window.
    }

    void OnClose(wxCloseEvent& event)
    {
        myUpdateTimer->Stop();
        myApp->ExitMainLoop();
        event.Skip(); // Default handler will close window.
    }

    DECLARE_EVENT_TABLE()

private:
    void UpdatePages()
    {
        // Delete all pages except "About" and "Log" at the end.
        while (myTabs->GetPageCount() > 2)
        {
            myTabList.erase(myTabList.begin());
            myTabs->DeletePage(0);
        }

        auto pad = Device::Pad();
        if (pad)
        {
            AddTab(0, new SensitivityTab(myTabs, pad), SensitivityTab::Title, true);
            AddTab(1, new MappingTab(myTabs, pad), MappingTab::Title);
            AddTab(2, new DeviceTab(myTabs), DeviceTab::Title);
            auto lights = Device::Lights();
            if (lights)
            {
                AddTab(3, new LightsTab(myTabs, lights), LightsTab::Title);
            }
        }
        else
        {
            AddTab(0, new IdleTab(myTabs), IdleTab::Title, true);
        }
    }

    void UpdateStatusText()
    {
        auto pad = Device::Pad();
        if (pad)
            SetStatusText(L"Connected to: " + pad->name, 0);
        else
            SetStatusText(wxEmptyString, 0);
    }

    void UpdatePollingRate()
    {
        auto rate = Device::PollingRate();
        if (rate > 0)
            SetStatusText(wxString::Format("%iHz", rate), 1);
        else
            SetStatusText(wxEmptyString, 1);
    }

    void AddTab(int index, BaseTab* tab, const wchar_t* title, bool select = false)
    {
        myTabs->InsertPage(index, tab->GetWindow(), title, select);
        myTabList.insert(myTabList.begin() + index, tab);
    }

    BaseTab* GetActiveTab()
    {
        auto page = myTabs->GetCurrentPage();
        for (auto tab : myTabList)
        {
            if (tab->GetWindow() == page)
                return tab;
        }
        return nullptr;
    }

    struct UpdateTimer : public wxTimer
    {
        UpdateTimer(MainWindow* owner) : owner(owner) {}
        void Notify() override { owner->Tick(); }
        MainWindow* owner;
    };

    wxApp* myApp;
    wxNotebook* myTabs;
    vector<BaseTab*> myTabList;
    unique_ptr<wxTimer> myUpdateTimer;
};

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_CLOSE(MainWindow::OnClose)
    EVT_MENU(MENU_EXIT, MainWindow::CloseApp)
    EVT_MENU(PROFILE_LOAD, MainWindow::ProfileLoad)
    EVT_MENU(PROFILE_SAVE, MainWindow::ProfileSave)
END_EVENT_TABLE()

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

        auto versionString = wstring(TOOL_NAME) + L" " +
            to_wstring(ADP_VERSION_MAJOR) + L"." + to_wstring(ADP_VERSION_MINOR);

        auto now = wxDateTime::Now().FormatISOCombined(' ');
        Log::Writef(L"Application started: %ls - %ls", versionString.data(), now.wc_str());

        Assets::Init();
        Device::Init();

        wxImage::AddHandler(new wxPNGHandler());

        wxIconBundle icons;

        //todo fix linux
#ifdef _MSC_VER
        icons.AddIcon(Files::Icon16(), wxBITMAP_TYPE_PNG);
        icons.AddIcon(Files::Icon32(), wxBITMAP_TYPE_PNG);
        icons.AddIcon(Files::Icon64(), wxBITMAP_TYPE_PNG);
#endif // _MSC_VER

        myWindow = new MainWindow(this, versionString.data());
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

}; // namespace adp.

wxIMPLEMENT_APP(adp::Application);
