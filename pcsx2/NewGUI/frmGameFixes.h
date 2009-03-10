
#include <wx/wx.h>
#include <wx/image.h>

#pragma once

class frmGameFixes: public wxDialog
{
public:

    frmGameFixes(wxWindow* parent, int id, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

protected:

    DECLARE_EVENT_TABLE();

public:
    void FPUCompareHack_Click(wxCommandEvent &event);
    void TriAce_Click(wxCommandEvent &event);
    void GodWar_Click(wxCommandEvent &event);
    void Ok_Click(wxCommandEvent &event);
    void Cancel_Click(wxCommandEvent &event);
};
