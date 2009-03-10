
#include "PrecompiledHeader.h"
#include <wx/wx.h>
#include <wx/image.h>
#include "frmMain.h"



class Pcsx2GUI: public wxApp
{
public:
    bool OnInit();
};

IMPLEMENT_APP(Pcsx2GUI)

bool Pcsx2GUI::OnInit()
{
    wxInitAllImageHandlers();
    frmMain* frameMain = new frmMain( NULL, wxID_ANY, wxEmptyString );
    SetTopWindow( frameMain );
    frameMain->Show();
    return true;
}
