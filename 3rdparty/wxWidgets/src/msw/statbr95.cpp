///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/statbr95.cpp
// Purpose:     native implementation of wxStatusBar
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.04.98
// RCS-ID:      $Id: statbr95.cpp 48670 2007-09-14 07:15:51Z JS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_STATUSBAR && wxUSE_NATIVE_STATUSBAR

#include "wx/statusbr.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/frame.h"
    #include "wx/settings.h"
    #include "wx/dcclient.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#include "wx/msw/private.h"
#include <windowsx.h>

#if wxUSE_UXTHEME
    #include "wx/msw/uxtheme.h"
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// windowsx.h and commctrl.h don't define those, so we do it here
#define StatusBar_SetParts(h, n, w) SendMessage(h, SB_SETPARTS, (WPARAM)n, (LPARAM)w)
#define StatusBar_SetText(h, n, t)  SendMessage(h, SB_SETTEXT, (WPARAM)n, (LPARAM)(LPCTSTR)t)
#define StatusBar_GetTextLen(h, n)  LOWORD(SendMessage(h, SB_GETTEXTLENGTH, (WPARAM)n, 0))
#define StatusBar_GetText(h, n, s)  LOWORD(SendMessage(h, SB_GETTEXT, (WPARAM)n, (LPARAM)(LPTSTR)s))

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxStatusBar95 class
// ----------------------------------------------------------------------------

wxStatusBar95::wxStatusBar95()
{
    SetParent(NULL);
    m_hWnd = 0;
    m_windowId = 0;
}

bool wxStatusBar95::Create(wxWindow *parent,
                           wxWindowID id,
                           long style,
                           const wxString& name)
{
    wxCHECK_MSG( parent, false, wxT("status bar must have a parent") );

    // Avoid giving the status bar a themed window
    style = (style & ~wxBORDER_MASK) | wxBORDER_NONE;

    SetName(name);
    SetWindowStyleFlag(style);
    SetParent(parent);

    parent->AddChild(this);

    m_windowId = id == wxID_ANY ? NewControlId() : id;

    DWORD wstyle = WS_CHILD | WS_VISIBLE;

    if ( style & wxCLIP_SIBLINGS )
        wstyle |= WS_CLIPSIBLINGS;

    // setting SBARS_SIZEGRIP is perfectly useless: it's always on by default
    // (at least in the version of comctl32.dll I'm using), and the only way to
    // turn it off is to use CCS_TOP style - as we position the status bar
    // manually anyhow (see DoMoveWindow), use CCS_TOP style if wxST_SIZEGRIP
    // is not given
    if ( !(style & wxST_SIZEGRIP) )
    {
        wstyle |= CCS_TOP;
    }
    else
    {
#ifndef __WXWINCE__
        // may be some versions of comctl32.dll do need it - anyhow, it won't
        // do any harm
        wstyle |= SBARS_SIZEGRIP;
#endif
    }

    m_hWnd = (WXHWND)CreateStatusWindow(wstyle,
                                        wxEmptyString,
                                        GetHwndOf(parent),
                                        m_windowId);
    if ( m_hWnd == 0 )
    {
        wxLogSysError(_("Failed to create a status bar."));

        return false;
    }

    SetFieldsCount(1);
    SubclassWin(m_hWnd);
    InheritAttributes();

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));

    // we must refresh the frame size when the statusbar is created, because
    // its client area might change
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if ( frame )
    {
        frame->SendSizeEvent();
    }

    return true;
}

wxStatusBar95::~wxStatusBar95()
{
    // we must refresh the frame size when the statusbar is deleted but the
    // frame is not - otherwise statusbar leaves a hole in the place it used to
    // occupy
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if ( frame && !frame->IsBeingDeleted() )
    {
        frame->SendSizeEvent();
    }
}

void wxStatusBar95::SetFieldsCount(int nFields, const int *widths)
{
    // this is a Windows limitation
    wxASSERT_MSG( (nFields > 0) && (nFields < 255), _T("too many fields") );

    wxStatusBarBase::SetFieldsCount(nFields, widths);

    SetFieldsWidth();
}

void wxStatusBar95::SetStatusWidths(int n, const int widths[])
{
    wxStatusBarBase::SetStatusWidths(n, widths);

    SetFieldsWidth();
}

void wxStatusBar95::SetFieldsWidth()
{
    if ( !m_nFields )
        return;

    int aBorders[3];
    SendMessage(GetHwnd(), SB_GETBORDERS, 0, (LPARAM)aBorders);

    int extraWidth = aBorders[2]; // space between fields

    wxArrayInt widthsAbs =
        CalculateAbsWidths(GetClientSize().x - extraWidth*(m_nFields - 1));

    int *pWidths = new int[m_nFields];

    int nCurPos = 0;
    for ( int i = 0; i < m_nFields; i++ ) {
        nCurPos += widthsAbs[i] + extraWidth;
        pWidths[i] = nCurPos;
    }

    if ( !StatusBar_SetParts(GetHwnd(), m_nFields, pWidths) ) {
        wxLogLastError(wxT("StatusBar_SetParts"));
    }

    delete [] pWidths;
}

void wxStatusBar95::SetStatusText(const wxString& strText, int nField)
{
    wxCHECK_RET( (nField >= 0) && (nField < m_nFields),
                 _T("invalid statusbar field index") );

    if ( strText == GetStatusText(nField) )
    {
       // don't call StatusBar_SetText() to avoid flicker
       return;
    }

    // Get field style, if any
    int style;
    if (m_statusStyles)
    {
        switch(m_statusStyles[nField])
        {
        case wxSB_RAISED:
            style = SBT_POPOUT;
            break;
        case wxSB_FLAT:
            style = SBT_NOBORDERS;
            break;
        case wxSB_NORMAL:
        default:
            style = 0;
            break;
        }
    }
    else
        style = 0;

    // Pass both field number and style. MSDN library doesn't mention
    // that nField and style have to be 'ORed'
    if ( !StatusBar_SetText(GetHwnd(), nField | style, strText) )
    {
        wxLogLastError(wxT("StatusBar_SetText"));
    }
}

wxString wxStatusBar95::GetStatusText(int nField) const
{
    wxCHECK_MSG( (nField >= 0) && (nField < m_nFields), wxEmptyString,
                 _T("invalid statusbar field index") );

    wxString str;
    int len = StatusBar_GetTextLen(GetHwnd(), nField);
    if ( len > 0 )
    {
        StatusBar_GetText(GetHwnd(), nField, wxStringBuffer(str, len));
    }

    return str;
}

int wxStatusBar95::GetBorderX() const
{
    int aBorders[3];
    SendMessage(GetHwnd(), SB_GETBORDERS, 0, (LPARAM)aBorders);

    return aBorders[0];
}

int wxStatusBar95::GetBorderY() const
{
    int aBorders[3];
    SendMessage(GetHwnd(), SB_GETBORDERS, 0, (LPARAM)aBorders);

    return aBorders[1];
}

void wxStatusBar95::SetMinHeight(int height)
{
    SendMessage(GetHwnd(), SB_SETMINHEIGHT, height + 2*GetBorderY(), 0);

    // have to send a (dummy) WM_SIZE to redraw it now
    SendMessage(GetHwnd(), WM_SIZE, 0, 0);
}

bool wxStatusBar95::GetFieldRect(int i, wxRect& rect) const
{
    wxCHECK_MSG( (i >= 0) && (i < m_nFields), false,
                 _T("invalid statusbar field index") );

    RECT r;
    if ( !::SendMessage(GetHwnd(), SB_GETRECT, i, (LPARAM)&r) )
    {
        wxLogLastError(wxT("SendMessage(SB_GETRECT)"));
    }

#if wxUSE_UXTHEME
    wxUxThemeHandle theme((wxStatusBar95 *)this, L"Status"); // const_cast
    if ( theme )
    {
        // by default Windows has a 2 pixel border to the right of the left
        // divider (or it could be a bug) but it looks wrong so remove it
        if ( i != 0 )
        {
            r.left -= 2;
        }

        wxUxThemeEngine::Get()->GetThemeBackgroundContentRect(theme, NULL,
                                                              1 /* SP_PANE */, 0,
                                                              &r, &r);
    }
#endif

    wxCopyRECTToRect(r, rect);

    return true;
}

void wxStatusBar95::DoMoveWindow(int x, int y, int width, int height)
{
    if ( GetParent()->IsSizeDeferred() )
    {
        wxWindowMSW::DoMoveWindow(x, y, width, height);
    }
    else
    {
        // parent pos/size isn't deferred so do it now but don't send
        // WM_WINDOWPOSCHANGING since we don't want to change pos/size later
        // we must use SWP_NOCOPYBITS here otherwise it paints incorrectly
        // if other windows are size deferred
        ::SetWindowPos(GetHwnd(), NULL, x, y, width, height,
                       SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE
#ifndef __WXWINCE__
                       | SWP_NOCOPYBITS | SWP_NOSENDCHANGING
#endif
                       );
    }

    // adjust fields widths to the new size
    SetFieldsWidth();

    // we have to trigger wxSizeEvent if there are children window in status
    // bar because GetFieldRect returned incorrect (not updated) values up to
    // here, which almost certainly resulted in incorrectly redrawn statusbar
    if ( m_children.GetCount() > 0 )
    {
        wxSizeEvent event(GetClientSize(), m_windowId);
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(event);
    }
}

void wxStatusBar95::SetStatusStyles(int n, const int styles[])
{
    wxStatusBarBase::SetStatusStyles(n, styles);

    if (n != m_nFields)
        return;

    for (int i = 0; i < n; i++)
    {
        int style;
        switch(styles[i])
        {
        case wxSB_RAISED:
            style = SBT_POPOUT;
            break;
        case wxSB_FLAT:
            style = SBT_NOBORDERS;
            break;
        case wxSB_NORMAL:
        default:
            style = 0;
            break;
        }
        // The SB_SETTEXT message is both used to set the field's text as well as
        // the fields' styles. MSDN library doesn't mention
        // that nField and style have to be 'ORed'
        wxString text = GetStatusText(i);
        if (!StatusBar_SetText(GetHwnd(), style | i, text))
        {
            wxLogLastError(wxT("StatusBar_SetText"));
        }
    }
}

WXLRESULT
wxStatusBar95::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
#ifndef __WXWINCE__
    if ( nMsg == WM_WINDOWPOSCHANGING )
    {
        WINDOWPOS *lpPos = (WINDOWPOS *)lParam;
        int x, y, w, h;
        GetPosition(&x, &y);
        GetSize(&w, &h);

        // we need real window coords and not wx client coords
        AdjustForParentClientOrigin(x, y);

        lpPos->x  = x;
        lpPos->y  = y;
        lpPos->cx = w;
        lpPos->cy = h;

        return 0;
    }

    if ( nMsg == WM_NCLBUTTONDOWN )
    {
        // if hit-test is on gripper then send message to TLW to begin
        // resizing. It is possible to send this message to any window.
        if ( wParam == HTBOTTOMRIGHT )
        {
            wxWindow *win;

            for ( win = GetParent(); win; win = win->GetParent() )
            {
                if ( win->IsTopLevel() )
                {
                    SendMessage(GetHwndOf(win), WM_NCLBUTTONDOWN,
                                wParam, lParam);

                    return 0;
                }
            }
        }
    }
#endif

    return wxStatusBarBase::MSWWindowProc(nMsg, wParam, lParam);
}

#endif // wxUSE_STATUSBAR && wxUSE_NATIVE_STATUSBAR
