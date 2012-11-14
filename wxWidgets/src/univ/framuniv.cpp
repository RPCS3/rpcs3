///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/frame.cpp
// Purpose:     wxFrame class for wxUniversal
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.05.01
// RCS-ID:      $Id: framuniv.cpp 42664 2006-10-29 20:39:31Z VZ $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/frame.h"

#ifndef WX_PRECOMP
    #include "wx/menu.h"
    #include "wx/statusbr.h"
    #include "wx/settings.h"
    #include "wx/toolbar.h"
#endif // WX_PRECOMP

// ============================================================================
// implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxFrame, wxFrameBase)
    EVT_SIZE(wxFrame::OnSize)
    EVT_SYS_COLOUR_CHANGED(wxFrame::OnSysColourChanged)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxFrame, wxTopLevelWindow)

// ----------------------------------------------------------------------------
// ctors
// ----------------------------------------------------------------------------

bool wxFrame::Create(wxWindow *parent,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
{
    if ( !wxTopLevelWindow::Create(parent, id, title, pos, size, style, name) )
        return false;

    SetOwnBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));

    return true;
}

// Responds to colour changes, and passes event on to children.
void wxFrame::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    SetOwnBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
    Refresh();

    event.Skip();
}

// ----------------------------------------------------------------------------
// menu support
// ----------------------------------------------------------------------------

void wxFrame::OnSize(wxSizeEvent& event)
{
#if wxUSE_MENUS
    PositionMenuBar();
#endif // wxUSE_MENUS
#if wxUSE_STATUSBAR
    PositionStatusBar();
#endif // wxUSE_STATUSBAR
#if wxUSE_TOOLBAR
    PositionToolBar();
#endif // wxUSE_TOOLBAR

    event.Skip();
}

void wxFrame::SendSizeEvent()
{
    wxSizeEvent event(GetSize(), GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

#if wxUSE_MENUS

void wxFrame::PositionMenuBar()
{
    if ( m_frameMenuBar )
    {
        // the menubar is positioned above the client size, hence the negative
        // y coord
        wxCoord heightMbar = m_frameMenuBar->GetSize().y;

        wxCoord heightTbar = 0;

#if wxUSE_TOOLBAR
        if ( m_frameToolBar )
            heightTbar = m_frameToolBar->GetSize().y;
#endif // wxUSE_TOOLBAR

        m_frameMenuBar->SetSize(0,
#ifdef __WXPM__   // FIXME -- remove this, make wxOS2/Univ behave as
                 //          the rest of the world!!!
                                GetClientSize().y - heightMbar - heightTbar,
#else
                                - (heightMbar + heightTbar),
#endif
                                GetClientSize().x, heightMbar);
    }
}

void wxFrame::DetachMenuBar()
{
    wxFrameBase::DetachMenuBar();
    SendSizeEvent();
}

void wxFrame::AttachMenuBar(wxMenuBar *menubar)
{
    wxFrameBase::AttachMenuBar(menubar);
    SendSizeEvent();
}

#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR

void wxFrame::PositionStatusBar()
{
    if ( m_frameStatusBar )
    {
        wxSize size = GetClientSize();
        m_frameStatusBar->SetSize(0, size.y, size.x, wxDefaultCoord);
    }
}

wxStatusBar* wxFrame::CreateStatusBar(int number, long style,
                                      wxWindowID id, const wxString& name)
{
    wxStatusBar *bar = wxFrameBase::CreateStatusBar(number, style, id, name);
    SendSizeEvent();
    return bar;
}

#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR

wxToolBar* wxFrame::CreateToolBar(long style, wxWindowID id, const wxString& name)
{
    if ( wxFrameBase::CreateToolBar(style, id, name) )
    {
        PositionToolBar();
    }

    return m_frameToolBar;
}

void wxFrame::PositionToolBar()
{
    if ( m_frameToolBar )
    {
        wxSize size = GetClientSize();
        int tw, th, tx, ty;

        tx = ty = 0;
        m_frameToolBar->GetSize(&tw, &th);
        if ( m_frameToolBar->GetWindowStyleFlag() & wxTB_VERTICAL )
        {
            tx = -tw;
            th = size.y;
        }
        else
        {
            ty = -th;
            tw = size.x;
        }

        m_frameToolBar->SetSize(tx, ty, tw, th);
    }
}
#endif // wxUSE_TOOLBAR

wxPoint wxFrame::GetClientAreaOrigin() const
{
    wxPoint pt = wxFrameBase::GetClientAreaOrigin();

#if wxUSE_MENUS && !defined(__WXPM__)
    if ( m_frameMenuBar )
    {
        pt.y += m_frameMenuBar->GetSize().y;
    }
#endif // wxUSE_MENUS

#if wxUSE_TOOLBAR
    if ( m_frameToolBar )
    {
        if ( m_frameToolBar->GetWindowStyleFlag() & wxTB_VERTICAL )
            pt.x += m_frameToolBar->GetSize().x;
        else
            pt.y += m_frameToolBar->GetSize().y;
    }
#endif // wxUSE_TOOLBAR

    return pt;
}

void wxFrame::DoGetClientSize(int *width, int *height) const
{
    wxFrameBase::DoGetClientSize(width, height);

#if wxUSE_MENUS
    if ( m_frameMenuBar && height )
    {
        (*height) -= m_frameMenuBar->GetSize().y;
    }
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    if ( m_frameStatusBar && height )
    {
        (*height) -= m_frameStatusBar->GetSize().y;
    }
#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR
    if ( m_frameToolBar )
    {
        if ( width && (m_frameToolBar->GetWindowStyleFlag() & wxTB_VERTICAL) )
            (*width) -= m_frameToolBar->GetSize().x;
        else if ( height )
            (*height) -= m_frameToolBar->GetSize().y;
    }
#endif // wxUSE_TOOLBAR
}

void wxFrame::DoSetClientSize(int width, int height)
{
#if wxUSE_MENUS
    if ( m_frameMenuBar )
    {
        height += m_frameMenuBar->GetSize().y;
    }
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    if ( m_frameStatusBar )
    {
        height += m_frameStatusBar->GetSize().y;
    }
#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR
    if ( m_frameToolBar )
    {
#if wxUSE_STATUSBAR
        height += m_frameStatusBar->GetSize().y;
#endif // wxUSE_STATUSBAR

        if ( m_frameToolBar->GetWindowStyleFlag() & wxTB_VERTICAL )
            width += m_frameToolBar->GetSize().x;
        else
            height += m_frameToolBar->GetSize().y;
    }
#endif // wxUSE_TOOLBAR

    wxFrameBase::DoSetClientSize(width, height);
}

wxSize wxFrame::GetMinSize() const
{
    wxSize size = wxFrameBase::GetMinSize();

#if wxUSE_MENUS
    if ( m_frameMenuBar )
    {
        const wxSize sizeMenu = m_frameMenuBar->GetBestSize();
        if ( sizeMenu.x > size.x )
            size.x = sizeMenu.x;
        size.y += sizeMenu.y;
    }
#endif // wxUSE_MENUS

#if wxUSE_TOOLBAR
    if ( m_frameToolBar )
    {
        size.y += m_frameToolBar->GetSize().y;
    }
#endif // wxUSE_TOOLBAR

#if wxUSE_STATUSBAR
    if ( m_frameStatusBar )
    {
        size.y += m_frameStatusBar->GetSize().y;
    }
#endif // wxUSE_STATUSBAR

    return size;
}

bool wxFrame::Enable(bool enable)
{
    if (!wxFrameBase::Enable(enable))
        return false;
#ifdef __WXMICROWIN__
    if (m_frameMenuBar)
        m_frameMenuBar->Enable(enable);
#endif
    return true;
}
