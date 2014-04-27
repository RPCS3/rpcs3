/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/mdig.cpp
// Purpose:     Generic MDI (Multiple Document Interface) classes
// Author:      Hans Van Leemputten
// Modified by:
// Created:     29/07/2002
// RCS-ID:      $Id: mdig.cpp 41069 2006-09-08 14:38:49Z VS $
// Copyright:   (c) Hans Van Leemputten
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_MDI

#include "wx/generic/mdig.h"

#ifndef WX_PRECOMP
    #include "wx/panel.h"
    #include "wx/menu.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif //WX_PRECOMP

#include "wx/stockitem.h"

enum MDI_MENU_ID
{
    wxWINDOWCLOSE = 4001,
    wxWINDOWCLOSEALL,
    wxWINDOWNEXT,
    wxWINDOWPREV
};

//-----------------------------------------------------------------------------
// wxGenericMDIParentFrame
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericMDIParentFrame, wxFrame)

BEGIN_EVENT_TABLE(wxGenericMDIParentFrame, wxFrame)
#if wxUSE_MENUS
    EVT_MENU (wxID_ANY, wxGenericMDIParentFrame::DoHandleMenu)
#endif
END_EVENT_TABLE()

wxGenericMDIParentFrame::wxGenericMDIParentFrame()
{
    Init();
}

wxGenericMDIParentFrame::wxGenericMDIParentFrame(wxWindow *parent,
                                   wxWindowID id,
                                   const wxString& title,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name)
{
    Init();

    (void)Create(parent, id, title, pos, size, style, name);
}

wxGenericMDIParentFrame::~wxGenericMDIParentFrame()
{
    // Make sure the client window is destructed before the menu bars are!
    wxDELETE(m_pClientWindow);

#if wxUSE_MENUS
    if (m_pMyMenuBar)
    {
        delete m_pMyMenuBar;
        m_pMyMenuBar = (wxMenuBar *) NULL;
    }

    RemoveWindowMenu(GetMenuBar());

    if (m_pWindowMenu)
    {
        delete m_pWindowMenu;
        m_pWindowMenu = (wxMenu*) NULL;
    }
#endif // wxUSE_MENUS
}

bool wxGenericMDIParentFrame::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxString& title,
                              const wxPoint& pos,
                              const wxSize& size,
                              long style,
                              const wxString& name)
{
  // this style can be used to prevent a window from having the standard MDI
  // "Window" menu
  if ( !(style & wxFRAME_NO_WINDOW_MENU) )
  {
#if wxUSE_MENUS
      m_pWindowMenu = new wxMenu;

      m_pWindowMenu->Append(wxWINDOWCLOSE,    _("Cl&ose"));
      m_pWindowMenu->Append(wxWINDOWCLOSEALL, _("Close All"));
      m_pWindowMenu->AppendSeparator();
      m_pWindowMenu->Append(wxWINDOWNEXT,     _("&Next"));
      m_pWindowMenu->Append(wxWINDOWPREV,     _("&Previous"));
#endif // wxUSE_MENUS
  }

  wxFrame::Create( parent, id, title, pos, size, style, name );

  OnCreateClient();

  return true;
}

#if wxUSE_MENUS
void wxGenericMDIParentFrame::SetWindowMenu(wxMenu* pMenu)
{
    // Replace the window menu from the currently loaded menu bar.
    wxMenuBar *pMenuBar = GetMenuBar();

    if (m_pWindowMenu)
    {
        RemoveWindowMenu(pMenuBar);

        wxDELETE(m_pWindowMenu);
    }

    if (pMenu)
    {
        m_pWindowMenu = pMenu;

        AddWindowMenu(pMenuBar);
    }
}

void wxGenericMDIParentFrame::SetMenuBar(wxMenuBar *pMenuBar)
{
    // Remove the Window menu from the old menu bar
    RemoveWindowMenu(GetMenuBar());
    // Add the Window menu to the new menu bar.
    AddWindowMenu(pMenuBar);

    wxFrame::SetMenuBar(pMenuBar);
}
#endif // wxUSE_MENUS

void wxGenericMDIParentFrame::SetChildMenuBar(wxGenericMDIChildFrame *pChild)
{
#if wxUSE_MENUS
    if (pChild  == (wxGenericMDIChildFrame *) NULL)
    {
        // No Child, set Our menu bar back.
        SetMenuBar(m_pMyMenuBar);

        // Make sure we know our menu bar is in use
        m_pMyMenuBar = (wxMenuBar*) NULL;
    }
    else
    {
        if (pChild->GetMenuBar() == (wxMenuBar*) NULL)
            return;

        // Do we need to save the current bar?
        if (m_pMyMenuBar == NULL)
            m_pMyMenuBar = GetMenuBar();

        SetMenuBar(pChild->GetMenuBar());
    }
#endif // wxUSE_MENUS
}

bool wxGenericMDIParentFrame::ProcessEvent(wxEvent& event)
{
    /*
     * Redirect events to active child first.
     */

    // Stops the same event being processed repeatedly
    static wxEventType inEvent = wxEVT_NULL;
    if (inEvent == event.GetEventType())
        return false;

    inEvent = event.GetEventType();

    // Let the active child (if any) process the event first.
    bool res = false;
    if (m_pActiveChild && event.IsKindOf(CLASSINFO(wxCommandEvent))
#if 0
        /* This is sure to not give problems... */
        && (event.GetEventType() == wxEVT_COMMAND_MENU_SELECTED ||
            event.GetEventType() == wxEVT_UPDATE_UI )
#else
        /* This was tested on wxMSW and worked... */
        && event.GetEventObject() != m_pClientWindow
        && !(event.GetEventType() == wxEVT_ACTIVATE ||
             event.GetEventType() == wxEVT_SET_FOCUS ||
             event.GetEventType() == wxEVT_KILL_FOCUS ||
             event.GetEventType() == wxEVT_CHILD_FOCUS ||
             event.GetEventType() == wxEVT_COMMAND_SET_FOCUS ||
             event.GetEventType() == wxEVT_COMMAND_KILL_FOCUS )
#endif
       )
    {
        res = m_pActiveChild->GetEventHandler()->ProcessEvent(event);
    }

    // If the event was not handled this frame will handle it!
    if (!res)
    {
        res = GetEventHandler()->wxEvtHandler::ProcessEvent(event);
    }

    inEvent = wxEVT_NULL;

    return res;
}

wxGenericMDIChildFrame *wxGenericMDIParentFrame::GetActiveChild() const
{
    return m_pActiveChild;
}

void wxGenericMDIParentFrame::SetActiveChild(wxGenericMDIChildFrame* pChildFrame)
{
    m_pActiveChild = pChildFrame;
}

wxGenericMDIClientWindow *wxGenericMDIParentFrame::GetClientWindow() const
{
    return m_pClientWindow;
}

wxGenericMDIClientWindow *wxGenericMDIParentFrame::OnCreateClient()
{
#if wxUSE_GENERIC_MDI_AS_NATIVE
    m_pClientWindow = new wxMDIClientWindow( this );
#else
    m_pClientWindow = new wxGenericMDIClientWindow( this );
#endif
    return m_pClientWindow;
}

void wxGenericMDIParentFrame::ActivateNext()
{
    if (m_pClientWindow && m_pClientWindow->GetSelection() != -1)
    {
        size_t active = m_pClientWindow->GetSelection() + 1;
        if (active >= m_pClientWindow->GetPageCount())
            active = 0;

        m_pClientWindow->SetSelection(active);
    }
}

void wxGenericMDIParentFrame::ActivatePrevious()
{
    if (m_pClientWindow && m_pClientWindow->GetSelection() != -1)
    {
        int active = m_pClientWindow->GetSelection() - 1;
        if (active < 0)
            active = m_pClientWindow->GetPageCount() - 1;

        m_pClientWindow->SetSelection(active);
    }
}

void wxGenericMDIParentFrame::Init()
{
    m_pClientWindow = (wxGenericMDIClientWindow *) NULL;
    m_pActiveChild = (wxGenericMDIChildFrame *) NULL;
#if wxUSE_MENUS
    m_pWindowMenu = (wxMenu *) NULL;
    m_pMyMenuBar = (wxMenuBar*) NULL;
#endif // wxUSE_MENUS
}

#if wxUSE_MENUS
void wxGenericMDIParentFrame::RemoveWindowMenu(wxMenuBar *pMenuBar)
{
    if (pMenuBar && m_pWindowMenu)
    {
        // Remove old window menu
        int pos = pMenuBar->FindMenu(_("&Window"));
        if (pos != wxNOT_FOUND)
        {
            wxASSERT(m_pWindowMenu == pMenuBar->GetMenu(pos)); // DBG:: We're going to delete the wrong menu!!!
            pMenuBar->Remove(pos);
        }
    }
}

void wxGenericMDIParentFrame::AddWindowMenu(wxMenuBar *pMenuBar)
{
    if (pMenuBar && m_pWindowMenu)
    {
        int pos = pMenuBar->FindMenu(wxGetStockLabel(wxID_HELP,false));
        if (pos == wxNOT_FOUND)
        {
            pMenuBar->Append(m_pWindowMenu, _("&Window"));
        }
        else
        {
            pMenuBar->Insert(pos, m_pWindowMenu, _("&Window"));
        }
    }
}

void wxGenericMDIParentFrame::DoHandleMenu(wxCommandEvent &event)
{
    switch (event.GetId())
    {
    case wxWINDOWCLOSE:
        if (m_pActiveChild)
        {
            m_pActiveChild->Close();
        }
        break;
    case wxWINDOWCLOSEALL:
        {
#if 0   // code is only needed if next #if is set to 0!
            wxGenericMDIChildFrame *pFirstActiveChild = m_pActiveChild;
#endif
            while (m_pActiveChild)
            {
                if (!m_pActiveChild->Close())
                {
                    return; // We failed...
                }
                else
                {
#if 1   // What's best? Delayed deleting or immediate deleting?
                    delete m_pActiveChild;
                    m_pActiveChild = NULL;
#else
                    ActivateNext();

                    if (pFirstActiveChild == m_pActiveChild)
                        return; // We've called Close on all items, no need to continue.
#endif
                }
            }
        }
        break;
    case wxWINDOWNEXT:
        ActivateNext();
        break;
    case wxWINDOWPREV:
        ActivatePrevious();
        break;
    default :
        event.Skip();
    }
}
#endif // wxUSE_MENUS

void wxGenericMDIParentFrame::DoGetClientSize(int *width, int *height) const
{
    wxFrame::DoGetClientSize( width, height );
}


//-----------------------------------------------------------------------------
// wxGenericMDIChildFrame
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericMDIChildFrame, wxPanel)

BEGIN_EVENT_TABLE(wxGenericMDIChildFrame, wxPanel)
    EVT_MENU_HIGHLIGHT_ALL(wxGenericMDIChildFrame::OnMenuHighlight)
    EVT_ACTIVATE(wxGenericMDIChildFrame::OnActivate)

    EVT_CLOSE(wxGenericMDIChildFrame::OnCloseWindow)
    EVT_SIZE(wxGenericMDIChildFrame::OnSize)
END_EVENT_TABLE()

wxGenericMDIChildFrame::wxGenericMDIChildFrame()
{
    Init();
}

wxGenericMDIChildFrame::wxGenericMDIChildFrame( wxGenericMDIParentFrame *parent,
      wxWindowID id, const wxString& title,
      const wxPoint& WXUNUSED(pos), const wxSize& size,
      long style, const wxString& name )
{
    Init();

    Create( parent, id, title, wxDefaultPosition, size, style, name );
}

wxGenericMDIChildFrame::~wxGenericMDIChildFrame()
{
    wxGenericMDIParentFrame *pParentFrame = GetMDIParentFrame();

    if (pParentFrame != NULL)
    {
        bool bActive = false;
        if (pParentFrame->GetActiveChild() == this)
        {
            pParentFrame->SetActiveChild((wxGenericMDIChildFrame*) NULL);
            pParentFrame->SetChildMenuBar((wxGenericMDIChildFrame*) NULL);
            bActive = true;
        }

        wxGenericMDIClientWindow *pClientWindow = pParentFrame->GetClientWindow();

        // Remove page if still there
        size_t pos;
        for (pos = 0; pos < pClientWindow->GetPageCount(); pos++)
        {
            if (pClientWindow->GetPage(pos) == this)
            {
                if (pClientWindow->RemovePage(pos))
                    pClientWindow->Refresh();
                break;
            }
        }

        if (bActive)
        {
            // Set the new selection to the a remaining page
            if (pClientWindow->GetPageCount() > pos)
            {
                pClientWindow->SetSelection(pos);
            }
            else
            {
                if ((int)pClientWindow->GetPageCount() - 1 >= 0)
                    pClientWindow->SetSelection(pClientWindow->GetPageCount() - 1);
            }
        }
    }

#if wxUSE_MENUS
    wxDELETE(m_pMenuBar);
#endif // wxUSE_MENUS
}

bool wxGenericMDIChildFrame::Create( wxGenericMDIParentFrame *parent,
      wxWindowID id, const wxString& title,
      const wxPoint& WXUNUSED(pos), const wxSize& size,
      long style, const wxString& name )
{
    wxGenericMDIClientWindow* pClientWindow = parent->GetClientWindow();

    wxASSERT_MSG((pClientWindow != (wxWindow*) NULL), wxT("Missing MDI client window.") );

    wxPanel::Create(pClientWindow, id, wxDefaultPosition, size, style, name);

    SetMDIParentFrame(parent);

    // This is the currently active child
    parent->SetActiveChild(this);

    m_Title = title;

    pClientWindow->AddPage(this, title, true);
    ApplyMDIChildFrameRect();   // Ok confirme the size change!
    pClientWindow->Refresh();

    return true;
}

#if wxUSE_MENUS
void wxGenericMDIChildFrame::SetMenuBar( wxMenuBar *menu_bar )
{
    wxMenuBar *pOldMenuBar = m_pMenuBar;
    m_pMenuBar = menu_bar;

    if (m_pMenuBar)
    {
        wxGenericMDIParentFrame *pParentFrame = GetMDIParentFrame();

        if (pParentFrame != NULL)
        {
            m_pMenuBar->SetParent(pParentFrame);

            if (pParentFrame->GetActiveChild() == this)
            {
                // Replace current menu bars
                if (pOldMenuBar)
                    pParentFrame->SetChildMenuBar((wxGenericMDIChildFrame*) NULL);
                pParentFrame->SetChildMenuBar((wxGenericMDIChildFrame*) this);
            }
        }
    }
}

wxMenuBar *wxGenericMDIChildFrame::GetMenuBar() const
{
    return m_pMenuBar;
}
#endif // wxUSE_MENUS

void wxGenericMDIChildFrame::SetTitle(const wxString& title)
{
    m_Title = title;

    wxGenericMDIParentFrame *pParentFrame = GetMDIParentFrame();

    if (pParentFrame != NULL)
    {
        wxGenericMDIClientWindow * pClientWindow = pParentFrame->GetClientWindow();

        if (pClientWindow != NULL)
        {
            size_t pos;
            for (pos = 0; pos < pClientWindow->GetPageCount(); pos++)
            {
                if (pClientWindow->GetPage(pos) == this)
                {
                    pClientWindow->SetPageText(pos, m_Title);
                    break;
                }
            }
        }
    }
}

wxString wxGenericMDIChildFrame::GetTitle() const
{
    return m_Title;
}

void wxGenericMDIChildFrame::Activate()
{
    wxGenericMDIParentFrame *pParentFrame = GetMDIParentFrame();

    if (pParentFrame != NULL)
    {
        wxGenericMDIClientWindow * pClientWindow = pParentFrame->GetClientWindow();

        if (pClientWindow != NULL)
        {
            size_t pos;
            for (pos = 0; pos < pClientWindow->GetPageCount(); pos++)
            {
                if (pClientWindow->GetPage(pos) == this)
                {
                    pClientWindow->SetSelection(pos);
                    break;
                }
            }
        }
    }
}

void wxGenericMDIChildFrame::OnMenuHighlight(wxMenuEvent& event)
{
#if wxUSE_STATUSBAR
    if ( m_pMDIParentFrame)
    {
        // we don't have any help text for this item,
        // but may be the MDI frame does?
        m_pMDIParentFrame->OnMenuHighlight(event);
    }
#else
    wxUnusedVar(event);
#endif // wxUSE_STATUSBAR
}

void wxGenericMDIChildFrame::OnActivate(wxActivateEvent& WXUNUSED(event))
{
    // Do mothing.
}

/*** Copied from top level..! ***/
// default resizing behaviour - if only ONE subwindow, resize to fill the
// whole client area
void wxGenericMDIChildFrame::OnSize(wxSizeEvent& WXUNUSED(event))
{
    // if we're using constraints or sizers - do use them
    if ( GetAutoLayout() )
    {
        Layout();
    }
    else
    {
        // do we have _exactly_ one child?
        wxWindow *child = (wxWindow *)NULL;
        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxWindow *win = node->GetData();

            // exclude top level and managed windows (status bar isn't
            // currently in the children list except under wxMac anyhow, but
            // it makes no harm to test for it)
            if ( !win->IsTopLevel() /*&& !IsOneOfBars(win)*/ )
            {
                if ( child )
                {
                    return;     // it's our second subwindow - nothing to do
                }

                child = win;
            }
        }

        // do we have any children at all?
        if ( child )
        {
            // exactly one child - set it's size to fill the whole frame
            int clientW, clientH;
            DoGetClientSize(&clientW, &clientH);

            // for whatever reasons, wxGTK wants to have a small offset - it
            // probably looks better with it?
#ifdef __WXGTK__
            static const int ofs = 1;
#else
            static const int ofs = 0;
#endif

            child->SetSize(ofs, ofs, clientW - 2*ofs, clientH - 2*ofs);
        }
    }
}

/*** Copied from top level..! ***/
// The default implementation for the close window event.
void wxGenericMDIChildFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    Destroy();
}

void wxGenericMDIChildFrame::SetMDIParentFrame(wxGenericMDIParentFrame* parentFrame)
{
    m_pMDIParentFrame = parentFrame;
}

wxGenericMDIParentFrame* wxGenericMDIChildFrame::GetMDIParentFrame() const
{
    return m_pMDIParentFrame;
}

void wxGenericMDIChildFrame::Init()
{
    m_pMDIParentFrame = (wxGenericMDIParentFrame *) NULL;
#if wxUSE_MENUS
    m_pMenuBar = (wxMenuBar *) NULL;
#endif // wxUSE_MENUS
}

void wxGenericMDIChildFrame::DoMoveWindow(int x, int y, int width, int height)
{
    m_MDIRect = wxRect(x, y, width, height);
}

void wxGenericMDIChildFrame::ApplyMDIChildFrameRect()
{
    wxPanel::DoMoveWindow(m_MDIRect.x, m_MDIRect.y, m_MDIRect.width, m_MDIRect.height);
}

//-----------------------------------------------------------------------------
// wxGenericMDIClientWindow
//-----------------------------------------------------------------------------

#define wxID_NOTEBOOK_CLIENT_AREA wxID_HIGHEST + 100

IMPLEMENT_DYNAMIC_CLASS(wxGenericMDIClientWindow, wxNotebook)

BEGIN_EVENT_TABLE(wxGenericMDIClientWindow, wxNotebook)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_NOTEBOOK_CLIENT_AREA, wxGenericMDIClientWindow::OnPageChanged)
    EVT_SIZE(wxGenericMDIClientWindow::OnSize)
END_EVENT_TABLE()


wxGenericMDIClientWindow::wxGenericMDIClientWindow()
{
}

wxGenericMDIClientWindow::wxGenericMDIClientWindow( wxGenericMDIParentFrame *parent, long style )
{
    CreateClient( parent, style );
}

wxGenericMDIClientWindow::~wxGenericMDIClientWindow()
{
    DestroyChildren();
}

bool wxGenericMDIClientWindow::CreateClient( wxGenericMDIParentFrame *parent, long style )
{
    SetWindowStyleFlag(style);

    bool success = wxNotebook::Create(parent, wxID_NOTEBOOK_CLIENT_AREA, wxPoint(0,0), wxSize(100, 100), 0);
    if (success)
    {
        /*
        wxFont font(10, wxSWISS, wxNORMAL, wxNORMAL);
        wxFont selFont(10, wxSWISS, wxNORMAL, wxBOLD);
        GetTabView()->SetTabFont(font);
        GetTabView()->SetSelectedTabFont(selFont);
        GetTabView()->SetTabSize(120, 18);
        GetTabView()->SetTabSelectionHeight(20);
        */
        return true;
    }
    else
        return false;
}

int wxGenericMDIClientWindow::SetSelection(size_t nPage)
{
    int oldSelection = wxNotebook::SetSelection(nPage);

#if !defined(__WXMSW__) // No need to do this for wxMSW as wxNotebook::SetSelection()
                        // will already cause this to be done!
    // Handle the page change.
    PageChanged(oldSelection, nPage);
#endif

    return oldSelection;
}

void wxGenericMDIClientWindow::PageChanged(int OldSelection, int newSelection)
{
    // Don't do to much work, only when something realy should change!
    if (OldSelection == newSelection)
        return;
    // Again check if we realy need to do this...
    if (newSelection != -1)
    {
        wxGenericMDIChildFrame* child = (wxGenericMDIChildFrame *)GetPage(newSelection);

        if (child->GetMDIParentFrame()->GetActiveChild() == child)
            return;
    }

    // Notify old active child that it has been deactivated
    if (OldSelection != -1)
    {
        wxGenericMDIChildFrame* oldChild = (wxGenericMDIChildFrame *)GetPage(OldSelection);
        if (oldChild)
        {
            wxActivateEvent event(wxEVT_ACTIVATE, false, oldChild->GetId());
            event.SetEventObject( oldChild );
            oldChild->GetEventHandler()->ProcessEvent(event);
        }
    }

    // Notify new active child that it has been activated
    if (newSelection != -1)
    {
        wxGenericMDIChildFrame* activeChild = (wxGenericMDIChildFrame *)GetPage(newSelection);
        if (activeChild)
        {
            wxActivateEvent event(wxEVT_ACTIVATE, true, activeChild->GetId());
            event.SetEventObject( activeChild );
            activeChild->GetEventHandler()->ProcessEvent(event);

            if (activeChild->GetMDIParentFrame())
            {
                activeChild->GetMDIParentFrame()->SetActiveChild(activeChild);
                activeChild->GetMDIParentFrame()->SetChildMenuBar(activeChild);
            }
        }
    }
}

void wxGenericMDIClientWindow::OnPageChanged(wxNotebookEvent& event)
{
    PageChanged(event.GetOldSelection(), event.GetSelection());

    event.Skip();
}

void wxGenericMDIClientWindow::OnSize(wxSizeEvent& event)
{
    wxNotebook::OnSize(event);

    size_t pos;
    for (pos = 0; pos < GetPageCount(); pos++)
    {
        ((wxGenericMDIChildFrame *)GetPage(pos))->ApplyMDIChildFrameRect();
    }
}


/*
 * Define normal wxMDI classes based on wxGenericMDI
 */

#if wxUSE_GENERIC_MDI_AS_NATIVE

wxMDIChildFrame * wxMDIParentFrame::GetActiveChild() const
    {
        wxGenericMDIChildFrame *pGFrame = wxGenericMDIParentFrame::GetActiveChild();
        wxMDIChildFrame *pFrame = wxDynamicCast(pGFrame, wxMDIChildFrame);

        wxASSERT_MSG(!(pFrame == NULL && pGFrame != NULL), wxT("Active frame is class not derived from wxMDIChildFrame!"));

        return pFrame;
    }

IMPLEMENT_DYNAMIC_CLASS(wxMDIParentFrame, wxGenericMDIParentFrame)
IMPLEMENT_DYNAMIC_CLASS(wxMDIChildFrame, wxGenericMDIChildFrame)
IMPLEMENT_DYNAMIC_CLASS(wxMDIClientWindow, wxGenericMDIClientWindow)

#endif // wxUSE_GENERIC_MDI_AS_NATIVE

#endif // wxUSE_MDI

