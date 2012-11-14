/////////////////////////////////////////////////////////////////////////
// File:        src/unix/taskbarx11.cpp
// Purpose:     wxTaskBarIcon class for common Unix desktops
// Author:      Vaclav Slavik
// Modified by:
// Created:     04/04/2003
// RCS-ID:      $Id: taskbarx11.cpp 53582 2008-05-12 23:54:34Z RD $
// Copyright:   (c) Vaclav Slavik, 2003
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////

// NB: This implementation does *not* work with every X11 window manager.
//     Currently only GNOME 1.2 and KDE 1,2,3 methods are implemented here.
//     Freedesktop.org's System Tray specification is implemented in
//     src/gtk/taskbar.cpp and used from here under wxGTK.
//
//     Thanks to Ian Campbell, author of XMMS Status Docklet, for publishing
//     KDE and GNOME 1.2 methods.


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef wxHAS_TASK_BAR_ICON

#include "wx/taskbar.h"

#ifndef  WX_PRECOMP
    #include "wx/log.h"
    #include "wx/frame.h"
    #include "wx/dcclient.h"
    #include "wx/statbmp.h"
    #include "wx/sizer.h"
    #include "wx/bitmap.h"
    #include "wx/image.h"
#endif

#ifdef __VMS
#pragma message disable nosimpint
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef __VMS
#pragma message enable nosimpint
#endif

// ----------------------------------------------------------------------------
// base class that implements toolkit-specific method:
// ----------------------------------------------------------------------------

#ifdef __WXGTK20__
    #include <gtk/gtk.h>
    #if GTK_CHECK_VERSION(2,1,0)
        #include "wx/gtk/taskbarpriv.h"
        #define TASKBAR_ICON_AREA_BASE_INCLUDED
    #endif
#endif

#ifndef TASKBAR_ICON_AREA_BASE_INCLUDED
    class WXDLLIMPEXP_ADV wxTaskBarIconAreaBase : public wxFrame
    {
    public:
        wxTaskBarIconAreaBase()
            : wxFrame(NULL, wxID_ANY, _T("systray icon"),
                      wxDefaultPosition, wxDefaultSize,
                      wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR |
                      wxSIMPLE_BORDER | wxFRAME_SHAPED) {}

        bool IsProtocolSupported() const { return false; }
    };
#endif


// ----------------------------------------------------------------------------
// toolkit dependent methods to set properties on helper window:
// ----------------------------------------------------------------------------

#if defined(__WXGTK__)
    #include <gdk/gdk.h>
    #include <gdk/gdkx.h>
    #include <gtk/gtk.h>
    #define GetDisplay()        GDK_DISPLAY()
    #define GetXWindow(wxwin)   GDK_WINDOW_XWINDOW((wxwin)->m_widget->window)
#elif defined(__WXX11__) || defined(__WXMOTIF__)
    #include "wx/x11/privx.h"
    #define GetDisplay()        ((Display*)wxGlobalDisplay())
    #define GetXWindow(wxwin)   ((Window)(wxwin)->GetHandle())
#else
    #error "You must define X11 accessors for this port!"
#endif


// ----------------------------------------------------------------------------
// wxTaskBarIconArea is the real window that shows the icon:
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxTaskBarIconArea : public wxTaskBarIconAreaBase
{
public:
    wxTaskBarIconArea(wxTaskBarIcon *icon, const wxBitmap &bmp);
    void SetTrayIcon(const wxBitmap& bmp);
    bool IsOk() { return true; }

protected:
    void SetLegacyWMProperties();

    void OnSizeChange(wxSizeEvent& event);
    void OnPaint(wxPaintEvent& evt);
    void OnMouseEvent(wxMouseEvent& event);
    void OnMenuEvent(wxCommandEvent& event);

    wxTaskBarIcon *m_icon;
    wxPoint        m_pos;
    wxBitmap       m_bmp;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxTaskBarIconArea, wxTaskBarIconAreaBase)
    EVT_SIZE(wxTaskBarIconArea::OnSizeChange)
    EVT_MOUSE_EVENTS(wxTaskBarIconArea::OnMouseEvent)
    EVT_MENU(wxID_ANY, wxTaskBarIconArea::OnMenuEvent)
    EVT_PAINT(wxTaskBarIconArea::OnPaint)
END_EVENT_TABLE()

wxTaskBarIconArea::wxTaskBarIconArea(wxTaskBarIcon *icon, const wxBitmap &bmp)
    : wxTaskBarIconAreaBase(), m_icon(icon), m_pos(0,0)
{
#if defined(__WXGTK20__) && defined(TASKBAR_ICON_AREA_BASE_INCLUDED)
    m_invokingWindow = icon;
#endif

    // Set initial size to bitmap size (tray manager may and often will
    // change it):
    SetClientSize(wxSize(bmp.GetWidth(), bmp.GetHeight()));

    SetTrayIcon(bmp);

    if (!IsProtocolSupported())
    {
        wxLogTrace(_T("systray"),
                   _T("using legacy KDE1,2 and GNOME 1.2 methods"));
        SetLegacyWMProperties();
    }
}

void wxTaskBarIconArea::SetTrayIcon(const wxBitmap& bmp)
{
    m_bmp = bmp;

    // determine suitable bitmap size:
    wxSize winsize(GetClientSize());
    wxSize bmpsize(m_bmp.GetWidth(), m_bmp.GetHeight());
    wxSize iconsize(wxMin(winsize.x, bmpsize.x), wxMin(winsize.y, bmpsize.y));

    // rescale the bitmap to fit into the tray icon window:
    if (bmpsize != iconsize)
    {
        wxImage img = m_bmp.ConvertToImage();
        img.Rescale(iconsize.x, iconsize.y);
        m_bmp = wxBitmap(img);
    }

    wxRegion region;
    region.Union(m_bmp);

    // if the bitmap is smaller than the window, offset it:
    if (winsize != iconsize)
    {
        m_pos.x = (winsize.x - iconsize.x) / 2;
        m_pos.y = (winsize.y - iconsize.y) / 2;
        region.Offset(m_pos.x, m_pos.y);
    }

    // set frame's shape to correct value and redraw:
    SetShape(region);
    Refresh();
}

void wxTaskBarIconArea::SetLegacyWMProperties()
{
#ifdef __WXGTK__
    gtk_widget_realize(m_widget);
#endif

    long data[1];

    // KDE 2 & KDE 3:
    Atom _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR =
        XInternAtom(GetDisplay(), "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
    data[0] = 0;
    XChangeProperty(GetDisplay(), GetXWindow(this),
                    _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
                    XA_WINDOW, 32,
                    PropModeReplace, (unsigned char*)data, 1);

    // GNOME 1.2 & KDE 1:
    Atom KWM_DOCKWINDOW =
        XInternAtom(GetDisplay(), "KWM_DOCKWINDOW", False);
    data[0] = 1;
    XChangeProperty(GetDisplay(), GetXWindow(this),
                    KWM_DOCKWINDOW,
                    KWM_DOCKWINDOW, 32,
                    PropModeReplace, (unsigned char*)data, 1);
}

void wxTaskBarIconArea::OnSizeChange(wxSizeEvent& WXUNUSED(event))
{
    wxLogTrace(_T("systray"), _T("icon size changed to %i x %i"),
               GetSize().x, GetSize().y);
    // rescale or reposition the icon as needed:
    wxBitmap bmp(m_bmp);
    SetTrayIcon(bmp);
}

void wxTaskBarIconArea::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, m_pos.x, m_pos.y, true);
}

void wxTaskBarIconArea::OnMouseEvent(wxMouseEvent& event)
{
    wxEventType type = 0;
    wxEventType mtype = event.GetEventType();

    if (mtype == wxEVT_LEFT_DOWN)
        type = wxEVT_TASKBAR_LEFT_DOWN;
    else if (mtype == wxEVT_LEFT_UP)
        type = wxEVT_TASKBAR_LEFT_UP;
    else if (mtype == wxEVT_LEFT_DCLICK)
        type = wxEVT_TASKBAR_LEFT_DCLICK;
    else if (mtype == wxEVT_RIGHT_DOWN)
        type = wxEVT_TASKBAR_RIGHT_DOWN;
    else if (mtype == wxEVT_RIGHT_UP)
        type = wxEVT_TASKBAR_RIGHT_UP;
    else if (mtype == wxEVT_RIGHT_DCLICK)
        type = wxEVT_TASKBAR_RIGHT_DCLICK;
    else if (mtype == wxEVT_MOTION)
        type = wxEVT_TASKBAR_MOVE;
    else
        return;

   wxTaskBarIconEvent e(type, m_icon);
   m_icon->ProcessEvent(e);
}

void wxTaskBarIconArea::OnMenuEvent(wxCommandEvent& event)
{
    m_icon->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// wxTaskBarIcon class:
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTaskBarIcon, wxEvtHandler)

wxTaskBarIcon::wxTaskBarIcon() : m_iconWnd(NULL)
{
}

wxTaskBarIcon::~wxTaskBarIcon()
{
    if (m_iconWnd)
    {
        m_iconWnd->Disconnect(wxEVT_DESTROY, (wxObjectEventFunction)NULL,
                              (wxObject*)NULL, this);
        RemoveIcon();
    }
}

bool wxTaskBarIcon::IsOk() const
{
    return true;
}

bool wxTaskBarIcon::IsIconInstalled() const
{
    return m_iconWnd != NULL;
}

// Destroy event from wxTaskBarIconArea
void wxTaskBarIcon::OnDestroy(wxWindowDestroyEvent&)
{
    // prevent crash if wxTaskBarIconArea is destroyed by something else,
    // for example if panel/kicker is killed
    m_iconWnd = NULL;
}

bool wxTaskBarIcon::SetIcon(const wxIcon& icon, const wxString& tooltip)
{
    wxBitmap bmp;
    bmp.CopyFromIcon(icon);

    if (!m_iconWnd)
    {
        m_iconWnd = new wxTaskBarIconArea(this, bmp);
        if (m_iconWnd->IsOk())
        {
            m_iconWnd->Connect(wxEVT_DESTROY,
                wxWindowDestroyEventHandler(wxTaskBarIcon::OnDestroy),
                NULL, this);
            m_iconWnd->Show();
        }
        else
        {
            m_iconWnd->Destroy();
            m_iconWnd = NULL;
            return false;
        }
    }
    else
    {
        m_iconWnd->SetTrayIcon(bmp);
    }

#if wxUSE_TOOLTIPS
    if (!tooltip.empty())
        m_iconWnd->SetToolTip(tooltip);
    else
        m_iconWnd->SetToolTip(NULL);
#else
    wxUnusedVar(tooltip);
#endif
    return true;
}

bool wxTaskBarIcon::RemoveIcon()
{
    if (!m_iconWnd)
        return false;
    m_iconWnd->Destroy();
    m_iconWnd = NULL;
    return true;
}

bool wxTaskBarIcon::PopupMenu(wxMenu *menu)
{
    if (!m_iconWnd)
        return false;
    m_iconWnd->PopupMenu(menu);
    return true;
}

#endif // wxHAS_TASK_BAR_ICON
