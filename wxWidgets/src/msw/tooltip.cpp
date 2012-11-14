///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/tooltip.cpp
// Purpose:     wxToolTip class implementation for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     31.01.99
// RCS-ID:      $Id: tooltip.cpp 62542 2009-11-03 14:11:01Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TOOLTIPS

#include "wx/tooltip.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/app.h"
    #include "wx/control.h"
    #include "wx/combobox.h"
#endif

#include "wx/tokenzr.h"
#include "wx/msw/private.h"

#ifndef TTTOOLINFO_V1_SIZE
    #define TTTOOLINFO_V1_SIZE 0x28
#endif

#ifndef TTF_TRANSPARENT
    #define TTF_TRANSPARENT 0x0100
#endif

// VZ: normally, the trick with subclassing the tooltip control and processing
//     TTM_WINDOWFROMPOINT should work but, somehow, it doesn't. I leave the
//     code here for now (but it's not compiled) in case we need it later.
//
//     For now I use an ugly workaround and process TTN_NEEDTEXT directly in
//     radio button wnd proc - fixing TTM_WINDOWFROMPOINT code would be nice
//     because it would then work for all controls, not only radioboxes but for
//     now I don't understand what's wrong with it...
#define wxUSE_TTM_WINDOWFROMPOINT   0

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// the tooltip parent window
WXHWND wxToolTip::ms_hwndTT = (WXHWND)NULL;

#if wxUSE_TTM_WINDOWFROMPOINT

// the tooltip window proc
static WNDPROC gs_wndprocToolTip = (WNDPROC)NULL;

#endif // wxUSE_TTM_WINDOWFROMPOINT

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// a wrapper around TOOLINFO Win32 structure
#ifdef __VISUALC__
    #pragma warning( disable : 4097 ) // we inherit from a typedef - so what?
#endif

class wxToolInfo : public TOOLINFO
{
public:
    wxToolInfo(HWND hwndOwner)
    {
        // initialize all members
        ::ZeroMemory(this, sizeof(TOOLINFO));

        // the structure TOOLINFO has been extended with a 4 byte field in
        // version 4.70 of comctl32.dll and another one in 5.01 but we don't
        // use these extended fields so use the old struct size to ensure that
        // the tooltips work on old (Windows 95) systems too
        cbSize = TTTOOLINFO_V1_SIZE;

        hwnd = hwndOwner;
        uFlags = TTF_IDISHWND;

        // we use TTF_TRANSPARENT to fix a problem which arises at least with
        // the text controls but may presumably happen with other controls
        // which display the tooltip at mouse position: it can start flashing
        // then as the control gets "focus lost" events and dismisses the
        // tooltip which then reappears because mouse remains hovering over the
        // control, see SF patch 1821229
        if ( wxApp::GetComCtl32Version() >= 470 )
        {
            uFlags |= TTF_TRANSPARENT;
        }

        uId = (UINT)hwndOwner;
    }
};

#ifdef __VISUALC__
    #pragma warning( default : 4097 )
#endif

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// send a message to the tooltip control if it exists
//
// NB: wParam is always 0 for the TTM_XXX messages we use
static inline LRESULT SendTooltipMessage(WXHWND hwnd, UINT msg, void *lParam)
{
    return hwnd ? ::SendMessage((HWND)hwnd, msg, 0, (LPARAM)lParam) : 0;
}

// send a message to all existing tooltip controls
static inline void
SendTooltipMessageToAll(WXHWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ( hwnd )
        ::SendMessage((HWND)hwnd, msg, wParam, lParam);
}

// ============================================================================
// implementation
// ============================================================================

#if wxUSE_TTM_WINDOWFROMPOINT

// ----------------------------------------------------------------------------
// window proc for our tooltip control
// ----------------------------------------------------------------------------

LRESULT APIENTRY wxToolTipWndProc(HWND hwndTT,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    if ( msg == TTM_WINDOWFROMPOINT )
    {
        LPPOINT ppt = (LPPOINT)lParam;

        // the window on which event occurred
        HWND hwnd = ::WindowFromPoint(*ppt);

        OutputDebugString("TTM_WINDOWFROMPOINT: ");
        OutputDebugString(wxString::Format("0x%08x => ", hwnd));

        // return a HWND corresponding to a wxWindow because only wxWidgets are
        // associated with tooltips using TTM_ADDTOOL
        wxWindow *win = wxGetWindowFromHWND((WXHWND)hwnd);

        if ( win )
        {
            hwnd = GetHwndOf(win);
            OutputDebugString(wxString::Format("0x%08x\r\n", hwnd));

#if 0
            // modify the point too!
            RECT rect;
            GetWindowRect(hwnd, &rect);

            ppt->x = (rect.right - rect.left) / 2;
            ppt->y = (rect.bottom - rect.top) / 2;
#endif // 0
            return (LRESULT)hwnd;
        }
        else
        {
            OutputDebugString("no window\r\n");
        }
    }

    return ::CallWindowProc(CASTWNDPROC gs_wndprocToolTip, hwndTT, msg, wParam, lParam);
}

#endif // wxUSE_TTM_WINDOWFROMPOINT

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

void wxToolTip::Enable(bool flag)
{
    // Make sure the tooltip has been created
    (void) GetToolTipCtrl();

    SendTooltipMessageToAll(ms_hwndTT, TTM_ACTIVATE, flag, 0);
}

void wxToolTip::SetDelay(long milliseconds)
{
    // Make sure the tooltip has been created
    (void) GetToolTipCtrl();

    SendTooltipMessageToAll(ms_hwndTT, TTM_SETDELAYTIME,
                            TTDT_INITIAL, milliseconds);
}

// ---------------------------------------------------------------------------
// implementation helpers
// ---------------------------------------------------------------------------

// create the tooltip ctrl for our parent frame if it doesn't exist yet
/* static */
WXHWND wxToolTip::GetToolTipCtrl()
{
    if ( !ms_hwndTT )
    {
        WXDWORD exflags = 0;
        if ( wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft )
        {
            exflags |= WS_EX_LAYOUTRTL;
        }

        // we want to show the tooltips always (even when the window is not
        // active) and we don't want to strip "&"s from them
        ms_hwndTT = (WXHWND)::CreateWindowEx(exflags,
                                             TOOLTIPS_CLASS,
                                             (LPCTSTR)NULL,
                                             TTS_ALWAYSTIP | TTS_NOPREFIX,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             NULL, (HMENU)NULL,
                                             wxGetInstance(),
                                             NULL);
       if ( ms_hwndTT )
       {
           HWND hwnd = (HWND)ms_hwndTT;
           SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

#if wxUSE_TTM_WINDOWFROMPOINT
           // subclass the newly created control
           gs_wndprocToolTip = wxSetWindowProc(hwnd, wxToolTipWndProc);
#endif // wxUSE_TTM_WINDOWFROMPOINT
       }
    }

    return ms_hwndTT;
}

/* static */
void wxToolTip::RelayEvent(WXMSG *msg)
{
    (void)SendTooltipMessage(GetToolTipCtrl(), TTM_RELAYEVENT, msg);
}

// ----------------------------------------------------------------------------
// ctor & dtor
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxToolTip, wxObject)

wxToolTip::wxToolTip(const wxString &tip)
         : m_text(tip)
{
    m_window = NULL;
}

wxToolTip::~wxToolTip()
{
    // the tooltip has to be removed before deleting. Otherwise, if it is visible
    // while being deleted, there will be a delay before it goes away.
    Remove();
}

// ----------------------------------------------------------------------------
// others
// ----------------------------------------------------------------------------

void wxToolTip::Remove(WXHWND hWnd)
{
    wxToolInfo ti((HWND)hWnd);
    (void)SendTooltipMessage(GetToolTipCtrl(), TTM_DELTOOL, &ti);
}

void wxToolTip::Remove()
{
    // remove this tool from the tooltip control
    if ( m_window )
    {
        Remove(m_window->GetHWND());
    }
}

void wxToolTip::Add(WXHWND hWnd)
{
    HWND hwnd = (HWND)hWnd;

    wxToolInfo ti(hwnd);

    // another possibility would be to specify LPSTR_TEXTCALLBACK here as we
    // store the tooltip text ourselves anyhow, and provide it in response to
    // TTN_NEEDTEXT (sent via WM_NOTIFY), but then we would be limited to 79
    // character tooltips as this is the size of the szText buffer in
    // NMTTDISPINFO struct -- and setting the tooltip here we can have tooltips
    // of any length
    ti.hwnd = hwnd;
    ti.lpszText = (wxChar *)m_text.c_str(); // const_cast

    if ( !SendTooltipMessage(GetToolTipCtrl(), TTM_ADDTOOL, &ti) )
    {
        wxLogDebug(_T("Failed to create the tooltip '%s'"), m_text.c_str());
    }
    else
    {
        // check for multiline toopltip
        int index = m_text.Find(_T('\n'));

        if ( index != wxNOT_FOUND )
        {
#ifdef TTM_SETMAXTIPWIDTH
            if ( wxApp::GetComCtl32Version() >= 470 )
            {
                // use TTM_SETMAXTIPWIDTH to make tooltip multiline using the
                // extent of its first line as max value
                HFONT hfont = (HFONT)
                    SendTooltipMessage(GetToolTipCtrl(), WM_GETFONT, 0);

                if ( !hfont )
                {
                    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                    if ( !hfont )
                    {
                        wxLogLastError(wxT("GetStockObject(DEFAULT_GUI_FONT)"));
                    }
                }

                MemoryHDC hdc;
                if ( !hdc )
                {
                    wxLogLastError(wxT("CreateCompatibleDC(NULL)"));
                }

                if ( !SelectObject(hdc, hfont) )
                {
                    wxLogLastError(wxT("SelectObject(hfont)"));
                }

                // find the width of the widest line
                int max = 0;
                wxStringTokenizer tokenizer(m_text, _T("\n"));
                wxString token = tokenizer.GetNextToken();
                while (token.length())
                {
                    SIZE sz;
                    if ( !::GetTextExtentPoint32(hdc, token, token.length(), &sz) )
                    {
                        wxLogLastError(wxT("GetTextExtentPoint32"));
                    }
                    if ( sz.cx > max )
                        max = sz.cx;

                    token = tokenizer.GetNextToken();
                }

                // only set a new width if it is bigger than the current setting
                if (max > SendTooltipMessage(GetToolTipCtrl(), TTM_GETMAXTIPWIDTH, 0))
                    SendTooltipMessage(GetToolTipCtrl(), TTM_SETMAXTIPWIDTH,
                                       (void *)max);
            }
            else
#endif // comctl32.dll >= 4.70
            {
                // replace the '\n's with spaces because otherwise they appear as
                // unprintable characters in the tooltip string
                m_text.Replace(_T("\n"), _T(" "));
                ti.lpszText = (wxChar *)m_text.c_str(); // const_cast

                if ( !SendTooltipMessage(GetToolTipCtrl(), TTM_ADDTOOL, &ti) )
                {
                    wxLogDebug(_T("Failed to create the tooltip '%s'"), m_text.c_str());
                }
            }
        }
    }
}

void wxToolTip::SetWindow(wxWindow *win)
{
    Remove();

    m_window = win;

    // add the window itself
    if ( m_window )
    {
        Add(m_window->GetHWND());
    }
#if !defined(__WXUNIVERSAL__)
    // and all of its subcontrols (e.g. radiobuttons in a radiobox) as well
    wxControl *control = wxDynamicCast(m_window, wxControl);
    if ( control )
    {
        const wxArrayLong& subcontrols = control->GetSubcontrols();
        size_t count = subcontrols.GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            int id = subcontrols[n];
            HWND hwnd = GetDlgItem(GetHwndOf(m_window), id);
            if ( !hwnd )
            {
                // may be it's a child of parent of the control, in fact?
                // (radiobuttons are subcontrols, i.e. children of the radiobox
                // for wxWidgets but are its siblings at Windows level)
                hwnd = GetDlgItem(GetHwndOf(m_window->GetParent()), id);
            }

            // must have it by now!
            wxASSERT_MSG( hwnd, _T("no hwnd for subcontrol?") );

            Add((WXHWND)hwnd);
        }
    }

    // VZ: it's ugly to do it here, but I don't want any major changes right
    //     now, later we will probably want to have wxWindow::OnGotToolTip() or
    //     something like this where the derived class can do such things
    //     itself instead of wxToolTip "knowing" about them all
    wxComboBox *combo = wxDynamicCast(control, wxComboBox);
    if ( combo )
    {
        WXHWND hwndComboEdit = combo->GetWindowStyle() & wxCB_READONLY
                                ? combo->GetHWND()
                                : combo->GetEditHWND();
        if ( hwndComboEdit )
        {
            Add(hwndComboEdit);
        }
    }
#endif // !defined(__WXUNIVERSAL__)
}

void wxToolTip::SetTip(const wxString& tip)
{
    m_text = tip;

    if ( m_window )
    {
        // update the tip text shown by the control
        wxToolInfo ti(GetHwndOf(m_window));
        ti.lpszText = (wxChar *)wxT("");
        (void)SendTooltipMessage(GetToolTipCtrl(), TTM_UPDATETIPTEXT, &ti);

        ti.lpszText = (wxChar *)m_text.c_str();
        (void)SendTooltipMessage(GetToolTipCtrl(), TTM_UPDATETIPTEXT, &ti);
    }
}

#endif // wxUSE_TOOLTIPS
