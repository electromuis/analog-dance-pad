#include "wx/wxprec.h"
#include "wx/setup.h"

#include "devices.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;
};
wxIMPLEMENT_APP_CONSOLE(MyApp);

class BasicDrawPane : public wxPanel
{
public:
    BasicDrawPane(wxFrame* parent);

    void paintEvent(wxPaintEvent& evt);
    void paintNow();
    void render(wxDC& dc);
    void resizeEvent(wxSizeEvent& evt);

    DECLARE_EVENT_TABLE()

private:
    wxBitmap* myBitmap;
};

BEGIN_EVENT_TABLE(BasicDrawPane, wxPanel)
    EVT_PAINT(BasicDrawPane::paintEvent)
    EVT_SIZE(BasicDrawPane::resizeEvent)
END_EVENT_TABLE()

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

private:
    BasicDrawPane* myDrawPane;
};

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    MyFrame* frame = new MyFrame("FSR Mini Pad Config");
    frame->Show();

    DeviceManager::init();

    return true;
}

int MyApp::OnExit()
{
    DeviceManager::shutdown();
    return wxApp::OnExit();
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title)
{
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this program");

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");

    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    myDrawPane = new BasicDrawPane(this);
    sizer->Add(myDrawPane, 1, wxEXPAND);
    SetSizer(sizer);

    Bind(wxEVT_MENU, &MyFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyFrame::OnIdle));

    CreateStatusBar(2);
    SetStatusText("Welcome to wxWidgets!");
}

void MyFrame::OnQuit(wxCommandEvent& event)
{
    if (!Close(true))
        return;
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("FSR Mini Pad Config", "About", wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnIdle(wxIdleEvent& event)
{
    if (DeviceManager::readSensorValues())
    {
        myDrawPane->Refresh(false);
    }
    event.RequestMore();
}

BasicDrawPane::BasicDrawPane(wxFrame* parent) :
    wxPanel(parent)
{
}

void BasicDrawPane::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void BasicDrawPane::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}

void BasicDrawPane::resizeEvent(wxSizeEvent& event)
{
    wxClientDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
}

void BasicDrawPane::render(wxDC& dc)
{
    auto device = DeviceManager::getDevice();
    if (!device)
        return;

    int w, h;
    dc.GetSize(&w, &h);
    dc.SetUserScale(w * 0.006, h * 0.006);

    int x = 10;
    for (int i = 0; i < device->numSensors(); ++i)
    {
        auto& sensor = device->sensor(i);
        if (!sensor.isMapped)
            continue;

        int barHeight = (sensor.value * 100) / 850;

        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(x, 10, 20, 100 - barHeight);
        dc.SetBrush(*wxYELLOW_BRUSH);
        dc.DrawRectangle(x, 110 - barHeight, 20, barHeight);

        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(x, 120, 20, 20);

        if (sensor.isPressed)
        {
            dc.SetBrush(*wxGREEN_BRUSH);
            dc.DrawRectangle(x + 2, 122, 16, 16);
        }

        x += 30;
    }
}