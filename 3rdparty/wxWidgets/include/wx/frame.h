/////////////////////////////////////////////////////////////////////////////
// Name:        wx/frame.h
// Purpose:     wxFrame class interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.11.99
// RCS-ID:      $Id: frame.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FRAME_H_BASE_
#define _WX_FRAME_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/toplevel.h"      // the base class

// the default names for various classs
extern WXDLLEXPORT_DATA(const wxChar) wxStatusLineNameStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxToolBarNameStr[];

class WXDLLIMPEXP_FWD_CORE wxFrame;
class WXDLLIMPEXP_FWD_CORE wxMenuBar;
class WXDLLIMPEXP_FWD_CORE wxStatusBar;
class WXDLLIMPEXP_FWD_CORE wxToolBar;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// wxFrame-specific (i.e. not for wxDialog) styles
#define wxFRAME_NO_TASKBAR      0x0002  // No taskbar button (MSW only)
#define wxFRAME_TOOL_WINDOW     0x0004  // No taskbar button, no system menu
#define wxFRAME_FLOAT_ON_PARENT 0x0008  // Always above its parent
#define wxFRAME_SHAPED          0x0010  // Create a window that is able to be shaped

// ----------------------------------------------------------------------------
// wxFrame is a top-level window with optional menubar, statusbar and toolbar
//
// For each of *bars, a frame may have several of them, but only one is
// managed by the frame, i.e. resized/moved when the frame is and whose size
// is accounted for in client size calculations - all others should be taken
// care of manually. The CreateXXXBar() functions create this, main, XXXBar,
// but the actual creation is done in OnCreateXXXBar() functions which may be
// overridden to create custom objects instead of standard ones when
// CreateXXXBar() is called.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxFrameBase : public wxTopLevelWindow
{
public:
    // construction
    wxFrameBase();
    virtual ~wxFrameBase();

    wxFrame *New(wxWindow *parent,
                 wxWindowID winid,
                 const wxString& title,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_FRAME_STYLE,
                 const wxString& name = wxFrameNameStr);

    // frame state
    // -----------

    // get the origin of the client area (which may be different from (0, 0)
    // if the frame has a toolbar) in client coordinates
    virtual wxPoint GetClientAreaOrigin() const;

    // sends a size event to the window using its current size -- this has an
    // effect of refreshing the window layout
    virtual void SendSizeEvent();

    // menu bar functions
    // ------------------

#if wxUSE_MENUS
    virtual void SetMenuBar(wxMenuBar *menubar);
    virtual wxMenuBar *GetMenuBar() const { return m_frameMenuBar; }
#endif // wxUSE_MENUS

    // process menu command: returns true if processed
    bool ProcessCommand(int winid);

    // status bar functions
    // --------------------
#if wxUSE_STATUSBAR
    // create the main status bar by calling OnCreateStatusBar()
    virtual wxStatusBar* CreateStatusBar(int number = 1,
                                         long style = wxST_SIZEGRIP|wxFULL_REPAINT_ON_RESIZE,
                                         wxWindowID winid = 0,
                                         const wxString& name =
                                            wxStatusLineNameStr);
    // return a new status bar
    virtual wxStatusBar *OnCreateStatusBar(int number,
                                           long style,
                                           wxWindowID winid,
                                           const wxString& name);
    // get the main status bar
    virtual wxStatusBar *GetStatusBar() const { return m_frameStatusBar; }

    // sets the main status bar
    virtual void SetStatusBar(wxStatusBar *statBar);

    // forward these to status bar
    virtual void SetStatusText(const wxString &text, int number = 0);
    virtual void SetStatusWidths(int n, const int widths_field[]);
    void PushStatusText(const wxString &text, int number = 0);
    void PopStatusText(int number = 0);

    // set the status bar pane the help will be shown in
    void SetStatusBarPane(int n) { m_statusBarPane = n; }
    int GetStatusBarPane() const { return m_statusBarPane; }
#endif // wxUSE_STATUSBAR

    // toolbar functions
    // -----------------

#if wxUSE_TOOLBAR
    // create main toolbar bycalling OnCreateToolBar()
    virtual wxToolBar* CreateToolBar(long style = -1,
                                     wxWindowID winid = wxID_ANY,
                                     const wxString& name = wxToolBarNameStr);
    // return a new toolbar
    virtual wxToolBar *OnCreateToolBar(long style,
                                       wxWindowID winid,
                                       const wxString& name );

    // get/set the main toolbar
    virtual wxToolBar *GetToolBar() const { return m_frameToolBar; }
    virtual void SetToolBar(wxToolBar *toolbar);
#endif // wxUSE_TOOLBAR

    // implementation only from now on
    // -------------------------------

    // event handlers
#if wxUSE_MENUS
#if wxUSE_STATUSBAR
    void OnMenuOpen(wxMenuEvent& event);
    void OnMenuClose(wxMenuEvent& event);
    void OnMenuHighlight(wxMenuEvent& event);
#endif // wxUSE_STATUSBAR

    // send wxUpdateUIEvents for all menu items in the menubar,
    // or just for menu if non-NULL
    virtual void DoMenuUpdates(wxMenu* menu = NULL);
#endif // wxUSE_MENUS

    // do the UI update processing for this window
    virtual void UpdateWindowUI(long flags = wxUPDATE_UI_NONE);

    // Implement internal behaviour (menu updating on some platforms)
    virtual void OnInternalIdle();

    // if there is no real wxTopLevelWindow on this platform we have to define
    // some wxTopLevelWindowBase pure virtual functions here to avoid breaking
    // old ports (wxMotif) which don't define them in wxFrame
#ifndef wxTopLevelWindowNative
    virtual bool ShowFullScreen(bool WXUNUSED(show),
                                long WXUNUSED(style) = wxFULLSCREEN_ALL)
        { return false; }
    virtual bool IsFullScreen() const
        { return false; }
#endif // no wxTopLevelWindowNative

#if wxUSE_MENUS || wxUSE_TOOLBAR
    // show help text (typically in the statusbar); show is false
    // if you are hiding the help, true otherwise
    virtual void DoGiveHelp(const wxString& text, bool show);
#endif

protected:
    // the frame main menu/status/tool bars
    // ------------------------------------

    // this (non virtual!) function should be called from dtor to delete the
    // main menubar, statusbar and toolbar (if any)
    void DeleteAllBars();

    // test whether this window makes part of the frame
    virtual bool IsOneOfBars(const wxWindow *win) const;

#if wxUSE_MENUS
    // override to update menu bar position when the frame size changes
    virtual void PositionMenuBar() { }

    // override to do something special when the menu bar is being removed
    // from the frame
    virtual void DetachMenuBar();

    // override to do something special when the menu bar is attached to the
    // frame
    virtual void AttachMenuBar(wxMenuBar *menubar);

    wxMenuBar *m_frameMenuBar;
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR && (wxUSE_MENUS || wxUSE_TOOLBAR)
    // the saved status bar text overwritten by DoGiveHelp()
    wxString m_oldStatusText;
#endif

#if wxUSE_STATUSBAR
    // override to update status bar position (or anything else) when
    // something changes
    virtual void PositionStatusBar() { }

    // show the help string for this menu item in the given status bar: the
    // status bar pointer can be NULL; return true if help was shown
    bool ShowMenuHelp(wxStatusBar *statbar, int helpid);

    wxStatusBar *m_frameStatusBar;
#endif // wxUSE_STATUSBAR


    int m_statusBarPane;

#if wxUSE_TOOLBAR
    // override to update status bar position (or anything else) when
    // something changes
    virtual void PositionToolBar() { }

    wxToolBar *m_frameToolBar;
#endif // wxUSE_TOOLBAR

#if wxUSE_MENUS && wxUSE_STATUSBAR
    DECLARE_EVENT_TABLE()
#endif // wxUSE_MENUS && wxUSE_STATUSBAR

    DECLARE_NO_COPY_CLASS(wxFrameBase)
};

// include the real class declaration
#if defined(__WXUNIVERSAL__) // && !defined(__WXMICROWIN__)
    #include "wx/univ/frame.h"
#else // !__WXUNIVERSAL__
    #if defined(__WXPALMOS__)
        #include "wx/palmos/frame.h"
    #elif defined(__WXMSW__)
        #include "wx/msw/frame.h"
    #elif defined(__WXGTK20__)
        #include "wx/gtk/frame.h"
    #elif defined(__WXGTK__)
        #include "wx/gtk1/frame.h"
    #elif defined(__WXMOTIF__)
        #include "wx/motif/frame.h"
    #elif defined(__WXMAC__)
        #include "wx/mac/frame.h"
    #elif defined(__WXCOCOA__)
        #include "wx/cocoa/frame.h"
    #elif defined(__WXPM__)
        #include "wx/os2/frame.h"
    #endif
#endif

#endif
    // _WX_FRAME_H_BASE_
