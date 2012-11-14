/////////////////////////////////////////////////////////////////////////////
// Name:        wx/toplevel.h
// Purpose:     Top level window, abstraction of wxFrame and wxDialog
// Author:      Vaclav Slavik
// Id:          $Id: toplevel.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2001-2002 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


#ifndef __WX_UNIV_TOPLEVEL_H__
#define __WX_UNIV_TOPLEVEL_H__

#include "wx/univ/inpcons.h"
#include "wx/univ/inphand.h"
#include "wx/icon.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// frame decorations type flags used in wxRenderer and wxColourScheme
enum
{
    wxTOPLEVEL_ACTIVE          = 0x00000001,
    wxTOPLEVEL_MAXIMIZED       = 0x00000002,
    wxTOPLEVEL_TITLEBAR        = 0x00000004,
    wxTOPLEVEL_ICON            = 0x00000008,
    wxTOPLEVEL_RESIZEABLE      = 0x00000010,
    wxTOPLEVEL_BORDER          = 0x00000020,
    wxTOPLEVEL_BUTTON_CLOSE    = 0x01000000,
    wxTOPLEVEL_BUTTON_MAXIMIZE = 0x02000000,
    wxTOPLEVEL_BUTTON_ICONIZE =  0x04000000,
    wxTOPLEVEL_BUTTON_RESTORE  = 0x08000000,
    wxTOPLEVEL_BUTTON_HELP     = 0x10000000
};

// frame hit test return values:
enum
{
    wxHT_TOPLEVEL_NOWHERE         = 0x00000000,
    wxHT_TOPLEVEL_CLIENT_AREA     = 0x00000001,
    wxHT_TOPLEVEL_ICON            = 0x00000002,
    wxHT_TOPLEVEL_TITLEBAR        = 0x00000004,

    wxHT_TOPLEVEL_BORDER_N        = 0x00000010,
    wxHT_TOPLEVEL_BORDER_S        = 0x00000020,
    wxHT_TOPLEVEL_BORDER_E        = 0x00000040,
    wxHT_TOPLEVEL_BORDER_W        = 0x00000080,
    wxHT_TOPLEVEL_BORDER_NE       = wxHT_TOPLEVEL_BORDER_N | wxHT_TOPLEVEL_BORDER_E,
    wxHT_TOPLEVEL_BORDER_SE       = wxHT_TOPLEVEL_BORDER_S | wxHT_TOPLEVEL_BORDER_E,
    wxHT_TOPLEVEL_BORDER_NW       = wxHT_TOPLEVEL_BORDER_N | wxHT_TOPLEVEL_BORDER_W,
    wxHT_TOPLEVEL_BORDER_SW       = wxHT_TOPLEVEL_BORDER_S | wxHT_TOPLEVEL_BORDER_W,
    wxHT_TOPLEVEL_ANY_BORDER      = 0x000000F0,

    wxHT_TOPLEVEL_BUTTON_CLOSE    = /*0x01000000*/ wxTOPLEVEL_BUTTON_CLOSE,
    wxHT_TOPLEVEL_BUTTON_MAXIMIZE = /*0x02000000*/ wxTOPLEVEL_BUTTON_MAXIMIZE,
    wxHT_TOPLEVEL_BUTTON_ICONIZE =  /*0x04000000*/ wxTOPLEVEL_BUTTON_ICONIZE,
    wxHT_TOPLEVEL_BUTTON_RESTORE  = /*0x08000000*/ wxTOPLEVEL_BUTTON_RESTORE,
    wxHT_TOPLEVEL_BUTTON_HELP     = /*0x10000000*/ wxTOPLEVEL_BUTTON_HELP,
    wxHT_TOPLEVEL_ANY_BUTTON      =   0x1F000000
};

// Flags for interactive frame manipulation functions (only in wxUniversal):
enum
{
    wxINTERACTIVE_MOVE           = 0x00000001,
    wxINTERACTIVE_RESIZE         = 0x00000002,
    wxINTERACTIVE_RESIZE_S       = 0x00000010,
    wxINTERACTIVE_RESIZE_N       = 0x00000020,
    wxINTERACTIVE_RESIZE_W       = 0x00000040,
    wxINTERACTIVE_RESIZE_E       = 0x00000080,
    wxINTERACTIVE_WAIT_FOR_INPUT = 0x10000000
};

// ----------------------------------------------------------------------------
// the actions supported by this control
// ----------------------------------------------------------------------------

#define wxACTION_TOPLEVEL_ACTIVATE       wxT("activate")   // (de)activate the frame
#define wxACTION_TOPLEVEL_BUTTON_PRESS   wxT("pressbtn")   // press titlebar btn
#define wxACTION_TOPLEVEL_BUTTON_RELEASE wxT("releasebtn") // press titlebar btn
#define wxACTION_TOPLEVEL_BUTTON_CLICK   wxT("clickbtn")   // press titlebar btn
#define wxACTION_TOPLEVEL_MOVE           wxT("move")       // move the frame
#define wxACTION_TOPLEVEL_RESIZE         wxT("resize")     // resize the frame

//-----------------------------------------------------------------------------
// wxTopLevelWindow
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxTopLevelWindow : public wxTopLevelWindowNative,
                                     public wxInputConsumer
{
public:
    // construction
    wxTopLevelWindow() { Init(); }
    wxTopLevelWindow(wxWindow *parent,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE,
                     const wxString& name = wxFrameNameStr)
    {
        Init();

        Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    // wxUniv-specific methods: do [not] use native decorations for this (or
    // all) window(s)
    //
    // notice that this has no effect if the system doesn't support any native
    // decorations anyhow and that by default native decorations are used
    //
    // if UseNativeDecorations() is used, it must be called before Create()
    static void UseNativeDecorationsByDefault(bool native = true);
    void UseNativeDecorations(bool native = true);
    bool IsUsingNativeDecorations() const;


    // implement base class pure virtuals
    virtual bool ShowFullScreen(bool show, long style = wxFULLSCREEN_ALL);
    virtual wxPoint GetClientAreaOrigin() const;
    virtual void SetIcon(const wxIcon& icon) { SetIcons( wxIconBundle( icon ) ); }
    virtual void SetIcons(const wxIconBundle& icons);

    // implementation from now on
    // --------------------------

    // tests for frame's part at given point
    long HitTest(const wxPoint& pt) const;

    virtual bool PerformAction(const wxControlAction& action,
                               long numArg = -1,
                               const wxString& strArg = wxEmptyString);

    static wxInputHandler *GetStdInputHandler(wxInputHandler *handlerDef);
    virtual wxInputHandler *DoGetStdInputHandler(wxInputHandler *handlerDef)
    {
        return GetStdInputHandler(handlerDef);
    }

    // move/resize the frame interactively, i.e. let the user do it
    virtual void InteractiveMove(int flags = wxINTERACTIVE_MOVE);

    virtual wxSize GetMinSize() const;

    virtual wxWindow *GetInputWindow() const { return wx_const_cast(wxTopLevelWindow*, this); }

protected:
    virtual void DoGetClientSize(int *width, int *height) const;
    virtual void DoSetClientSize(int width, int height);

    // handle titlebar button click event
    virtual void ClickTitleBarButton(long button);

    // return wxTOPLEVEL_xxx combination based on current state of the frame
    long GetDecorationsStyle() const;

    // common part of all ctors
    void Init();

    void RefreshTitleBar();
    void OnNcPaint(wxNcPaintEvent& event);
    void OnSystemMenu(wxCommandEvent& event);

    // true if wxTLW should render decorations (aka titlebar) itself
    static int ms_drawDecorations;
    // true if wxTLW can be iconized
    static int ms_canIconize;

    // true if we're using native decorations
    bool m_usingNativeDecorations;
    // true for currently active frame
    bool m_isActive;
    // version of icon for titlebar (16x16)
    wxIcon m_titlebarIcon;
    // saved window style in fullscreen mdoe
    long m_fsSavedStyle;
    // currently pressed titlebar button
    long m_pressedButton;

    DECLARE_DYNAMIC_CLASS(wxTopLevelWindow)
    DECLARE_EVENT_TABLE()
    WX_DECLARE_INPUT_CONSUMER()
};

#endif // __WX_UNIV_TOPLEVEL_H__
