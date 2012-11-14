/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/mdig.h
// Purpose:     Generic MDI (Multiple Document Interface) classes
// Author:      Hans Van Leemputten
// Modified by:
// Created:     29/07/2002
// RCS-ID:      $Id: mdig.h 41223 2006-09-14 17:36:18Z PC $
// Copyright:   (c) Hans Van Leemputten
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MDIG_H_
#define _WX_MDIG_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/frame.h"
#include "wx/panel.h"
#include "wx/notebook.h"

extern WXDLLEXPORT_DATA(const wxChar) wxStatusLineNameStr[];


//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericMDIParentFrame;
class WXDLLEXPORT wxGenericMDIClientWindow;
class WXDLLEXPORT wxGenericMDIChildFrame;

//-----------------------------------------------------------------------------
// wxGenericMDIParentFrame
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericMDIParentFrame: public wxFrame
{
public:
    wxGenericMDIParentFrame();
    wxGenericMDIParentFrame(wxWindow *parent,
                     wxWindowID winid,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                     const wxString& name = wxFrameNameStr);

    virtual ~wxGenericMDIParentFrame();
    bool Create( wxWindow *parent,
                 wxWindowID winid,
                 const wxString& title,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                 const wxString& name = wxFrameNameStr );

#if wxUSE_MENUS
    wxMenu* GetWindowMenu() const { return m_pWindowMenu; };
    void SetWindowMenu(wxMenu* pMenu);

    virtual void SetMenuBar(wxMenuBar *pMenuBar);
#endif // wxUSE_MENUS

    void SetChildMenuBar(wxGenericMDIChildFrame *pChild);

    virtual bool ProcessEvent(wxEvent& event);

    wxGenericMDIChildFrame *GetActiveChild() const;
    inline void SetActiveChild(wxGenericMDIChildFrame* pChildFrame);

    wxGenericMDIClientWindow *GetClientWindow() const;
    virtual wxGenericMDIClientWindow *OnCreateClient();

    virtual void Cascade() { /* Has no effect */ }
    virtual void Tile(wxOrientation WXUNUSED(orient) = wxHORIZONTAL) { }
    virtual void ArrangeIcons() { /* Has no effect */ }
    virtual void ActivateNext();
    virtual void ActivatePrevious();

protected:
    wxGenericMDIClientWindow   *m_pClientWindow;
    wxGenericMDIChildFrame     *m_pActiveChild;
#if wxUSE_MENUS
    wxMenu              *m_pWindowMenu;
    wxMenuBar           *m_pMyMenuBar;
#endif // wxUSE_MENUS

protected:
    void Init();

#if wxUSE_MENUS
    void RemoveWindowMenu(wxMenuBar *pMenuBar);
    void AddWindowMenu(wxMenuBar *pMenuBar);

    void DoHandleMenu(wxCommandEvent &event);
#endif // wxUSE_MENUS

    virtual void DoGetClientSize(int *width, int *height) const;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericMDIParentFrame)
};

//-----------------------------------------------------------------------------
// wxGenericMDIChildFrame
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericMDIChildFrame: public wxPanel
{
public:
    wxGenericMDIChildFrame();
    wxGenericMDIChildFrame( wxGenericMDIParentFrame *parent,
                     wxWindowID winid,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE,
                     const wxString& name = wxFrameNameStr );

    virtual ~wxGenericMDIChildFrame();
    bool Create( wxGenericMDIParentFrame *parent,
                 wxWindowID winid,
                 const wxString& title,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_FRAME_STYLE,
                 const wxString& name = wxFrameNameStr );

#if wxUSE_MENUS
    virtual void SetMenuBar( wxMenuBar *menu_bar );
    virtual wxMenuBar *GetMenuBar() const;
#endif // wxUSE_MENUS

    virtual void SetTitle(const wxString& title);
    virtual wxString GetTitle() const;

    virtual void Activate();

#if wxUSE_STATUSBAR
    // no status bars
    virtual wxStatusBar* CreateStatusBar( int WXUNUSED(number) = 1,
                                        long WXUNUSED(style) = 1,
                                        wxWindowID WXUNUSED(winid) = 1,
                                        const wxString& WXUNUSED(name) = wxEmptyString)
      { return (wxStatusBar*)NULL; }

    virtual wxStatusBar *GetStatusBar() const { return (wxStatusBar*)NULL; }
    virtual void SetStatusText( const wxString &WXUNUSED(text), int WXUNUSED(number)=0 ) {}
    virtual void SetStatusWidths( int WXUNUSED(n), const int WXUNUSED(widths_field)[] ) {}
#endif

    // no size hints
    virtual void DoSetSizeHints( int WXUNUSED(minW),
                               int WXUNUSED(minH),
                               int WXUNUSED(maxW) = wxDefaultCoord,
                               int WXUNUSED(maxH) = wxDefaultCoord,
                               int WXUNUSED(incW) = wxDefaultCoord,
                               int WXUNUSED(incH) = wxDefaultCoord) {}

#if wxUSE_TOOLBAR
    // no toolbar bars
    virtual wxToolBar* CreateToolBar( long WXUNUSED(style),
                                       wxWindowID WXUNUSED(winid),
                                       const wxString& WXUNUSED(name) )
        { return (wxToolBar*)NULL; }
    virtual wxToolBar *GetToolBar() const { return (wxToolBar*)NULL; }
#endif

    // no icon
    void SetIcon( const wxIcon& WXUNUSED(icon) ) { }
    void SetIcons( const wxIconBundle& WXUNUSED(icons) ) { }

    // no maximize etc
    virtual void Maximize( bool WXUNUSED(maximize) = true) { /* Has no effect */ }
    virtual void Restore() { /* Has no effect */ }
    virtual void Iconize(bool WXUNUSED(iconize)  = true) { /* Has no effect */ }
    virtual bool IsMaximized() const { return true; }
    virtual bool IsIconized() const { return false; }
    virtual bool ShowFullScreen(bool WXUNUSED(show), long WXUNUSED(style)) { return false; }
    virtual bool IsFullScreen() const { return false; }

    virtual bool IsTopLevel() const { return false; }

    void OnMenuHighlight(wxMenuEvent& event);
    void OnActivate(wxActivateEvent& event);

    // The next 2 are copied from top level...
    void OnCloseWindow(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);

    void SetMDIParentFrame(wxGenericMDIParentFrame* parentFrame);
    wxGenericMDIParentFrame* GetMDIParentFrame() const;

protected:
    wxGenericMDIParentFrame *m_pMDIParentFrame;
    wxRect            m_MDIRect;
    wxString          m_Title;

#if wxUSE_MENUS
    wxMenuBar        *m_pMenuBar;
#endif // wxUSE_MENUS

protected:
    void Init();

    virtual void DoMoveWindow(int x, int y, int width, int height);

    // This function needs to be called when a size change is confirmed,
    // we needed this function to prevent any body from the outside
    // changing the panel... it messes the UI layout when we would allow it.
    void ApplyMDIChildFrameRect();

private:
    DECLARE_DYNAMIC_CLASS(wxGenericMDIChildFrame)
    DECLARE_EVENT_TABLE()

    friend class wxGenericMDIClientWindow;
};

//-----------------------------------------------------------------------------
// wxGenericMDIClientWindow
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericMDIClientWindow: public wxNotebook
{
public:
    wxGenericMDIClientWindow();
    wxGenericMDIClientWindow( wxGenericMDIParentFrame *parent, long style = 0 );
    virtual ~wxGenericMDIClientWindow();
    virtual bool CreateClient( wxGenericMDIParentFrame *parent, long style = wxVSCROLL | wxHSCROLL );

    virtual int SetSelection(size_t nPage);

protected:
    void PageChanged(int OldSelection, int newSelection);

    void OnPageChanged(wxNotebookEvent& event);
    void OnSize(wxSizeEvent& event);

private:
    DECLARE_DYNAMIC_CLASS(wxGenericMDIClientWindow)
    DECLARE_EVENT_TABLE()
};


/*
 * Define normal wxMDI classes based on wxGenericMDI
 */

#ifndef wxUSE_GENERIC_MDI_AS_NATIVE
#if defined(__WXUNIVERSAL__) || defined(__WXPM__) || defined(__WXCOCOA__)
#define wxUSE_GENERIC_MDI_AS_NATIVE   1
#else
#define wxUSE_GENERIC_MDI_AS_NATIVE   0
#endif
#endif // wxUSE_GENERIC_MDI_AS_NATIVE

#if wxUSE_GENERIC_MDI_AS_NATIVE

class wxMDIChildFrame ;

//-----------------------------------------------------------------------------
// wxMDIParentFrame
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxMDIParentFrame: public wxGenericMDIParentFrame
{
public:
    wxMDIParentFrame() {}
    wxMDIParentFrame(wxWindow *parent,
                     wxWindowID winid,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE | wxVSCROLL | wxHSCROLL,
                     const wxString& name = wxFrameNameStr)
         :wxGenericMDIParentFrame(parent, winid, title, pos, size, style, name)
     {
     }

    wxMDIChildFrame * GetActiveChild() const ;


private:
    DECLARE_DYNAMIC_CLASS(wxMDIParentFrame)
};

//-----------------------------------------------------------------------------
// wxMDIChildFrame
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxMDIChildFrame: public wxGenericMDIChildFrame
{
public:
    wxMDIChildFrame() {}

    wxMDIChildFrame( wxGenericMDIParentFrame *parent,
                     wxWindowID winid,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE,
                     const wxString& name = wxFrameNameStr )
         :wxGenericMDIChildFrame(parent, winid, title, pos, size, style, name)
     {
     }
private:
    DECLARE_DYNAMIC_CLASS(wxMDIChildFrame)
};

//-----------------------------------------------------------------------------
// wxMDIClientWindow
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxMDIClientWindow: public wxGenericMDIClientWindow
{
public:
    wxMDIClientWindow() {}

    wxMDIClientWindow( wxGenericMDIParentFrame *parent, long style = 0 )
        :wxGenericMDIClientWindow(parent, style)
    {
    }

private:
    DECLARE_DYNAMIC_CLASS(wxMDIClientWindow)
};

#endif

#endif
    // _WX_MDIG_H_
