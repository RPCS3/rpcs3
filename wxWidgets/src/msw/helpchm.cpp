/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/helpchm.cpp
// Purpose:     Help system: MS HTML Help implementation
// Author:      Julian Smart
// Modified by:
// Created:     16/04/2000
// RCS-ID:      $Id: helpchm.cpp 60697 2009-05-20 13:15:24Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HELP && wxUSE_MS_HTML_HELP

#include "wx/filefn.h"
#include "wx/msw/helpchm.h"

#include "wx/dynload.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/app.h"
#endif

#include "wx/msw/private.h"
#include "wx/msw/htmlhelp.h"

// ----------------------------------------------------------------------------
// utility functions to manage the loading/unloading
// of hhctrl.ocx
// ----------------------------------------------------------------------------

#ifndef UNICODE
    typedef HWND ( WINAPI * HTMLHELP )( HWND, LPCSTR, UINT, DWORD );
    #define HTMLHELP_NAME wxT("HtmlHelpA")
#else // ANSI
    typedef HWND ( WINAPI * HTMLHELP )( HWND, LPCWSTR, UINT, DWORD );
    #define HTMLHELP_NAME wxT("HtmlHelpW")
#endif

HTMLHELP GetHtmlHelpFunction()
{
    static HTMLHELP s_htmlHelp = NULL;

    if ( !s_htmlHelp )
    {
        static wxDynamicLibrary s_dllHtmlHelp(_T("HHCTRL.OCX"), wxDL_VERBATIM);

        if ( !s_dllHtmlHelp.IsLoaded() )
        {
            wxLogError(_("MS HTML Help functions are unavailable because the MS HTML Help library is not installed on this machine. Please install it."));
        }
        else
        {
            s_htmlHelp = (HTMLHELP)s_dllHtmlHelp.GetSymbol(HTMLHELP_NAME);
            if ( !s_htmlHelp )
            {
                wxLogError(_("Failed to initialize MS HTML Help."));
            }
        }
    }

    return s_htmlHelp;
}

// find the window to use in HtmlHelp() call: use the given one by default but
// fall back to the top level app window and then the desktop if it's NULL
static HWND GetSuitableHWND(wxWindow *win)
{
    if ( !win && wxTheApp )
        win = wxTheApp->GetTopWindow();

    return win ? GetHwndOf(win) : ::GetDesktopWindow();
}

// wrap the real HtmlHelp() but just return false (and not crash) if it
// couldn't be loaded
//
// it also takes a wxWindow instead of HWND
static bool
CallHtmlHelpFunction(wxWindow *win, const wxChar *str, UINT uint, DWORD dword)
{
    HTMLHELP htmlHelp = GetHtmlHelpFunction();

    return htmlHelp && htmlHelp(GetSuitableHWND(win), str, uint, dword);
}

IMPLEMENT_DYNAMIC_CLASS(wxCHMHelpController, wxHelpControllerBase)

bool wxCHMHelpController::Initialize(const wxString& filename)
{
    if ( !GetHtmlHelpFunction() )
        return false;

    m_helpFile = filename;
    return true;
}

bool wxCHMHelpController::LoadFile(const wxString& file)
{
    if (!file.IsEmpty())
        m_helpFile = file;
    return true;
}

bool wxCHMHelpController::DisplayContents()
{
    if (m_helpFile.IsEmpty())
        return false;

    wxString str = GetValidFilename(m_helpFile);

    return CallHtmlHelpFunction(GetParentWindow(), str, HH_DISPLAY_TOPIC, 0L);
}

// Use topic or HTML filename
bool wxCHMHelpController::DisplaySection(const wxString& section)
{
    if (m_helpFile.IsEmpty())
        return false;

    wxString str = GetValidFilename(m_helpFile);

    // Is this an HTML file or a keyword?
    if ( section.Find(wxT(".htm")) != wxNOT_FOUND )
    {
        // interpret as a file name
        return CallHtmlHelpFunction(GetParentWindow(), str, HH_DISPLAY_TOPIC,
                                    wxPtrToUInt(section.c_str()));
    }

    return KeywordSearch(section);
}

// Use context number
bool wxCHMHelpController::DisplaySection(int section)
{
    if (m_helpFile.IsEmpty())
        return false;

    wxString str = GetValidFilename(m_helpFile);

    return CallHtmlHelpFunction(GetParentWindow(), str, HH_HELP_CONTEXT,
                                (DWORD)section);
}

bool wxCHMHelpController::DisplayContextPopup(int contextId)
{
    if (m_helpFile.IsEmpty()) return false;

    wxString str = GetValidFilename(m_helpFile);

    // We also have to specify the popups file (default is cshelp.txt).
    // str += wxT("::/cshelp.txt");

    HH_POPUP popup;
    popup.cbStruct = sizeof(popup);
    popup.hinst = (HINSTANCE) wxGetInstance();
    popup.idString = contextId ;

    GetCursorPos(& popup.pt);
    popup.clrForeground = ::GetSysColor(COLOR_INFOTEXT);
    popup.clrBackground = ::GetSysColor(COLOR_INFOBK);
    popup.rcMargins.top = popup.rcMargins.left = popup.rcMargins.right = popup.rcMargins.bottom = -1;
    popup.pszFont = NULL;
    popup.pszText = NULL;

    return CallHtmlHelpFunction(GetParentWindow(), str, HH_DISPLAY_TEXT_POPUP,
                                wxPtrToUInt(&popup));
}

bool
wxCHMHelpController::DisplayTextPopup(const wxString& text, const wxPoint& pos)
{
    return ShowContextHelpPopup(text, pos, GetParentWindow());
}

/* static */
bool wxCHMHelpController::ShowContextHelpPopup(const wxString& text,
                                               const wxPoint& pos,
                                               wxWindow *window)
{
    HH_POPUP popup;
    popup.cbStruct = sizeof(popup);
    popup.hinst = (HINSTANCE) wxGetInstance();
    popup.idString = 0 ;
    popup.pt.x = pos.x; popup.pt.y = pos.y;
    popup.clrForeground = (COLORREF)-1;
    popup.clrBackground = (COLORREF)-1;
    popup.rcMargins.top = popup.rcMargins.left = popup.rcMargins.right = popup.rcMargins.bottom = -1;
    popup.pszFont = NULL;
    popup.pszText = (const wxChar*) text;

    return CallHtmlHelpFunction(window, NULL, HH_DISPLAY_TEXT_POPUP,
                                wxPtrToUInt(&popup));
}

bool wxCHMHelpController::DisplayBlock(long block)
{
    return DisplaySection(block);
}

bool wxCHMHelpController::KeywordSearch(const wxString& k,
                                        wxHelpSearchMode WXUNUSED(mode))
{
    if (m_helpFile.IsEmpty())
        return false;

    wxString str = GetValidFilename(m_helpFile);

    HH_AKLINK link;
    link.cbStruct =     sizeof(HH_AKLINK) ;
    link.fReserved =    FALSE ;
    link.pszKeywords =  k.c_str() ;
    link.pszUrl =       NULL ;
    link.pszMsgText =   NULL ;
    link.pszMsgTitle =  NULL ;
    link.pszWindow =    NULL ;
    link.fIndexOnFail = TRUE ;

    return CallHtmlHelpFunction(GetParentWindow(), str, HH_KEYWORD_LOOKUP,
                                wxPtrToUInt(&link));
}

bool wxCHMHelpController::Quit()
{
    return CallHtmlHelpFunction(GetParentWindow(), NULL, HH_CLOSE_ALL, 0L);
}

// Append extension if necessary.
wxString wxCHMHelpController::GetValidFilename(const wxString& file) const
{
    wxString path, name, ext;
    wxSplitPath(file, & path, & name, & ext);

    wxString fullName;
    if (path.IsEmpty())
        fullName = name + wxT(".chm");
    else if (path.Last() == wxT('\\'))
        fullName = path + name + wxT(".chm");
    else
        fullName = path + wxT("\\") + name + wxT(".chm");
    return fullName;
}

#endif // wxUSE_HELP
