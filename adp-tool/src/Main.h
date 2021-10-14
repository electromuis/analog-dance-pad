#include "Adp.h"

#include "wx/wx.h"
#include "wx/notebook.h"

namespace adp {
	
static const wchar_t* TOOL_NAME = L"ADP Tool";

class MainWindow;

class Application : public wxApp
{
public:
    ~Application();

    bool OnInit() override;

    void Restart();

    int OnExit() override;

    wxString GetTempDir(bool create);
    wxString GetTempDir();

private:
    MainWindow* myWindow;
    bool doRestart = false;
};

}; // namespace adp.

wxDECLARE_APP(adp::Application);