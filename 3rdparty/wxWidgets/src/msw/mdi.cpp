/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/mdi.cpp
// Purpose:     MDI classes for wxMSW
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: mdi.cpp 46112 2007-05-18 16:33:45Z VZ $
// Copyright:   (c) Julian Smart
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

#if wxUSE_MDI && !defined(__WXUNIVERSAL__)

#include "wx/mdi.h"

#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/menu.h"
    #include "wx/app.h"
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/statusbr.h"
    #include "wx/settings.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/toolbar.h"
#endif

#include "wx/stockitem.h"
#include "wx/msw/private.h"

#if wxUSE_STATUSBAR && wxUSE_NATIVE_STATUSBAR
    #include "wx/msw/statbr95.h"
#endif

#include <string.h>

// ---------------------------------------------------------------------------
// global variables
// ---------------------------------------------------------------------------

extern wxMenu *wxCurrentPopupMenu;

extern const wxChar *wxMDIFrameClassName;   // from app.cpp
extern const wxChar *wxMDIChildFrameClassName;
extern const wxChar *wxMDIChildFrameClassNameNoRedraw;
extern void wxRemoveHandleAssociation(wxWindow *win);

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------

static const int IDM_WINDOWTILEHOR  = 4001;
static const int IDM_WINDOWCASCADE = 4002;
static const int IDM_WINDOWICONS = 4003;
static const int IDM_WINDOWNEXT = 4004;
static const int IDM_WINDOWTILEVERT = 4005;
static const int IDM_WINDOWPREV = 4006;

// This range gives a maximum of 500 MDI children. Should be enough :-)
static const int wxFIRST_MDI_CHILD = 4100;
static const int wxLAST_MDI_CHILD = 4600;

// ---------------------------------------------------------------------------
// private functions
// ---------------------------------------------------------------------------

// set the MDI menus (by sending the WM_MDISETMENU message) and update the menu
// of the parent of win (which is supposed to be the MDI client window)
static void MDISetMenu(wxWindow *win, HMENU hmenuFrame, HMENU hmenuWindow);

// insert the window menu (subMenu) into menu just before "Help" submenu or at
// the very end if not found
static void InsertWindowMenu(wxWindow *win, WXHMENU menu, HMENU subMenu);

// Remove the window menu
static void RemoveWindowMenu(wxWindow *win, WXHMENU menu);

// is this an id of an MDI child?
inline bool IsMdiCommandId(int id)
{
    return (id >= wxFIRST_MDI_CHILD) && (id <= wxLAST_MDI_CHILD);
}

// unpack the parameters of WM_MDIACTIVATE message
static void UnpackMDIActivate(WXWPARAM wParam, WXLPARAM lParam,
                              WXWORD *activate, WXHWND *hwndAct, WXHWND *hwndDeact);

// return the HMENU of the MDI menu
static inline HMENU GetMDIWindowMenu(wxMDIParentFrame *frame)
{
    wxMenu *menu = frame->GetWindowMenu();
    return menu ? GetHmenuOf(menu) : 0;
}

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxWin macros
// ---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxMDIParentFrame, wxFrame)
IMPLEMENT_DYNAMIC_CLASS(wxMDIChildFrame, wxFrame)
IMPLEMENT_DYNAMIC_CLASS(wxMDIClientWindow, wxWindow)

BEGIN_EVENT_TABLE(wxMDIParentFrame, wxFrame)
    EVT_SIZE(wxMDIParentFrame::OnSize)
    EVT_ICONIZE(wxMDIParentFrame::OnIconized)
    EVT_SYS_COLOUR_CHANGED(wxMDIParentFrame::OnSysColourChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMDIChildFrame, wxFrame)
    EVT_IDLE(wxMDIChildFrame::OnIdle)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMDIClientWindow, wxWindow)
    EVT_SCROLL(wxMDIClientWindow::OnScroll)
END_EVENT_TABLE()

// ===========================================================================
// wxMDIParentFrame: the frame which contains the client window which manages
// the children
// ===========================================================================

wxMDIParentFrame::wxMDIParentFrame()
{
    m_clientWindow = NULL;
    m_currentChild = NULL;
    m_windowMenu = (wxMenu*) NULL;
    m_parentFrameActive = true;
}

bool wxMDIParentFrame::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxString& title,
                              const wxPoint& pos,
                              const wxSize& size,
                              long style,
                              const wxString& name)
{
  m_clientWindow = NULL;
  m_currentChild = NULL;

  // this style can be used to prevent a window from having the standard MDI
  // "Window" menu
  if ( style & wxFRAME_NO_WINDOW_MENU )
  {
      m_windowMenu = (wxMenu *)NULL;
  }
  else // normal case: we have the window menu, so construct it
  {
      m_windowMenu = new wxMenu;

      m_windowMenu->Append(IDM_WINDOWCASCADE, _("&Cascade"));
      m_windowMenu->Append(IDM_WINDOWTILEHOR, _("Tile &Horizontally"));
      m_windowMenu->Append(IDM_WINDOWTILEVERT, _("Tile &Vertically"));
      m_windowMenu->AppendSeparator();
      m_windowMenu->Append(IDM_WINDOWICONS, _("&Arrange Icons"));
      m_windowMenu->Append(IDM_WINDOWNEXT, _("&Next"));
      m_windowMenu->Append(IDM_WINDOWPREV, _("&Previous"));
  }

  m_parentFrameActive = true;

  if (!parent)
    wxTopLevelWindows.Append(this);

  SetName(name);
  m_windowStyle = style;

  if ( parent )
      parent->AddChild(this);

  if ( id != wxID_ANY )
    m_windowId = id;
  else
    m_windowId = NewControlId();

  WXDWORD exflags;
  WXDWORD msflags = MSWGetCreateWindowFlags(&exflags);
  msflags &= ~WS_VSCROLL;
  msflags &= ~WS_HSCROLL;

  if ( !wxWindow::MSWCreate(wxMDIFrameClassName,
                            title,
                            pos, size,
                            msflags,
                            exflags) )
  {
      return false;
  }

  SetOwnBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));

  // unlike (almost?) all other windows, frames are created hidden
  m_isShown = false;

  return true;
}

wxMDIParentFrame::~wxMDIParentFrame()
{
    // see comment in ~wxMDIChildFrame
#if wxUSE_TOOLBAR
    m_frameToolBar = NULL;
#endif
#if wxUSE_STATUSBAR
    m_frameStatusBar = NULL;
#endif // wxUSE_STATUSBAR

    DestroyChildren();

    if (m_windowMenu)
    {
        delete m_windowMenu;
        m_windowMenu = (wxMenu*) NULL;
    }

    // the MDI frame menubar is not automatically deleted by Windows unlike for
    // the normal frames
    if ( m_hMenu )
    {
        ::DestroyMenu((HMENU)m_hMenu);
        m_hMenu = (WXHMENU)NULL;
    }

    if ( m_clientWindow )
    {
        if ( m_clientWindow->MSWGetOldWndProc() )
            m_clientWindow->UnsubclassWin();

        m_clientWindow->SetHWND(0);
        delete m_clientWindow;
    }
}

#if wxUSE_MENUS_NATIVE

void wxMDIParentFrame::InternalSetMenuBar()
{
    m_parentFrameActive = true;

    InsertWindowMenu(GetClientWindow(), m_hMenu, GetMDIWindowMenu(this));
}

#endif // wxUSE_MENUS_NATIVE

void wxMDIParentFrame::SetWindowMenu(wxMenu* menu)
{
    if (m_windowMenu)
    {
        if (GetMenuBar())
        {
            // Remove old window menu
            RemoveWindowMenu(GetClientWindow(), m_hMenu);
        }

        delete m_windowMenu;
        m_windowMenu = (wxMenu*) NULL;
    }

    if (menu)
    {
        m_windowMenu = menu;
        if (GetMenuBar())
        {
            InsertWindowMenu(GetClientWindow(), m_hMenu,
                             GetHmenuOf(m_windowMenu));
        }
    }
}

void wxMDIParentFrame::DoMenuUpdates(wxMenu* menu)
{
    wxMDIChildFrame *child = GetActiveChild();
    if ( child )
    {
        wxEvtHandler* source = child->GetEventHandler();
        wxMenuBar* bar = child->GetMenuBar();

        if (menu)
        {
            menu->UpdateUI(source);
        }
        else
        {
            if ( bar != NULL )
            {
                int nCount = bar->GetMenuCount();
                for (int n = 0; n < nCount; n++)
                    bar->GetMenu(n)->UpdateUI(source);
            }
        }
    }
    else
    {
        wxFrameBase::DoMenuUpdates(menu);
    }
}

void wxMDIParentFrame::UpdateClientSize()
{
    if ( GetClientWindow() )
    {
        int width, height;
        GetClientSize(&width, &height);

        GetClientWindow()->SetSize(0, 0, width, height);
    }
}

void wxMDIParentFrame::OnSize(wxSizeEvent& WXUNUSED(event))
{
    UpdateClientSize();

    // do not call event.Skip() here, it somehow messes up MDI client window
}

void wxMDIParentFrame::OnIconized(wxIconizeEvent& event)
{
    event.Skip();

    if ( !event.Iconized() )
    {
        UpdateClientSize();
    }
}

// Returns the active MDI child window
wxMDIChildFrame *wxMDIParentFrame::GetActiveChild() const
{
    HWND hWnd = (HWND)::SendMessage(GetWinHwnd(GetClientWindow()),
                                    WM_MDIGETACTIVE, 0, 0L);
    if ( hWnd == 0 )
        return NULL;
    else
        return (wxMDIChildFrame *)wxFindWinFromHandle((WXHWND) hWnd);
}

// Create the client window class (don't Create the window, just return a new
// class)
wxMDIClientWindow *wxMDIParentFrame::OnCreateClient()
{
    return new wxMDIClientWindow;
}

// Responds to colour changes, and passes event on to children.
void wxMDIParentFrame::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    if ( m_clientWindow )
    {
        m_clientWindow->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
        m_clientWindow->Refresh();
    }

    event.Skip();
}

WXHICON wxMDIParentFrame::GetDefaultIcon() const
{
    // we don't have any standard icons (any more)
    return (WXHICON)0;
}

// ---------------------------------------------------------------------------
// MDI operations
// ---------------------------------------------------------------------------

void wxMDIParentFrame::Cascade()
{
    ::SendMessage(GetWinHwnd(GetClientWindow()), WM_MDICASCADE, 0, 0);
}

void wxMDIParentFrame::Tile(wxOrientation orient)
{
    wxASSERT_MSG( orient == wxHORIZONTAL || orient == wxVERTICAL,
                  _T("invalid orientation value") );

    ::SendMessage(GetWinHwnd(GetClientWindow()), WM_MDITILE,
                  orient == wxHORIZONTAL ? MDITILE_HORIZONTAL
                                         : MDITILE_VERTICAL, 0);
}

void wxMDIParentFrame::ArrangeIcons()
{
    ::SendMessage(GetWinHwnd(GetClientWindow()), WM_MDIICONARRANGE, 0, 0);
}

void wxMDIParentFrame::ActivateNext()
{
    ::SendMessage(GetWinHwnd(GetClientWindow()), WM_MDINEXT, 0, 0);
}

void wxMDIParentFrame::ActivatePrevious()
{
    ::SendMessage(GetWinHwnd(GetClientWindow()), WM_MDINEXT, 0, 1);
}

// ---------------------------------------------------------------------------
// the MDI parent frame window proc
// ---------------------------------------------------------------------------

WXLRESULT wxMDIParentFrame::MSWWindowProc(WXUINT message,
                                     WXWPARAM wParam,
                                     WXLPARAM lParam)
{
    WXLRESULT rc = 0;
    bool processed = false;

    switch ( message )
    {
        case WM_ACTIVATE:
            {
                WXWORD state, minimized;
                WXHWND hwnd;
                UnpackActivate(wParam, lParam, &state, &minimized, &hwnd);

                processed = HandleActivate(state, minimized != 0, hwnd);
            }
            break;

        case WM_COMMAND:
            {
                WXWORD id, cmd;
                WXHWND hwnd;
                UnpackCommand(wParam, lParam, &id, &hwnd, &cmd);

                (void)HandleCommand(id, cmd, hwnd);

                // even if the frame didn't process it, there is no need to try it
                // once again (i.e. call wxFrame::HandleCommand()) - we just did it,
                // so pretend we processed the message anyhow
                processed = true;
            }

            // always pass this message DefFrameProc(), otherwise MDI menu
            // commands (and sys commands - more surprisingly!) won't work
            MSWDefWindowProc(message, wParam, lParam);
            break;

        case WM_CREATE:
            m_clientWindow = OnCreateClient();
            // Uses own style for client style
            if ( !m_clientWindow->CreateClient(this, GetWindowStyleFlag()) )
            {
                wxLogMessage(_("Failed to create MDI parent frame."));

                rc = -1;
            }

            processed = true;
            break;

        case WM_ERASEBKGND:
            processed = true;

            // we erase background ourselves
            rc = true;
            break;

        case WM_MENUSELECT:
            {
                WXWORD item, flags;
                WXHMENU hmenu;
                UnpackMenuSelect(wParam, lParam, &item, &flags, &hmenu);

                if ( m_parentFrameActive )
                {
                    processed = HandleMenuSelect(item, flags, hmenu);
                }
                else if (m_currentChild)
                {
                    processed = m_currentChild->
                        HandleMenuSelect(item, flags, hmenu);
                }
            }
            break;

        case WM_SIZE:
            // though we don't (usually) resize the MDI client to exactly fit the
            // client area we need to pass this one to DefFrameProc to allow the children to show
            break;
    }

    if ( !processed )
        rc = wxFrame::MSWWindowProc(message, wParam, lParam);

    return rc;
}

bool wxMDIParentFrame::HandleActivate(int state, bool minimized, WXHWND activate)
{
    bool processed = false;

    if ( wxWindow::HandleActivate(state, minimized, activate) )
    {
        // already processed
        processed = true;
    }

    // If this window is an MDI parent, we must also send an OnActivate message
    // to the current child.
    if ( (m_currentChild != NULL) &&
         ((state == WA_ACTIVE) || (state == WA_CLICKACTIVE)) )
    {
        wxActivateEvent event(wxEVT_ACTIVATE, true, m_currentChild->GetId());
        event.SetEventObject( m_currentChild );
        if ( m_currentChild->GetEventHandler()->ProcessEvent(event) )
            processed = true;
    }

    return processed;
}

bool wxMDIParentFrame::HandleCommand(WXWORD id, WXWORD cmd, WXHWND hwnd)
{
    // In case it's e.g. a toolbar.
    if ( hwnd )
    {
        wxWindow *win = wxFindWinFromHandle(hwnd);
        if ( win )
            return win->MSWCommand(cmd, id);
    }

    if (wxCurrentPopupMenu)
    {
        wxMenu *popupMenu = wxCurrentPopupMenu;
        wxCurrentPopupMenu = NULL;
        if (popupMenu->MSWCommand(cmd, id))
            return true;
    }

    // is it one of standard MDI commands?
    WXWPARAM wParam = 0;
    WXLPARAM lParam = 0;
    int msg;
    switch ( id )
    {
        case IDM_WINDOWCASCADE:
            msg = WM_MDICASCADE;
            wParam = MDITILE_SKIPDISABLED;
            break;

        case IDM_WINDOWTILEHOR:
            wParam |= MDITILE_HORIZONTAL;
            // fall through

        case IDM_WINDOWTILEVERT:
            if ( !wParam )
                wParam = MDITILE_VERTICAL;
            msg = WM_MDITILE;
            wParam |= MDITILE_SKIPDISABLED;
            break;

        case IDM_WINDOWICONS:
            msg = WM_MDIICONARRANGE;
            break;

        case IDM_WINDOWNEXT:
            msg = WM_MDINEXT;
            lParam = 0;         // next child
            break;

        case IDM_WINDOWPREV:
            msg = WM_MDINEXT;
            lParam = 1;         // previous child
            break;

        default:
            msg = 0;
    }

    if ( msg )
    {
        ::SendMessage(GetWinHwnd(GetClientWindow()), msg, wParam, lParam);

        return true;
    }

    // FIXME VZ: what does this test do??
    if (id >= 0xF000)
    {
        return false; // Get WndProc to call default proc
    }

    if ( IsMdiCommandId(id) )
    {
        wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
        while ( node )
        {
            wxWindow *child = node->GetData();
            if ( child->GetHWND() )
            {
                long childId = wxGetWindowId(child->GetHWND());
                if (childId == (long)id)
                {
                    ::SendMessage( GetWinHwnd(GetClientWindow()),
                                   WM_MDIACTIVATE,
                                   (WPARAM)child->GetHWND(), 0);
                    return true;
                }
            }
            node = node->GetNext();
        }
    }
    else if ( m_parentFrameActive )
    {
        return ProcessCommand(id);
    }
    else if ( m_currentChild )
    {
        return m_currentChild->HandleCommand(id, cmd, hwnd);
    }
    else
    {
        // this shouldn't happen because it means that our messages are being
        // lost (they're not sent to the parent frame nor to the children)
        wxFAIL_MSG(wxT("MDI parent frame is not active, yet there is no active MDI child?"));
    }

    return false;
}

WXLRESULT wxMDIParentFrame::MSWDefWindowProc(WXUINT message,
                                        WXWPARAM wParam,
                                        WXLPARAM lParam)
{
    WXHWND clientWnd;
    if ( GetClientWindow() )
        clientWnd = GetClientWindow()->GetHWND();
    else
        clientWnd = 0;

    return DefFrameProc(GetHwnd(), (HWND)clientWnd, message, wParam, lParam);
}

bool wxMDIParentFrame::MSWTranslateMessage(WXMSG* msg)
{
    MSG *pMsg = (MSG *)msg;

    // first let the current child get it
    if ( m_currentChild && m_currentChild->GetHWND() &&
         m_currentChild->MSWTranslateMessage(msg) )
    {
        return true;
    }

    // then try out accel table (will also check the menu accels)
    if ( wxFrame::MSWTranslateMessage(msg) )
    {
        return true;
    }

    // finally, check for MDI specific built in accel keys
    if ( pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN )
    {
        if ( ::TranslateMDISysAccel(GetWinHwnd(GetClientWindow()), pMsg))
            return true;
    }

    return false;
}

// ===========================================================================
// wxMDIChildFrame
// ===========================================================================

void wxMDIChildFrame::Init()
{
    m_needsResize = true;
    m_needsInitialShow = true;
}

bool wxMDIChildFrame::Create(wxMDIParentFrame *parent,
                             wxWindowID id,
                             const wxString& title,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name)
{
  SetName(name);

  if ( id != wxID_ANY )
    m_windowId = id;
  else
    m_windowId = (int)NewControlId();

  if ( parent )
  {
      parent->AddChild(this);
  }

  int x = pos.x;
  int y = pos.y;
  int width = size.x;
  int height = size.y;

  MDICREATESTRUCT mcs;

  mcs.szClass = style & wxFULL_REPAINT_ON_RESIZE
                    ? wxMDIChildFrameClassName
                    : wxMDIChildFrameClassNameNoRedraw;
  mcs.szTitle = title;
  mcs.hOwner = wxGetInstance();
  if (x != wxDefaultCoord)
      mcs.x = x;
  else
      mcs.x = CW_USEDEFAULT;

  if (y != wxDefaultCoord)
      mcs.y = y;
  else
      mcs.y = CW_USEDEFAULT;

  if (width != wxDefaultCoord)
      mcs.cx = width;
  else
      mcs.cx = CW_USEDEFAULT;

  if (height != wxDefaultCoord)
      mcs.cy = height;
  else
      mcs.cy = CW_USEDEFAULT;

  DWORD msflags = WS_OVERLAPPED | WS_CLIPCHILDREN;
  if (style & wxMINIMIZE_BOX)
    msflags |= WS_MINIMIZEBOX;
  if (style & wxMAXIMIZE_BOX)
    msflags |= WS_MAXIMIZEBOX;
  if (style & wxRESIZE_BORDER)
    msflags |= WS_THICKFRAME;
  if (style & wxSYSTEM_MENU)
    msflags |= WS_SYSMENU;
  if ((style & wxMINIMIZE) || (style & wxICONIZE))
    msflags |= WS_MINIMIZE;
  if (style & wxMAXIMIZE)
    msflags |= WS_MAXIMIZE;
  if (style & wxCAPTION)
    msflags |= WS_CAPTION;

  mcs.style = msflags;

  mcs.lParam = 0;

  wxWindowCreationHook hook(this);

  m_hWnd = (WXHWND)::SendMessage(GetWinHwnd(parent->GetClientWindow()),
                                 WM_MDICREATE, 0, (LONG)(LPSTR)&mcs);

  if ( !m_hWnd )
  {
      wxLogLastError(_T("WM_MDICREATE"));
      return false;
  }

  SubclassWin(m_hWnd);

  return true;
}

wxMDIChildFrame::~wxMDIChildFrame()
{
    // if we hadn't been created, there is nothing to destroy
    if ( !m_hWnd )
        return;

    // will be destroyed by DestroyChildren() but reset them before calling it
    // to avoid using dangling pointers if a callback comes in the meanwhile
#if wxUSE_TOOLBAR
    m_frameToolBar = NULL;
#endif
#if wxUSE_STATUSBAR
    m_frameStatusBar = NULL;
#endif // wxUSE_STATUSBAR

    DestroyChildren();

    RemoveWindowMenu(NULL, m_hMenu);

    MSWDestroyWindow();
}

bool wxMDIChildFrame::Show(bool show)
{
    m_needsInitialShow = false;

    if (!wxFrame::Show(show))
        return false;

    // KH: Without this call, new MDI children do not become active.
    // This was added here after the same BringWindowToTop call was
    // removed from wxTopLevelWindow::Show (November 2005)
    if ( show )
        ::BringWindowToTop(GetHwnd());

    // we need to refresh the MDI frame window menu to include (or exclude if
    // we've been hidden) this frame
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();
    MDISetMenu(parent->GetClientWindow(), NULL, NULL);

    return true;
}

// Set the client size (i.e. leave the calculation of borders etc.
// to wxWidgets)
void wxMDIChildFrame::DoSetClientSize(int width, int height)
{
  HWND hWnd = GetHwnd();

  RECT rect;
  ::GetClientRect(hWnd, &rect);

  RECT rect2;
  GetWindowRect(hWnd, &rect2);

  // Find the difference between the entire window (title bar and all)
  // and the client area; add this to the new client size to move the
  // window
  int actual_width = rect2.right - rect2.left - rect.right + width;
  int actual_height = rect2.bottom - rect2.top - rect.bottom + height;

#if wxUSE_STATUSBAR
  if (GetStatusBar() && GetStatusBar()->IsShown())
  {
    int sx, sy;
    GetStatusBar()->GetSize(&sx, &sy);
    actual_height += sy;
  }
#endif // wxUSE_STATUSBAR

  POINT point;
  point.x = rect2.left;
  point.y = rect2.top;

  // If there's an MDI parent, must subtract the parent's top left corner
  // since MoveWindow moves relative to the parent
  wxMDIParentFrame *mdiParent = (wxMDIParentFrame *)GetParent();
  ::ScreenToClient((HWND) mdiParent->GetClientWindow()->GetHWND(), &point);

  MoveWindow(hWnd, point.x, point.y, actual_width, actual_height, (BOOL)true);

  wxSize size(width, height);
  wxSizeEvent event(size, m_windowId);
  event.SetEventObject( this );
  GetEventHandler()->ProcessEvent(event);
}

// Unlike other wxTopLevelWindowBase, the mdi child's "GetPosition" is not the
//  same as its GetScreenPosition
void wxMDIChildFrame::DoGetScreenPosition(int *x, int *y) const
{
  HWND hWnd = GetHwnd();

  RECT rect;
  ::GetWindowRect(hWnd, &rect);
  if (x)
     *x = rect.left;
  if (y)
     *y = rect.top;
}


void wxMDIChildFrame::DoGetPosition(int *x, int *y) const
{
  RECT rect;
  GetWindowRect(GetHwnd(), &rect);
  POINT point;
  point.x = rect.left;
  point.y = rect.top;

  // Since we now have the absolute screen coords,
  // if there's a parent we must subtract its top left corner
  wxMDIParentFrame *mdiParent = (wxMDIParentFrame *)GetParent();
  ::ScreenToClient((HWND) mdiParent->GetClientWindow()->GetHWND(), &point);

  if (x)
      *x = point.x;
  if (y)
      *y = point.y;
}

void wxMDIChildFrame::InternalSetMenuBar()
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();

    InsertWindowMenu(parent->GetClientWindow(),
                     m_hMenu, GetMDIWindowMenu(parent));

    parent->m_parentFrameActive = false;
}

void wxMDIChildFrame::DetachMenuBar()
{
    RemoveWindowMenu(NULL, m_hMenu);
    wxFrame::DetachMenuBar();
}

WXHICON wxMDIChildFrame::GetDefaultIcon() const
{
    // we don't have any standard icons (any more)
    return (WXHICON)0;
}

// ---------------------------------------------------------------------------
// MDI operations
// ---------------------------------------------------------------------------

void wxMDIChildFrame::Maximize(bool maximize)
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();
    if ( parent && parent->GetClientWindow() )
    {
        ::SendMessage(GetWinHwnd(parent->GetClientWindow()),
                      maximize ? WM_MDIMAXIMIZE : WM_MDIRESTORE,
                      (WPARAM)GetHwnd(), 0);
    }
}

void wxMDIChildFrame::Restore()
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();
    if ( parent && parent->GetClientWindow() )
    {
        ::SendMessage(GetWinHwnd(parent->GetClientWindow()), WM_MDIRESTORE,
                      (WPARAM) GetHwnd(), 0);
    }
}

void wxMDIChildFrame::Activate()
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();
    if ( parent && parent->GetClientWindow() )
    {
        ::SendMessage(GetWinHwnd(parent->GetClientWindow()), WM_MDIACTIVATE,
                      (WPARAM) GetHwnd(), 0);
    }
}

// ---------------------------------------------------------------------------
// MDI window proc and message handlers
// ---------------------------------------------------------------------------

WXLRESULT wxMDIChildFrame::MSWWindowProc(WXUINT message,
                                    WXWPARAM wParam,
                                    WXLPARAM lParam)
{
    WXLRESULT rc = 0;
    bool processed = false;

    switch ( message )
    {
        case WM_COMMAND:
            {
                WORD id, cmd;
                WXHWND hwnd;
                UnpackCommand((WXWPARAM)wParam, (WXLPARAM)lParam,
                              &id, &hwnd, &cmd);

                processed = HandleCommand(id, cmd, (WXHWND)hwnd);
            }
            break;

        case WM_GETMINMAXINFO:
            processed = HandleGetMinMaxInfo((MINMAXINFO *)lParam);
            break;

        case WM_MDIACTIVATE:
            {
                WXWORD act;
                WXHWND hwndAct, hwndDeact;
                UnpackMDIActivate(wParam, lParam, &act, &hwndAct, &hwndDeact);

                processed = HandleMDIActivate(act, hwndAct, hwndDeact);
            }
            // fall through

        case WM_MOVE:
            // must pass WM_MOVE to DefMDIChildProc() to recalculate MDI client
            // scrollbars if necessary

            // fall through

        case WM_SIZE:
            // must pass WM_SIZE to DefMDIChildProc(), otherwise many weird
            // things happen
            MSWDefWindowProc(message, wParam, lParam);
            break;

        case WM_SYSCOMMAND:
            // DefMDIChildProc handles SC_{NEXT/PREV}WINDOW here, so pass it
            // the message (the base class version does not)
            return MSWDefWindowProc(message, wParam, lParam);

        case WM_WINDOWPOSCHANGING:
            processed = HandleWindowPosChanging((LPWINDOWPOS)lParam);
            break;
    }

    if ( !processed )
        rc = wxFrame::MSWWindowProc(message, wParam, lParam);

    return rc;
}

bool wxMDIChildFrame::HandleCommand(WXWORD id, WXWORD cmd, WXHWND hwnd)
{
    // In case it's e.g. a toolbar.
    if ( hwnd )
    {
        wxWindow *win = wxFindWinFromHandle(hwnd);
        if (win)
            return win->MSWCommand(cmd, id);
    }

    if (wxCurrentPopupMenu)
    {
        wxMenu *popupMenu = wxCurrentPopupMenu;
        wxCurrentPopupMenu = NULL;
        if (popupMenu->MSWCommand(cmd, id))
            return true;
    }

    bool processed;
    if (GetMenuBar() && GetMenuBar()->FindItem(id))
    {
        processed = ProcessCommand(id);
    }
    else
    {
        processed = false;
    }

    return processed;
}

bool wxMDIChildFrame::HandleMDIActivate(long WXUNUSED(activate),
                                        WXHWND hwndAct,
                                        WXHWND hwndDeact)
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();

    HMENU menuToSet = 0;

    bool activated;

    if ( m_hWnd == hwndAct )
    {
        activated = true;
        parent->m_currentChild = this;

        HMENU child_menu = (HMENU)GetWinMenu();
        if ( child_menu )
        {
            parent->m_parentFrameActive = false;

            menuToSet = child_menu;
        }
    }
    else if ( m_hWnd == hwndDeact )
    {
        wxASSERT_MSG( parent->m_currentChild == this,
                      wxT("can't deactivate MDI child which wasn't active!") );

        activated = false;
        parent->m_currentChild = NULL;

        HMENU parent_menu = (HMENU)parent->GetWinMenu();

        // activate the the parent menu only when there is no other child
        // that has been activated
        if ( parent_menu && !hwndAct )
        {
            parent->m_parentFrameActive = true;

            menuToSet = parent_menu;
        }
    }
    else
    {
        // we have nothing to do with it
        return false;
    }

    if ( menuToSet )
    {
        MDISetMenu(parent->GetClientWindow(),
                   menuToSet, GetMDIWindowMenu(parent));
    }

    wxActivateEvent event(wxEVT_ACTIVATE, activated, m_windowId);
    event.SetEventObject( this );

    ResetWindowStyle((void *)NULL);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxMDIChildFrame::HandleWindowPosChanging(void *pos)
{
    WINDOWPOS *lpPos = (WINDOWPOS *)pos;

    if (!(lpPos->flags & SWP_NOSIZE))
    {
        RECT rectClient;
        DWORD dwExStyle = ::GetWindowLong(GetHwnd(), GWL_EXSTYLE);
        DWORD dwStyle = ::GetWindowLong(GetHwnd(), GWL_STYLE);
        if (ResetWindowStyle((void *) & rectClient) && (dwStyle & WS_MAXIMIZE))
        {
            ::AdjustWindowRectEx(&rectClient, dwStyle, false, dwExStyle);
            lpPos->x = rectClient.left;
            lpPos->y = rectClient.top;
            lpPos->cx = rectClient.right - rectClient.left;
            lpPos->cy = rectClient.bottom - rectClient.top;
        }
    }

    return false;
}

bool wxMDIChildFrame::HandleGetMinMaxInfo(void *mmInfo)
{
    MINMAXINFO *info = (MINMAXINFO *)mmInfo;

    // let the default window proc calculate the size of MDI children
    // frames because it is based on the size of the MDI client window,
    // not on the values specified in wxWindow m_max variables
    bool processed = MSWDefWindowProc(WM_GETMINMAXINFO, 0, (LPARAM)mmInfo) != 0;

    int minWidth = GetMinWidth(),
        minHeight = GetMinHeight();

    // but allow GetSizeHints() to set the min size
    if ( minWidth != wxDefaultCoord )
    {
        info->ptMinTrackSize.x = minWidth;

        processed = true;
    }

    if ( minHeight != wxDefaultCoord )
    {
        info->ptMinTrackSize.y = minHeight;

        processed = true;
    }

    return processed;
}

// ---------------------------------------------------------------------------
// MDI specific message translation/preprocessing
// ---------------------------------------------------------------------------

WXLRESULT wxMDIChildFrame::MSWDefWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    return DefMDIChildProc(GetHwnd(),
                           (UINT)message, (WPARAM)wParam, (LPARAM)lParam);
}

bool wxMDIChildFrame::MSWTranslateMessage(WXMSG* msg)
{
    // we must pass the parent frame to ::TranslateAccelerator(), otherwise it
    // doesn't do its job correctly for MDI child menus
    return MSWDoTranslateMessage((wxMDIChildFrame *)GetParent(), msg);
}

// ---------------------------------------------------------------------------
// misc
// ---------------------------------------------------------------------------

void wxMDIChildFrame::MSWDestroyWindow()
{
    wxMDIParentFrame *parent = (wxMDIParentFrame *)GetParent();

    // Must make sure this handle is invalidated (set to NULL) since all sorts
    // of things could happen after the child client is destroyed, but before
    // the wxFrame is destroyed.

    HWND oldHandle = (HWND)GetHWND();
    SendMessage(GetWinHwnd(parent->GetClientWindow()), WM_MDIDESTROY,
                (WPARAM)oldHandle, 0);

    if (parent->GetActiveChild() == (wxMDIChildFrame*) NULL)
        ResetWindowStyle((void*) NULL);

    if (m_hMenu)
    {
        ::DestroyMenu((HMENU) m_hMenu);
        m_hMenu = 0;
    }
    wxRemoveHandleAssociation(this);
    m_hWnd = 0;
}

// Change the client window's extended style so we don't get a client edge
// style when a child is maximised (a double border looks silly.)
bool wxMDIChildFrame::ResetWindowStyle(void *vrect)
{
    RECT *rect = (RECT *)vrect;
    wxMDIParentFrame* pFrameWnd = (wxMDIParentFrame *)GetParent();
    wxMDIChildFrame* pChild = pFrameWnd->GetActiveChild();

    if (!pChild || (pChild == this))
    {
        HWND hwndClient = GetWinHwnd(pFrameWnd->GetClientWindow());
        DWORD dwStyle = ::GetWindowLong(hwndClient, GWL_EXSTYLE);

        // we want to test whether there is a maximized child, so just set
        // dwThisStyle to 0 if there is no child at all
        DWORD dwThisStyle = pChild
            ? ::GetWindowLong(GetWinHwnd(pChild), GWL_STYLE) : 0;
        DWORD dwNewStyle = dwStyle;
        if ( dwThisStyle & WS_MAXIMIZE )
            dwNewStyle &= ~(WS_EX_CLIENTEDGE);
        else
            dwNewStyle |= WS_EX_CLIENTEDGE;

        if (dwStyle != dwNewStyle)
        {
            // force update of everything
            ::RedrawWindow(hwndClient, NULL, NULL,
                           RDW_INVALIDATE | RDW_ALLCHILDREN);
            ::SetWindowLong(hwndClient, GWL_EXSTYLE, dwNewStyle);
            ::SetWindowPos(hwndClient, NULL, 0, 0, 0, 0,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE |
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                           SWP_NOCOPYBITS);
            if (rect)
                ::GetClientRect(hwndClient, rect);

            return true;
        }
    }

    return false;
}

// ===========================================================================
// wxMDIClientWindow: the window of predefined (by Windows) class which
// contains the child frames
// ===========================================================================

bool wxMDIClientWindow::CreateClient(wxMDIParentFrame *parent, long style)
{
    m_backgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);

    CLIENTCREATESTRUCT ccs;
    m_windowStyle = style;
    m_parent = parent;

    ccs.hWindowMenu = GetMDIWindowMenu(parent);
    ccs.idFirstChild = wxFIRST_MDI_CHILD;

    DWORD msStyle = MDIS_ALLCHILDSTYLES | WS_VISIBLE | WS_CHILD |
                    WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    if ( style & wxHSCROLL )
        msStyle |= WS_HSCROLL;
    if ( style & wxVSCROLL )
        msStyle |= WS_VSCROLL;

    DWORD exStyle = WS_EX_CLIENTEDGE;

    wxWindowCreationHook hook(this);
    m_hWnd = (WXHWND)::CreateWindowEx
                       (
                        exStyle,
                        wxT("MDICLIENT"),
                        NULL,
                        msStyle,
                        0, 0, 0, 0,
                        GetWinHwnd(parent),
                        NULL,
                        wxGetInstance(),
                        (LPSTR)(LPCLIENTCREATESTRUCT)&ccs);
    if ( !m_hWnd )
    {
        wxLogLastError(wxT("CreateWindowEx(MDI client)"));

        return false;
    }

    SubclassWin(m_hWnd);

    return true;
}

// Explicitly call default scroll behaviour
void wxMDIClientWindow::OnScroll(wxScrollEvent& event)
{
    // Note: for client windows, the scroll position is not set in
    // WM_HSCROLL, WM_VSCROLL, so we can't easily determine what
    // scroll position we're at.
    // This makes it hard to paint patterns or bitmaps in the background,
    // and have the client area scrollable as well.

    if ( event.GetOrientation() == wxHORIZONTAL )
        m_scrollX = event.GetPosition(); // Always returns zero!
    else
        m_scrollY = event.GetPosition(); // Always returns zero!

    event.Skip();
}

void wxMDIClientWindow::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    // Try to fix a problem whereby if you show an MDI child frame, then reposition the
    // client area, you can end up with a non-refreshed portion in the client window
    // (see OGL studio sample). So check if the position is changed and if so,
    // redraw the MDI child frames.

    const wxPoint oldPos = GetPosition();

    wxWindow::DoSetSize(x, y, width, height, sizeFlags | wxSIZE_FORCE);

    const wxPoint newPos = GetPosition();

    if ((newPos.x != oldPos.x) || (newPos.y != oldPos.y))
    {
        if (GetParent())
        {
            wxWindowList::compatibility_iterator node = GetParent()->GetChildren().GetFirst();
            while (node)
            {
                wxWindow *child = node->GetData();
                if (child->IsKindOf(CLASSINFO(wxMDIChildFrame)))
                {
                   ::RedrawWindow(GetHwndOf(child),
                                  NULL,
                                  NULL,
                                  RDW_FRAME |
                                  RDW_ALLCHILDREN |
                                  RDW_INVALIDATE);
                }
                node = node->GetNext();
            }
        }
    }
}

void wxMDIChildFrame::OnIdle(wxIdleEvent& event)
{
    // wxMSW prior to 2.5.3 created MDI child frames as visible, which resulted
    // in flicker e.g. when the frame contained controls with non-trivial
    // layout. Since 2.5.3, the frame is created hidden as all other top level
    // windows. In order to maintain backward compatibility, the frame is shown
    // in OnIdle, unless Show(false) was called by the programmer before.
    if ( m_needsInitialShow )
    {
        Show(true);
    }

    // MDI child frames get their WM_SIZE when they're constructed but at this
    // moment they don't have any children yet so all child windows will be
    // positioned incorrectly when they are added later - to fix this, we
    // generate an artificial size event here
    if ( m_needsResize )
    {
        m_needsResize = false; // avoid any possibility of recursion

        SendSizeEvent();
    }

    event.Skip();
}

// ---------------------------------------------------------------------------
// non member functions
// ---------------------------------------------------------------------------

static void MDISetMenu(wxWindow *win, HMENU hmenuFrame, HMENU hmenuWindow)
{
    if ( hmenuFrame || hmenuWindow )
    {
        if ( !::SendMessage(GetWinHwnd(win),
                            WM_MDISETMENU,
                            (WPARAM)hmenuFrame,
                            (LPARAM)hmenuWindow) )
        {
            wxLogLastError(_T("SendMessage(WM_MDISETMENU)"));
        }
    }

    // update menu bar of the parent window
    wxWindow *parent = win->GetParent();
    wxCHECK_RET( parent, wxT("MDI client without parent frame? weird...") );

    ::SendMessage(GetWinHwnd(win), WM_MDIREFRESHMENU, 0, 0L);

    ::DrawMenuBar(GetWinHwnd(parent));
}

static void InsertWindowMenu(wxWindow *win, WXHMENU menu, HMENU subMenu)
{
    // Try to insert Window menu in front of Help, otherwise append it.
    HMENU hmenu = (HMENU)menu;

    if (subMenu)
    {
        int N = GetMenuItemCount(hmenu);
        bool success = false;
        for ( int i = 0; i < N; i++ )
        {
            wxChar buf[256];
            int chars = GetMenuString(hmenu, i, buf, WXSIZEOF(buf), MF_BYPOSITION);
            if ( chars == 0 )
            {
                wxLogLastError(wxT("GetMenuString"));

                continue;
            }

            wxString strBuf(buf);
            if ( wxStripMenuCodes(strBuf) == wxGetStockLabel(wxID_HELP,false) )
            {
                success = true;
                ::InsertMenu(hmenu, i, MF_BYPOSITION | MF_POPUP | MF_STRING,
                             (UINT)subMenu, _("&Window"));
                break;
            }
        }

        if ( !success )
        {
            ::AppendMenu(hmenu, MF_POPUP, (UINT)subMenu, _("&Window"));
        }
    }

    MDISetMenu(win, hmenu, subMenu);
}

static void RemoveWindowMenu(wxWindow *win, WXHMENU menu)
{
    HMENU hMenu = (HMENU)menu;

    if ( hMenu )
    {
        wxChar buf[1024];

        int N = ::GetMenuItemCount(hMenu);
        for ( int i = 0; i < N; i++ )
        {
            if ( !::GetMenuString(hMenu, i, buf, WXSIZEOF(buf), MF_BYPOSITION) )
            {
                // Ignore successful read of menu string with length 0 which
                // occurs, for example, for a maximized MDI childs system menu
                if ( ::GetLastError() != 0 )
                {
                    wxLogLastError(wxT("GetMenuString"));
                }

                continue;
            }

            if ( wxStrcmp(buf, _("&Window")) == 0 )
            {
                if ( !::RemoveMenu(hMenu, i, MF_BYPOSITION) )
                {
                    wxLogLastError(wxT("RemoveMenu"));
                }

                break;
            }
        }
    }

    if ( win )
    {
        // we don't change the windows menu, but we update the main one
        MDISetMenu(win, hMenu, NULL);
    }
}

static void UnpackMDIActivate(WXWPARAM wParam, WXLPARAM lParam,
                              WXWORD *activate, WXHWND *hwndAct, WXHWND *hwndDeact)
{
    *activate = true;
    *hwndAct = (WXHWND)lParam;
    *hwndDeact = (WXHWND)wParam;
}

#endif // wxUSE_MDI && !defined(__WXUNIVERSAL__)
