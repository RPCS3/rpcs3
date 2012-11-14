/////////////////////////////////////////////////////////////////////////
// File:        src/msw/taskbar.cpp
// Purpose:     Implements wxTaskBarIcon class for manipulating icons on
//              the Windows task bar.
// Author:      Julian Smart
// Modified by: Vaclav Slavik
// Created:     24/3/98
// RCS-ID:      $Id: taskbar.cpp 50294 2007-11-28 01:59:59Z VZ $
// Copyright:   (c)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/frame.h"
    #include "wx/utils.h"
    #include "wx/menu.h"
#endif

#include "wx/msw/private.h"
#include "wx/msw/winundef.h"

#include <string.h>
#include "wx/taskbar.h"

#ifdef __WXWINCE__
    #include <winreg.h>
    #include <shellapi.h>
#endif

// initialized on demand
UINT   gs_msgTaskbar = 0;
UINT   gs_msgRestartTaskbar = 0;

#if WXWIN_COMPATIBILITY_2_4
BEGIN_EVENT_TABLE(wxTaskBarIcon, wxTaskBarIconBase)
    EVT_TASKBAR_MOVE         (wxTaskBarIcon::_OnMouseMove)
    EVT_TASKBAR_LEFT_DOWN    (wxTaskBarIcon::_OnLButtonDown)
    EVT_TASKBAR_LEFT_UP      (wxTaskBarIcon::_OnLButtonUp)
    EVT_TASKBAR_RIGHT_DOWN   (wxTaskBarIcon::_OnRButtonDown)
    EVT_TASKBAR_RIGHT_UP     (wxTaskBarIcon::_OnRButtonUp)
    EVT_TASKBAR_LEFT_DCLICK  (wxTaskBarIcon::_OnLButtonDClick)
    EVT_TASKBAR_RIGHT_DCLICK (wxTaskBarIcon::_OnRButtonDClick)
END_EVENT_TABLE()
#endif


IMPLEMENT_DYNAMIC_CLASS(wxTaskBarIcon, wxEvtHandler)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxTaskBarIconWindow: helper window
// ----------------------------------------------------------------------------

// NB: this class serves two purposes:
//     1. win32 needs a HWND associated with taskbar icon, this provides it
//     2. we need wxTopLevelWindow so that the app doesn't exit when
//        last frame is closed but there still is a taskbar icon
class wxTaskBarIconWindow : public wxFrame
{
public:
    wxTaskBarIconWindow(wxTaskBarIcon *icon)
        : wxFrame(NULL, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0),
          m_icon(icon)
    {
    }

    WXLRESULT MSWWindowProc(WXUINT msg,
                            WXWPARAM wParam, WXLPARAM lParam)
    {
        if (msg == gs_msgRestartTaskbar || msg == gs_msgTaskbar)
        {
            return m_icon->WindowProc(msg, wParam, lParam);
        }
        else
        {
            return wxFrame::MSWWindowProc(msg, wParam, lParam);
        }
    }

private:
    wxTaskBarIcon *m_icon;
};


// ----------------------------------------------------------------------------
// NotifyIconData: wrapper around NOTIFYICONDATA
// ----------------------------------------------------------------------------

struct NotifyIconData : public NOTIFYICONDATA
{
    NotifyIconData(WXHWND hwnd)
    {
        memset(this, 0, sizeof(NOTIFYICONDATA));
        cbSize = sizeof(NOTIFYICONDATA);
        hWnd = (HWND) hwnd;
        uCallbackMessage = gs_msgTaskbar;
        uFlags = NIF_MESSAGE;

        // we use the same id for all taskbar icons as we don't need it to
        // distinguish between them
        uID = 99;
    }
};

// ----------------------------------------------------------------------------
// wxTaskBarIcon
// ----------------------------------------------------------------------------

wxTaskBarIcon::wxTaskBarIcon()
{
    m_win = NULL;
    m_iconAdded = false;
    RegisterWindowMessages();
}

wxTaskBarIcon::~wxTaskBarIcon()
{
    if (m_iconAdded)
        RemoveIcon();

    if (m_win)
        m_win->Destroy();
}

// Operations
bool wxTaskBarIcon::SetIcon(const wxIcon& icon, const wxString& tooltip)
{
    // NB: we have to create the window lazily because of backward compatibility,
    //     old applications may create a wxTaskBarIcon instance before wxApp
    //     is initialized (as samples/taskbar used to do)
    if (!m_win)
    {
        m_win = new wxTaskBarIconWindow(this);
    }

    m_icon = icon;
    m_strTooltip = tooltip;

    NotifyIconData notifyData(GetHwndOf(m_win));

    if (icon.Ok())
    {
        notifyData.uFlags |= NIF_ICON;
        notifyData.hIcon = GetHiconOf(icon);
    }

    // set NIF_TIP even for an empty tooltip: otherwise it would be impossible
    // to remove an existing tooltip using this function
    notifyData.uFlags |= NIF_TIP;
    if ( !tooltip.empty() )
    {
        wxStrncpy(notifyData.szTip, tooltip.c_str(), WXSIZEOF(notifyData.szTip));
    }

    bool ok = Shell_NotifyIcon(m_iconAdded ? NIM_MODIFY
                                           : NIM_ADD, &notifyData) != 0;

    if ( !m_iconAdded && ok )
        m_iconAdded = true;

    return ok;
}

bool wxTaskBarIcon::RemoveIcon()
{
    if (!m_iconAdded)
        return false;

    m_iconAdded = false;

    NotifyIconData notifyData(GetHwndOf(m_win));

    return Shell_NotifyIcon(NIM_DELETE, &notifyData) != 0;
}

bool wxTaskBarIcon::PopupMenu(wxMenu *menu)
{
    wxASSERT_MSG( m_win != NULL, _T("taskbar icon not initialized") );

    static bool s_inPopup = false;

    if (s_inPopup)
        return false;

    s_inPopup = true;

    int         x, y;
    wxGetMousePosition(&x, &y);

    m_win->Move(x, y);

    m_win->PushEventHandler(this);

    menu->UpdateUI();

    // the SetForegroundWindow() and PostMessage() calls are needed to work
    // around Win32 bug with the popup menus shown for the notifications as
    // documented at http://support.microsoft.com/kb/q135788/
    ::SetForegroundWindow(GetHwndOf(m_win));

    bool rval = m_win->PopupMenu(menu, 0, 0);

    ::PostMessage(GetHwndOf(m_win), WM_NULL, 0, 0L);

    m_win->PopEventHandler(false);

    s_inPopup = false;

    return rval;
}

#if WXWIN_COMPATIBILITY_2_4
// Overridables
void wxTaskBarIcon::OnMouseMove(wxEvent& e)         { e.Skip(); }
void wxTaskBarIcon::OnLButtonDown(wxEvent& e)       { e.Skip(); }
void wxTaskBarIcon::OnLButtonUp(wxEvent& e)         { e.Skip(); }
void wxTaskBarIcon::OnRButtonDown(wxEvent& e)       { e.Skip(); }
void wxTaskBarIcon::OnRButtonUp(wxEvent& e)         { e.Skip(); }
void wxTaskBarIcon::OnLButtonDClick(wxEvent& e)     { e.Skip(); }
void wxTaskBarIcon::OnRButtonDClick(wxEvent& e)     { e.Skip(); }

void wxTaskBarIcon::_OnMouseMove(wxTaskBarIconEvent& e)
    { OnMouseMove(e);     }
void wxTaskBarIcon::_OnLButtonDown(wxTaskBarIconEvent& e)
    { OnLButtonDown(e);   }
void wxTaskBarIcon::_OnLButtonUp(wxTaskBarIconEvent& e)
    { OnLButtonUp(e);     }
void wxTaskBarIcon::_OnRButtonDown(wxTaskBarIconEvent& e)
    { OnRButtonDown(e);   }
void wxTaskBarIcon::_OnRButtonUp(wxTaskBarIconEvent& e)
    { OnRButtonUp(e);     }
void wxTaskBarIcon::_OnLButtonDClick(wxTaskBarIconEvent& e)
    { OnLButtonDClick(e); }
void wxTaskBarIcon::_OnRButtonDClick(wxTaskBarIconEvent& e)
    { OnRButtonDClick(e); }
#endif

void wxTaskBarIcon::RegisterWindowMessages()
{
    static bool s_registered = false;

    if ( !s_registered )
    {
        // Taskbar restart msg will be sent to us if the icon needs to be redrawn
        gs_msgRestartTaskbar = RegisterWindowMessage(wxT("TaskbarCreated"));

        // Also register the taskbar message here
        gs_msgTaskbar = ::RegisterWindowMessage(wxT("wxTaskBarIconMessage"));

        s_registered = true;
    }
}

// ----------------------------------------------------------------------------
// wxTaskBarIcon window proc
// ----------------------------------------------------------------------------

long wxTaskBarIcon::WindowProc(unsigned int msg,
                               unsigned int WXUNUSED(wParam),
                               long lParam)
{
    wxEventType eventType = 0;

    if (msg == gs_msgRestartTaskbar)   // does the icon need to be redrawn?
    {
        m_iconAdded = false;
        SetIcon(m_icon, m_strTooltip);
        return 0;
    }

    // this function should only be called for gs_msg(Restart)Taskbar messages
    wxASSERT(msg == gs_msgTaskbar);

    switch (lParam)
    {
        case WM_LBUTTONDOWN:
            eventType = wxEVT_TASKBAR_LEFT_DOWN;
            break;

        case WM_LBUTTONUP:
            eventType = wxEVT_TASKBAR_LEFT_UP;
            break;

        case WM_RBUTTONDOWN:
            eventType = wxEVT_TASKBAR_RIGHT_DOWN;
            break;

        case WM_RBUTTONUP:
            eventType = wxEVT_TASKBAR_RIGHT_UP;
            break;

        case WM_LBUTTONDBLCLK:
            eventType = wxEVT_TASKBAR_LEFT_DCLICK;
            break;

        case WM_RBUTTONDBLCLK:
            eventType = wxEVT_TASKBAR_RIGHT_DCLICK;
            break;

        case WM_MOUSEMOVE:
            eventType = wxEVT_TASKBAR_MOVE;
            break;

        default:
            break;
    }

    if (eventType)
    {
        wxTaskBarIconEvent event(eventType, this);

        ProcessEvent(event);
    }

    return 0;
}
