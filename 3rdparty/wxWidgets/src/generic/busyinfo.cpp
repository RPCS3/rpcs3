/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/busyinfo.cpp
// Purpose:     Information window when app is busy
// Author:      Vaclav Slavik
// Copyright:   (c) 1999 Vaclav Slavik
// RCS-ID:      $Id: busyinfo.cpp 44898 2007-03-18 20:39:13Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_BUSYINFO

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/stattext.h"
    #include "wx/panel.h"
    #include "wx/utils.h"
#endif

#include "wx/busyinfo.h"

class WXDLLEXPORT wxInfoFrame : public wxFrame
{
public:
    wxInfoFrame(wxWindow *parent, const wxString& message);

private:
    DECLARE_NO_COPY_CLASS(wxInfoFrame)
};


wxInfoFrame::wxInfoFrame(wxWindow *parent, const wxString& message)
           : wxFrame(parent, wxID_ANY, wxT("Busy"),
                     wxDefaultPosition, wxDefaultSize,
#if defined(__WXX11__)
                     wxRESIZE_BORDER
#else
                     wxSIMPLE_BORDER
#endif
                     | wxFRAME_TOOL_WINDOW)
{
    wxPanel *panel = new wxPanel( this );
    wxStaticText *text = new wxStaticText(panel, wxID_ANY, message);

    panel->SetCursor(*wxHOURGLASS_CURSOR);
    text->SetCursor(*wxHOURGLASS_CURSOR);

    // make the frame of at least the standard size (400*80) but big enough
    // for the text we show
    wxSize sizeText = text->GetBestSize();
#ifdef __WXPM__
    int                             nX = 0;
    int                             nY = 0;
    int                             nWidth = 0;
    int                             nHeight = 0;
    int                             nParentHeight = parent->GetClientSize().y;
    int                             nParentWidth = parent->GetClientSize().x;
    int                             nColor;

    SetBackgroundColour(wxT("WHITE"));
    nColor = (LONG)GetBackgroundColour().GetPixel();

    ::WinSetPresParam( GetHwnd()
                      ,PP_BACKGROUNDCOLOR
                      ,sizeof(LONG)
                      ,(PVOID)&nColor
                     );
    panel->SetBackgroundColour(wxT("WHITE"));
    nColor = (LONG)panel->GetBackgroundColour().GetPixel();

    ::WinSetPresParam( GetHwndOf(panel)
                      ,PP_BACKGROUNDCOLOR
                      ,sizeof(LONG)
                      ,(PVOID)&nColor
                     );
    nWidth = wxMax(sizeText.x, 340) + 60;
    nHeight = wxMax(sizeText.y, 40) + 40;
    nX = (nParentWidth - nWidth) / 2;
    nY = (nParentHeight / 2) - (nHeight / 2);
    nY = nParentHeight - (nY + nHeight);
    ::WinSetWindowPos( m_hFrame
                      ,HWND_TOP
                      ,nX
                      ,nY
                      ,nWidth
                      ,nHeight
                      ,SWP_SIZE | SWP_MOVE | SWP_ACTIVATE
                     );
    text->SetBackgroundColour(wxT("WHITE"));
    nColor = (LONG)text->GetBackgroundColour().GetPixel();

    ::WinSetPresParam( GetHwndOf(text)
                      ,PP_BACKGROUNDCOLOR
                      ,sizeof(LONG)
                      ,(PVOID)&nColor
                     );
    text->Center(wxBOTH);
#else
    SetClientSize(wxMax(sizeText.x, 340) + 60, wxMax(sizeText.y, 40) + 40);

    // need to size the panel correctly first so that text->Centre() works
    panel->SetSize(GetClientSize());

    text->Centre(wxBOTH);
    Centre(wxBOTH);
#endif
}

wxBusyInfo::wxBusyInfo(const wxString& message, wxWindow *parent)
{
    m_InfoFrame = new wxInfoFrame( parent, message);
    if ( parent && parent->HasFlag(wxSTAY_ON_TOP) )
    {
        // we must have this flag to be in front of our parent if it has it
        m_InfoFrame->SetWindowStyleFlag(wxSTAY_ON_TOP);
    }

    m_InfoFrame->Show(true);
    m_InfoFrame->Refresh();
    m_InfoFrame->Update();
}

wxBusyInfo::~wxBusyInfo()
{
    m_InfoFrame->Show(false);
    m_InfoFrame->Close();
}

#endif // wxUSE_BUSYINFO
