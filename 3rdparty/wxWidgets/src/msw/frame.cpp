/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/frame.cpp
// Purpose:     wxFrame
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: frame.cpp 59086 2009-02-22 16:07:58Z CE $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/frame.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/app.h"
    #include "wx/menu.h"
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/settings.h"
    #include "wx/dcclient.h"
    #include "wx/mdi.h"
    #include "wx/panel.h"
    #include "wx/log.h"
    #include "wx/toolbar.h"
    #include "wx/statusbr.h"
    #include "wx/menuitem.h"
#endif // WX_PRECOMP

#include "wx/msw/private.h"

#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
    #include <ole2.h>
    #include <aygshell.h>
    #include "wx/msw/winundef.h"
#endif

#include "wx/generic/statusbr.h"

#ifdef __WXUNIVERSAL__
    #include "wx/univ/theme.h"
    #include "wx/univ/colschem.h"
#endif // __WXUNIVERSAL__

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

#if wxUSE_MENUS_NATIVE
    extern wxMenu *wxCurrentPopupMenu;
#endif // wxUSE_MENUS_NATIVE

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFrame, wxFrameBase)
    EVT_SYS_COLOUR_CHANGED(wxFrame::OnSysColourChanged)
END_EVENT_TABLE()

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxFrameStyle )

wxBEGIN_FLAGS( wxFrameStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

    // frame styles
    wxFLAGS_MEMBER(wxSTAY_ON_TOP)
    wxFLAGS_MEMBER(wxCAPTION)
#if WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxTHICK_FRAME)
#endif // WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxSYSTEM_MENU)
    wxFLAGS_MEMBER(wxRESIZE_BORDER)
#if WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxRESIZE_BOX)
#endif // WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxCLOSE_BOX)
    wxFLAGS_MEMBER(wxMAXIMIZE_BOX)
    wxFLAGS_MEMBER(wxMINIMIZE_BOX)

    wxFLAGS_MEMBER(wxFRAME_TOOL_WINDOW)
    wxFLAGS_MEMBER(wxFRAME_FLOAT_ON_PARENT)

    wxFLAGS_MEMBER(wxFRAME_SHAPED)

wxEND_FLAGS( wxFrameStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxFrame, wxTopLevelWindow,"wx/frame.h")

wxBEGIN_PROPERTIES_TABLE(wxFrame)
    wxEVENT_PROPERTY( Menu , wxEVT_COMMAND_MENU_SELECTED , wxCommandEvent)

    wxPROPERTY( Title,wxString, SetTitle, GetTitle, wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxFrameStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
    wxPROPERTY( MenuBar , wxMenuBar * , SetMenuBar , GetMenuBar , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxFrame)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxFrame , wxWindow* , Parent , wxWindowID , Id , wxString , Title , wxPoint , Position , wxSize , Size , long , WindowStyle)

#else
IMPLEMENT_DYNAMIC_CLASS(wxFrame, wxTopLevelWindow)
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// static class members
// ----------------------------------------------------------------------------

#if wxUSE_STATUSBAR
    #if wxUSE_NATIVE_STATUSBAR
        bool wxFrame::m_useNativeStatusBar = true;
    #else
        bool wxFrame::m_useNativeStatusBar = false;
    #endif
#endif // wxUSE_NATIVE_STATUSBAR

// ----------------------------------------------------------------------------
// creation/destruction
// ----------------------------------------------------------------------------

void wxFrame::Init()
{
#if wxUSE_MENUS
    m_hMenu = NULL;
#endif // wxUSE_MENUS

#if wxUSE_TOOLTIPS
    m_hwndToolTip = 0;
#endif

    m_wasMinimized = false;
}

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

#if defined(__SMARTPHONE__)
    SetLeftMenu(wxID_EXIT, _("Done"));
#endif

#if wxUSE_ACCEL && defined(__POCKETPC__)
    // The guidelines state that Ctrl+Q should quit the app.
    // Let's define an accelerator table to send wxID_EXIT.
    wxAcceleratorEntry entries[1];
    entries[0].Set(wxACCEL_CTRL,   'Q',         wxID_EXIT);
    wxAcceleratorTable accel(1, entries);
    SetAcceleratorTable(accel);
#endif // wxUSE_ACCEL && __POCKETPC__

    return true;
}

wxFrame::~wxFrame()
{
    m_isBeingDeleted = true;
    DeleteAllBars();
}

// ----------------------------------------------------------------------------
// wxFrame client size calculations
// ----------------------------------------------------------------------------

void wxFrame::DoSetClientSize(int width, int height)
{
    // leave enough space for the status bar if we have (and show) it
#if wxUSE_STATUSBAR
    wxStatusBar *statbar = GetStatusBar();
    if ( statbar && statbar->IsShown() )
    {
        height += statbar->GetSize().y;
    }
#endif // wxUSE_STATUSBAR

    // call GetClientAreaOrigin() to take the toolbar into account
    wxPoint pt = GetClientAreaOrigin();
    width += pt.x;
    height += pt.y;

#if wxUSE_TOOLBAR
    wxToolBar * const toolbar = GetToolBar();
    if ( toolbar )
    {
        if ( toolbar->HasFlag(wxTB_RIGHT | wxTB_BOTTOM) )
        {
            const wxSize sizeTB = toolbar->GetSize();
            if ( toolbar->HasFlag(wxTB_RIGHT) )
                width -= sizeTB.x;
            else // wxTB_BOTTOM
                height -= sizeTB.y;
        }
        //else: toolbar already taken into account by GetClientAreaOrigin()
    }
#endif // wxUSE_TOOLBAR

    wxTopLevelWindow::DoSetClientSize(width, height);
}

// Get size *available for subwindows* i.e. excluding menu bar, toolbar etc.
void wxFrame::DoGetClientSize(int *x, int *y) const
{
    wxTopLevelWindow::DoGetClientSize(x, y);

    // account for the possible toolbar
    wxPoint pt = GetClientAreaOrigin();
    if ( x )
        *x -= pt.x;

    if ( y )
        *y -= pt.y;

#if wxUSE_TOOLBAR
    wxToolBar * const toolbar = GetToolBar();
    if ( toolbar )
    {
        if ( toolbar->HasFlag(wxTB_RIGHT | wxTB_BOTTOM) )
        {
            const wxSize sizeTB = toolbar->GetSize();
            if ( toolbar->HasFlag(wxTB_RIGHT) )
            {
                if ( x )
                    *x -= sizeTB.x;
            }
            else // wxTB_BOTTOM
            {
                if ( y )
                    *y -= sizeTB.y;
            }
        }
        //else: toolbar already taken into account by GetClientAreaOrigin()
    }
#endif // wxUSE_TOOLBAR

#if wxUSE_STATUSBAR
    // adjust client area height to take the status bar into account
    if ( y )
    {
        wxStatusBar *statbar = GetStatusBar();
        if ( statbar && statbar->IsShown() )
        {
            *y -= statbar->GetClientSize().y;
        }
    }
#endif // wxUSE_STATUSBAR
}

// ----------------------------------------------------------------------------
// wxFrame: various geometry-related functions
// ----------------------------------------------------------------------------

void wxFrame::Raise()
{
    ::SetForegroundWindow(GetHwnd());
}

// generate an artificial resize event
void wxFrame::SendSizeEvent()
{
    if ( !m_iconized )
    {
        RECT r = wxGetWindowRect(GetHwnd());

        (void)::PostMessage(GetHwnd(), WM_SIZE,
                            IsMaximized() ? SIZE_MAXIMIZED : SIZE_RESTORED,
                            MAKELPARAM(r.right - r.left, r.bottom - r.top));
    }
}

#if wxUSE_STATUSBAR
wxStatusBar *wxFrame::OnCreateStatusBar(int number,
                                        long style,
                                        wxWindowID id,
                                        const wxString& name)
{
    wxStatusBar *statusBar wxDUMMY_INITIALIZE(NULL);

#if wxUSE_NATIVE_STATUSBAR
    if ( !UsesNativeStatusBar() )
    {
        statusBar = (wxStatusBar *)new wxStatusBarGeneric(this, id, style);
    }
    else
#endif
    {
        statusBar = new wxStatusBar(this, id, style, name);
    }

    statusBar->SetFieldsCount(number);

    return statusBar;
}

void wxFrame::PositionStatusBar()
{
    if ( !m_frameStatusBar || !m_frameStatusBar->IsShown() )
        return;

    int w, h;
    GetClientSize(&w, &h);

    int sw, sh;
    m_frameStatusBar->GetSize(&sw, &sh);

    int x = 0;
#if wxUSE_TOOLBAR
    wxToolBar * const toolbar = GetToolBar();
    if ( toolbar && !toolbar->HasFlag(wxTB_TOP) )
    {
        const wxSize sizeTB = toolbar->GetSize();

        if ( toolbar->HasFlag(wxTB_LEFT | wxTB_RIGHT) )
        {
            if ( toolbar->HasFlag(wxTB_LEFT) )
                x -= sizeTB.x;

            w += sizeTB.x;
        }
        else // wxTB_BOTTOM
        {
            // we need to position the status bar below the toolbar
            h += sizeTB.y;
        }
    }
    //else: no adjustments necessary for the toolbar on top
#endif // wxUSE_TOOLBAR

    // Since we wish the status bar to be directly under the client area,
    // we use the adjusted sizes without using wxSIZE_NO_ADJUSTMENTS.
    m_frameStatusBar->SetSize(x, h, w, sh);
}

#endif // wxUSE_STATUSBAR

#if wxUSE_MENUS_NATIVE

void wxFrame::AttachMenuBar(wxMenuBar *menubar)
{
#if defined(__SMARTPHONE__) && defined(__WXWINCE__)

    wxMenu *autoMenu = NULL;

    if( menubar->GetMenuCount() == 1 )
    {
        autoMenu = wxTopLevelWindowMSW::ButtonMenu::DuplicateMenu(menubar->GetMenu(0));
        SetRightMenu(wxID_ANY, menubar->GetLabelTop(0), autoMenu);
    }
    else
    {
        autoMenu = new wxMenu;

        for( size_t n = 0; n < menubar->GetMenuCount(); n++ )
        {
            wxMenu *item = menubar->GetMenu(n);
            wxString label = menubar->GetLabelTop(n);
            wxMenu *new_item = wxTopLevelWindowMSW::ButtonMenu::DuplicateMenu(item);
            autoMenu->Append(wxID_ANY, label, new_item);
        }

        SetRightMenu(wxID_ANY, _("Menu"), autoMenu);
    }

#elif defined(WINCE_WITHOUT_COMMANDBAR)
    if (!GetToolBar())
    {
        wxToolMenuBar* toolBar = new wxToolMenuBar(this, wxID_ANY,
                         wxDefaultPosition, wxDefaultSize,
                         wxBORDER_NONE | wxTB_HORIZONTAL,
                         wxToolBarNameStr, menubar);
        SetToolBar(toolBar);
        menubar->SetToolBar(toolBar);
    }

    // When the main window is created using CW_USEDEFAULT the height of the
    // menubar is not taken into account, so we resize it afterwards if a
    // menubar is present
    HWND hwndMenuBar = SHFindMenuBar(GetHwnd());
    if ( hwndMenuBar )
    {
        RECT mbRect;
        ::GetWindowRect(hwndMenuBar, &mbRect);
        const int menuHeight = mbRect.bottom - mbRect.top;

        RECT rc;
        ::GetWindowRect(GetHwnd(), &rc);
        // adjust for menu / titlebar height
        rc.bottom -= (2*menuHeight-1);

        ::MoveWindow(GetHwnd(), rc.left, rc.top, rc.right, rc.bottom, FALSE);
    }
#endif

    wxFrameBase::AttachMenuBar(menubar);

    if ( !menubar )
    {
        // actually remove the menu from the frame
        m_hMenu = (WXHMENU)0;
        InternalSetMenuBar();
    }
    else // set new non NULL menu bar
    {
#if !defined(__WXWINCE__) || defined(WINCE_WITH_COMMANDBAR)
        // Can set a menubar several times.
        if ( menubar->GetHMenu() )
        {
            m_hMenu = menubar->GetHMenu();
        }
        else // no HMENU yet
        {
            m_hMenu = menubar->Create();

            if ( !m_hMenu )
            {
                wxFAIL_MSG( _T("failed to create menu bar") );
                return;
            }
        }
#endif
        InternalSetMenuBar();
    }
}

void wxFrame::InternalSetMenuBar()
{
#if defined(__WXMICROWIN__) || defined(__WXWINCE__)
    // Nothing
#else
    if ( !::SetMenu(GetHwnd(), (HMENU)m_hMenu) )
    {
        wxLogLastError(wxT("SetMenu"));
    }
#endif
}

#endif // wxUSE_MENUS_NATIVE

// Responds to colour changes, and passes event on to children.
void wxFrame::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    SetOwnBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
    Refresh();

#if wxUSE_STATUSBAR
    if ( m_frameStatusBar )
    {
        wxSysColourChangedEvent event2;
        event2.SetEventObject( m_frameStatusBar );
        m_frameStatusBar->GetEventHandler()->ProcessEvent(event2);
    }
#endif // wxUSE_STATUSBAR

    // Propagate the event to the non-top-level children
    wxWindow::OnSysColourChanged(event);
}

// Pass true to show full screen, false to restore.
bool wxFrame::ShowFullScreen(bool show, long style)
{
    // TODO-CE: add support for CE
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    if ( IsFullScreen() == show )
        return false;

    if (show)
    {
        // zap the toolbar, menubar, and statusbar if needed
        //
        // TODO: hide commandbar for WINCE_WITH_COMMANDBAR
#if wxUSE_TOOLBAR
        wxToolBar *theToolBar = GetToolBar();

        if ((style & wxFULLSCREEN_NOTOOLBAR) && theToolBar)
        {
            if ( theToolBar->IsShown() )
            {
                theToolBar->SetSize(wxDefaultCoord,0);
                theToolBar->Show(false);
            }
            else // prevent it from being restored later
            {
                style &= ~wxFULLSCREEN_NOTOOLBAR;
            }
        }
#endif // wxUSE_TOOLBAR

        if (style & wxFULLSCREEN_NOMENUBAR)
            SetMenu((HWND)GetHWND(), (HMENU) NULL);

#if wxUSE_STATUSBAR
        wxStatusBar *theStatusBar = GetStatusBar();

        // Save the number of fields in the statusbar
        if ((style & wxFULLSCREEN_NOSTATUSBAR) && theStatusBar)
        {
            if ( theStatusBar->IsShown() )
                theStatusBar->Show(false);
            else
                style &= ~wxFULLSCREEN_NOSTATUSBAR;
        }
#endif // wxUSE_STATUSBAR
    }
    else // restore to normal
    {
        // restore the toolbar, menubar, and statusbar if we had hid them
#if wxUSE_TOOLBAR
        wxToolBar *theToolBar = GetToolBar();

        if ((m_fsStyle & wxFULLSCREEN_NOTOOLBAR) && theToolBar)
        {
            theToolBar->Show(true);
        }
#endif // wxUSE_TOOLBAR

        if (m_fsStyle & wxFULLSCREEN_NOMENUBAR)
        {
            WXHMENU menu = m_hMenu;

#if wxUSE_MDI_ARCHITECTURE
            wxMDIParentFrame *frame = wxDynamicCast(this, wxMDIParentFrame);
            if (frame)
            {
                wxMDIChildFrame *child = frame->GetActiveChild();
                if (child)
                {
                    menu = child->GetWinMenu();
                }
            }
#endif // wxUSE_MDI_ARCHITECTURE

            if (menu)
            {
                ::SetMenu(GetHwnd(), (HMENU)menu);
            }
        }

#if wxUSE_STATUSBAR
        wxStatusBar *theStatusBar = GetStatusBar();

        if ((m_fsStyle & wxFULLSCREEN_NOSTATUSBAR) && theStatusBar)
        {
            theStatusBar->Show(true);
            PositionStatusBar();
        }
#endif // wxUSE_STATUSBAR
    }
#endif // !defined(__WXMICROWIN__) && !defined(__WXWINCE__)

    return wxFrameBase::ShowFullScreen(show, style);
}

// ----------------------------------------------------------------------------
// tool/status bar stuff
// ----------------------------------------------------------------------------

#if wxUSE_TOOLBAR

wxToolBar* wxFrame::CreateToolBar(long style, wxWindowID id, const wxString& name)
{
#if defined(WINCE_WITHOUT_COMMANDBAR)
    // We may already have a toolbar from calling SetMenuBar.
    if (GetToolBar())
        return GetToolBar();
#endif
    if ( wxFrameBase::CreateToolBar(style, id, name) )
    {
        PositionToolBar();
    }

    return m_frameToolBar;
}

void wxFrame::PositionToolBar()
{
    // TODO: we want to do something different in WinCE, because the toolbar
    //       should be associated with the commandbar, instead of being
    //       independent window.
#if !defined(WINCE_WITHOUT_COMMANDBAR)
    wxToolBar *toolbar = GetToolBar();
    if ( toolbar && toolbar->IsShown() )
    {
        // don't call our (or even wxTopLevelWindow) version because we want
        // the real (full) client area size, not excluding the tool/status bar
        int width, height;
        wxWindow::DoGetClientSize(&width, &height);

#if wxUSE_STATUSBAR
        wxStatusBar *statbar = GetStatusBar();
        if ( statbar && statbar->IsShown() )
        {
            height -= statbar->GetClientSize().y;
        }
#endif // wxUSE_STATUSBAR

        int tx, ty, tw, th;
        toolbar->GetPosition( &tx, &ty );
        toolbar->GetSize( &tw, &th );

        int x = 0, y = 0;
        if ( toolbar->HasFlag(wxTB_BOTTOM) )
        {
            x = 0;
            y = height - th;
        }
        else if ( toolbar->HasFlag(wxTB_RIGHT) )
        {
            x = width - tw;
            y = 0;
        }
        else // left or top
        {
            x = 0;
            y = 0;
        }

#if defined(WINCE_WITH_COMMANDBAR)
        // We're using a commandbar - so we have to allow for it.
        if (GetMenuBar() && GetMenuBar()->GetCommandBar())
        {
            RECT rect;
            ::GetWindowRect((HWND) GetMenuBar()->GetCommandBar(), &rect);
            y = rect.bottom - rect.top;
        }
#endif // WINCE_WITH_COMMANDBAR

        if ( toolbar->HasFlag(wxTB_BOTTOM) )
        {
            if ( ty < 0 && ( -ty == th ) )
                ty = height - th;
            if ( tx < 0 && (-tx == tw ) )
                tx = 0;
        }
        else if ( toolbar->HasFlag(wxTB_RIGHT) )
        {
            if( ty < 0 && ( -ty == th ) )
                ty = 0;
            if( tx < 0 && ( -tx == tw ) )
                tx = width - tw;
        }
        else // left or top
        {
            if (ty < 0 && (-ty == th))
                ty = 0;
            if (tx < 0 && (-tx == tw))
                tx = 0;
        }

        int desiredW = tw;
        int desiredH = th;

        if ( toolbar->IsVertical() )
        {
            desiredH = height;
        }
        else
        {
            desiredW = width;
        }

        // use the 'real' MSW position here, don't offset relativly to the
        // client area origin

        // Optimise such that we don't have to always resize the toolbar
        // when the frame changes, otherwise we'll get a lot of flicker.
        bool heightChanging wxDUMMY_INITIALIZE(true);
        bool widthChanging wxDUMMY_INITIALIZE(true);

        if ( toolbar->IsVertical() )
        {
            // It's OK if the current height is greater than what can be shown.
            heightChanging = (desiredH > th) ;
            widthChanging = (desiredW != tw) ;

            // The next time around, we may not have to set the size
            if (heightChanging)
                desiredH = desiredH + 200;
        }
        else
        {
            // It's OK if the current width is greater than what can be shown.
            widthChanging = (desiredW > tw) ;
            heightChanging = (desiredH != th) ;

            // The next time around, we may not have to set the size
            if (widthChanging)
                desiredW = desiredW + 200;
        }

        if (tx != 0 || ty != 0 || widthChanging || heightChanging)
            toolbar->SetSize(x, y, desiredW, desiredH, wxSIZE_NO_ADJUSTMENTS);

    }
#endif // !WINCE_WITH_COMMANDBAR
}

#endif // wxUSE_TOOLBAR

// ----------------------------------------------------------------------------
// frame state (iconized/maximized/...)
// ----------------------------------------------------------------------------

// propagate our state change to all child frames: this allows us to emulate X
// Windows behaviour where child frames float independently of the parent one
// on the desktop, but are iconized/restored with it
void wxFrame::IconizeChildFrames(bool bIconize)
{
    m_iconized = bIconize;

    for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxWindow *win = node->GetData();

        // iconizing the frames with this style under Win95 shell puts them at
        // the bottom of the screen (as the MDI children) instead of making
        // them appear in the taskbar because they are, by virtue of this
        // style, not managed by the taskbar - instead leave Windows take care
        // of them
        if ( win->GetWindowStyle() & wxFRAME_TOOL_WINDOW )
            continue;

        // the child MDI frames are a special case and should not be touched by
        // the parent frame - instead, they are managed by the user
        wxFrame *frame = wxDynamicCast(win, wxFrame);
        if ( frame
#if wxUSE_MDI_ARCHITECTURE
                && !frame->IsMDIChild()
#endif // wxUSE_MDI_ARCHITECTURE
           )
        {
            // we don't want to restore the child frames which had been
            // iconized even before we were iconized, so save the child frame
            // status when iconizing the parent frame and check it when
            // restoring it
            if ( bIconize )
            {
                frame->m_wasMinimized = frame->IsIconized();
            }

            // note that we shouldn't touch the hidden frames neither because
            // iconizing/restoring them would show them as a side effect
            if ( !frame->m_wasMinimized && frame->IsShown() )
                frame->Iconize(bIconize);
        }
    }
}

WXHICON wxFrame::GetDefaultIcon() const
{
    // we don't have any standard icons (any more)
    return (WXHICON)0;
}

// ===========================================================================
// message processing
// ===========================================================================

// ---------------------------------------------------------------------------
// preprocessing
// ---------------------------------------------------------------------------

bool wxFrame::MSWDoTranslateMessage(wxFrame *frame, WXMSG *pMsg)
{
    if ( wxWindow::MSWTranslateMessage(pMsg) )
        return true;

#if wxUSE_MENUS && wxUSE_ACCEL && !defined(__WXUNIVERSAL__)
    // try the menu bar accels
    wxMenuBar *menuBar = GetMenuBar();
    if ( menuBar )
    {
        const wxAcceleratorTable& acceleratorTable = menuBar->GetAccelTable();
        return acceleratorTable.Translate(frame, pMsg);
    }
#endif // wxUSE_MENUS && wxUSE_ACCEL

    return false;
}

// ---------------------------------------------------------------------------
// our private (non virtual) message handlers
// ---------------------------------------------------------------------------

bool wxFrame::HandlePaint()
{
    RECT rect;
    if ( ::GetUpdateRect(GetHwnd(), &rect, FALSE) )
    {
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
        if ( m_iconized )
        {
            const wxIcon& icon = GetIcon();
            HICON hIcon = icon.Ok() ? GetHiconOf(icon)
                                    : (HICON)GetDefaultIcon();

            // Hold a pointer to the dc so long as the OnPaint() message
            // is being processed
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(GetHwnd(), &ps);

            // Erase background before painting or we get white background
            MSWDefWindowProc(WM_ICONERASEBKGND, (WORD)(LONG)ps.hdc, 0L);

            if ( hIcon )
            {
                RECT rect;
                ::GetClientRect(GetHwnd(), &rect);

                // FIXME: why hardcoded?
                static const int icon_width = 32;
                static const int icon_height = 32;

                int icon_x = (int)((rect.right - icon_width)/2);
                int icon_y = (int)((rect.bottom - icon_height)/2);

                ::DrawIcon(hdc, icon_x, icon_y, hIcon);
            }

            ::EndPaint(GetHwnd(), &ps);

            return true;
        }
        else
 #endif
        {
            return wxWindow::HandlePaint();
        }
    }
    else
    {
        // nothing to paint - processed
        return true;
    }
}

bool wxFrame::HandleSize(int WXUNUSED(x), int WXUNUSED(y), WXUINT id)
{
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    switch ( id )
    {
        case SIZE_RESTORED:
        case SIZE_MAXIMIZED:
            // only do it it if we were iconized before, otherwise resizing the
            // parent frame has a curious side effect of bringing it under it's
            // children
            if ( !m_iconized )
                break;

            // restore all child frames too
            IconizeChildFrames(false);

            (void)SendIconizeEvent(false);
            break;

        case SIZE_MINIMIZED:
            // iconize all child frames too
            IconizeChildFrames(true);
            break;
    }
#else
    wxUnusedVar(id);
#endif // !__WXWINCE__

    if ( !m_iconized )
    {
#if wxUSE_STATUSBAR
        PositionStatusBar();
#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR
        PositionToolBar();
#endif // wxUSE_TOOLBAR

#if defined(WINCE_WITH_COMMANDBAR)
        // Position the menu command bar
        if (GetMenuBar() && GetMenuBar()->GetCommandBar())
        {
            RECT rect;
            ::GetWindowRect((HWND) GetMenuBar()->GetCommandBar(), &rect);
            wxSize clientSz = GetClientSize();

            if ( !::MoveWindow((HWND) GetMenuBar()->GetCommandBar(), 0, 0, clientSz.x, rect.bottom - rect.top, true ) )
            {
                wxLogLastError(wxT("MoveWindow"));
            }

        }
#endif // WINCE_WITH_COMMANDBAR
    }

    // call the base class version to generate the appropriate events
    return false;
}

bool wxFrame::HandleCommand(WXWORD id, WXWORD cmd, WXHWND control)
{
    if ( control )
    {
        // In case it's e.g. a toolbar.
        wxWindow *win = wxFindWinFromHandle(control);
        if ( win )
            return win->MSWCommand(cmd, id);
    }

    // handle here commands from menus and accelerators for our menu bar items,
    // all the rest is handled by wxWindow itself
    if ( !control && (cmd == 0 /* menu */ || cmd == 1 /* accel */) )
    {
#if wxUSE_MENUS_NATIVE
        if ( !wxCurrentPopupMenu )
#endif // wxUSE_MENUS_NATIVE
        {
            if ( GetMenuBar() && ProcessCommand(id) )
                return true;
        }
    }

    return wxFrameBase::HandleCommand(id, cmd, control);
}

bool wxFrame::HandleMenuSelect(WXWORD nItem, WXWORD flags, WXHMENU hMenu)
{
    int item;
    if ( flags == 0xFFFF && hMenu == 0 )
    {
        // menu was removed from screen
        item = -1;
    }
#ifndef __WXMICROWIN__
    else if ( !(flags & MF_POPUP) && !(flags & MF_SEPARATOR) )
    {
        item = nItem;
    }
#endif
    else
    {
        // don't give hints for separators (doesn't make sense) nor for the
        // items opening popup menus (they don't have them anyhow) but do clear
        // the status line - otherwise, we would be left with the help message
        // for the previous item which doesn't apply any more
        DoGiveHelp(wxEmptyString, false);

        return false;
    }

    wxMenuEvent event(wxEVT_MENU_HIGHLIGHT, item);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxFrame::HandleMenuLoop(const wxEventType& evtType, WXWORD isPopup)
{
    // we don't have the menu id here, so we use the id to specify if the event
    // was from a popup menu or a normal one
    wxMenuEvent event(evtType, isPopup ? -1 : 0);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

// ---------------------------------------------------------------------------
// the window proc for wxFrame
// ---------------------------------------------------------------------------

WXLRESULT wxFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    WXLRESULT rc = 0;
    bool processed = false;

    switch ( message )
    {
        case WM_CLOSE:
            // if we can't close, tell the system that we processed the
            // message - otherwise it would close us
            processed = !Close();
            break;

        case WM_SIZE:
            processed = HandleSize(LOWORD(lParam), HIWORD(lParam), wParam);
            break;

        case WM_COMMAND:
            {
                WORD id, cmd;
                WXHWND hwnd;
                UnpackCommand((WXWPARAM)wParam, (WXLPARAM)lParam,
                              &id, &hwnd, &cmd);

                HandleCommand(id, cmd, (WXHWND)hwnd);

                // don't pass WM_COMMAND to the base class as it would generate
                // another wxCommandEvent which would result in its handler
                // being called twice if it uses event.Skip()
                processed = true;
            }
            break;

        case WM_PAINT:
            processed = HandlePaint();
            break;

        case WM_INITMENUPOPUP:
            processed = HandleInitMenuPopup((WXHMENU) wParam);
            break;

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
        case WM_MENUSELECT:
            {
                WXWORD item, flags;
                WXHMENU hmenu;
                UnpackMenuSelect(wParam, lParam, &item, &flags, &hmenu);

                processed = HandleMenuSelect(item, flags, hmenu);
            }
            break;

        case WM_EXITMENULOOP:
            processed = HandleMenuLoop(wxEVT_MENU_CLOSE, (WXWORD)wParam);
            break;

        case WM_QUERYDRAGICON:
            {
                const wxIcon& icon = GetIcon();
                HICON hIcon = icon.Ok() ? GetHiconOf(icon)
                                        : (HICON)GetDefaultIcon();
                rc = (long)hIcon;
                processed = rc != 0;
            }
            break;
#endif // !__WXMICROWIN__
    }

    if ( !processed )
        rc = wxFrameBase::MSWWindowProc(message, wParam, lParam);

    return rc;
}

// handle WM_INITMENUPOPUP message
bool wxFrame::HandleInitMenuPopup(WXHMENU hMenu)
{
    wxMenu* menu = NULL;
    if (GetMenuBar())
    {
        int nCount = GetMenuBar()->GetMenuCount();
        for (int n = 0; n < nCount; n++)
        {
            if (GetMenuBar()->GetMenu(n)->GetHMenu() == hMenu)
            {
                menu = GetMenuBar()->GetMenu(n);
                break;
            }
        }
    }

    wxMenuEvent event(wxEVT_MENU_OPEN, 0, menu);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// wxFrame size management: we exclude the areas taken by menu/status/toolbars
// from the client area, so the client area is what's really available for the
// frame contents
// ----------------------------------------------------------------------------

// get the origin of the client area in the client coordinates
wxPoint wxFrame::GetClientAreaOrigin() const
{
    wxPoint pt = wxTopLevelWindow::GetClientAreaOrigin();

#if wxUSE_TOOLBAR && !defined(__WXUNIVERSAL__) && \
  (!defined(__WXWINCE__) || (_WIN32_WCE >= 400 && !defined(__POCKETPC__) && !defined(__SMARTPHONE__)))
    wxToolBar * const toolbar = GetToolBar();
    if ( toolbar && toolbar->IsShown() )
    {
        const wxSize sizeTB = toolbar->GetSize();

        if ( toolbar->HasFlag(wxTB_TOP) )
        {
            pt.y += sizeTB.y;
        }
        else if ( toolbar->HasFlag(wxTB_LEFT) )
        {
            pt.x += sizeTB.x;
        }
    }
#endif // wxUSE_TOOLBAR

#if defined(WINCE_WITH_COMMANDBAR)
    if (GetMenuBar() && GetMenuBar()->GetCommandBar())
    {
        RECT rect;
        ::GetWindowRect((HWND) GetMenuBar()->GetCommandBar(), &rect);
        pt.y += (rect.bottom - rect.top);
    }
#endif

    return pt;
}
