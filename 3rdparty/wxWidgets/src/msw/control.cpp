/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/control.cpp
// Purpose:     wxControl class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: control.cpp 54424 2008-06-29 21:46:29Z VZ $
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

#if wxUSE_CONTROLS

#include "wx/control.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/event.h"
    #include "wx/app.h"
    #include "wx/dcclient.h"
    #include "wx/log.h"
    #include "wx/settings.h"
#endif

#if wxUSE_LISTCTRL
    #include "wx/listctrl.h"
#endif // wxUSE_LISTCTRL

#if wxUSE_TREECTRL
    #include "wx/treectrl.h"
#endif // wxUSE_TREECTRL

#include "wx/msw/private.h"
#include "wx/msw/uxtheme.h"

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxControl, wxWindow)

// ============================================================================
// wxControl implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxControl ctor/dtor
// ----------------------------------------------------------------------------

wxControl::~wxControl()
{
    m_isBeingDeleted = true;
}

// ----------------------------------------------------------------------------
// control window creation
// ----------------------------------------------------------------------------

bool wxControl::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxValidator& wxVALIDATOR_PARAM(validator),
                       const wxString& name)
{
    if ( !wxWindow::Create(parent, id, pos, size, style, name) )
        return false;

#if wxUSE_VALIDATORS
    SetValidator(validator);
#endif

    return true;
}

bool wxControl::MSWCreateControl(const wxChar *classname,
                                 const wxString& label,
                                 const wxPoint& pos,
                                 const wxSize& size)
{
    WXDWORD exstyle;
    WXDWORD msStyle = MSWGetStyle(GetWindowStyle(), &exstyle);

    return MSWCreateControl(classname, msStyle, pos, size, label, exstyle);
}

bool wxControl::MSWCreateControl(const wxChar *classname,
                                 WXDWORD style,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 const wxString& label,
                                 WXDWORD exstyle)
{
    // if no extended style given, determine it ourselves
    if ( exstyle == (WXDWORD)-1 )
    {
        exstyle = 0;
        (void) MSWGetStyle(GetWindowStyle(), &exstyle);
    }

    // all controls should have this style
    style |= WS_CHILD;

    // create the control visible if it's currently shown for wxWidgets
    if ( m_isShown )
    {
        style |= WS_VISIBLE;
    }

    // choose the position for the control: we have a problem with default size
    // here as we can't calculate the best size before the control exists
    // (DoGetBestSize() may need to use m_hWnd), so just choose the minimal
    // possible but non 0 size because 0 window width/height result in problems
    // elsewhere
    int x = pos.x == wxDefaultCoord ? 0 : pos.x,
        y = pos.y == wxDefaultCoord ? 0 : pos.y,
        w = size.x == wxDefaultCoord ? 1 : size.x,
        h = size.y == wxDefaultCoord ? 1 : size.y;

    // ... and adjust it to account for a possible parent frames toolbar
    AdjustForParentClientOrigin(x, y);

    m_hWnd = (WXHWND)::CreateWindowEx
                       (
                        exstyle,            // extended style
                        classname,          // the kind of control to create
                        label,              // the window name
                        style,              // the window style
                        x, y, w, h,         // the window position and size
                        GetHwndOf(GetParent()),  // parent
                        (HMENU)GetId(),     // child id
                        wxGetInstance(),    // app instance
                        NULL                // creation parameters
                       );

    if ( !m_hWnd )
    {
#ifdef __WXDEBUG__
        wxFAIL_MSG(wxString::Format
                   (
                    _T("CreateWindowEx(\"%s\", flags=%08x, ex=%08x) failed"),
                    classname, (unsigned int)style, (unsigned int)exstyle
                   ));
#endif // __WXDEBUG__

        return false;
    }

#if !wxUSE_UNICODE
    // Text labels starting with the character 0xff (which is a valid character
    // in many code pages) don't appear correctly as CreateWindowEx() has some
    // special treatment for this case, apparently the strings starting with -1
    // are not really strings but something called "ordinals". There is no
    // documentation about it but the fact is that the label gets mangled or
    // not displayed at all if we don't do this, see #9572.
    //
    // Notice that 0xffff is not a valid Unicode character so the problem
    // doesn't arise in Unicode build.
    if ( !label.empty() && label[0] == -1 )
        ::SetWindowText(GetHwnd(), label.c_str());
#endif // !wxUSE_UNICODE

    // install wxWidgets window proc for this window
    SubclassWin(m_hWnd);

    // set up fonts and colours
    InheritAttributes();
    if ( !m_hasFont )
    {
        bool setFont = true;

        wxFont font = GetDefaultAttributes().font;

        // if we set a font for {list,tree}ctrls and the font size is changed in
        // the display properties then the font size for these controls doesn't
        // automatically adjust when they receive WM_SETTINGCHANGE

        // FIXME: replace the dynamic casts with virtual function calls!!
#if wxUSE_LISTCTRL || wxUSE_TREECTRL
        bool testFont = false;
#if wxUSE_LISTCTRL
        if ( wxDynamicCastThis(wxListCtrl) )
            testFont = true;
#endif // wxUSE_LISTCTRL
#if wxUSE_TREECTRL
        if ( wxDynamicCastThis(wxTreeCtrl) )
            testFont = true;
#endif // wxUSE_TREECTRL

        if ( testFont )
        {
            // not sure if we need to explicitly set the font here for Win95/NT4
            // but we definitely can't do it for any newer version
            // see wxGetCCDefaultFont() in src/msw/settings.cpp for explanation
            // of why this test works

            // TODO: test Win95/NT4 to see if this is needed or breaks the
            // font resizing as it does on newer versions
            if ( font != wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) )
            {
                setFont = false;
            }
        }
#endif // wxUSE_LISTCTRL || wxUSE_TREECTRL

        if ( setFont )
        {
            SetFont(GetDefaultAttributes().font);
        }
    }

    // set the size now if no initial size specified
    SetInitialSize(size);

    return true;
}

// ----------------------------------------------------------------------------
// various accessors
// ----------------------------------------------------------------------------

wxBorder wxControl::GetDefaultBorder() const
{
    // we want to automatically give controls a sunken style (confusingly,
    // it may not really mean sunken at all as we map it to WS_EX_CLIENTEDGE
    // which is not sunken at all under Windows XP -- rather, just the default)
#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
    return wxBORDER_SIMPLE;
#else
    return wxBORDER_SUNKEN;
#endif
}

WXDWORD wxControl::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    long msStyle = wxWindow::MSWGetStyle(style, exstyle);

    if ( AcceptsFocus() )
    {
        msStyle |= WS_TABSTOP;
    }

    return msStyle;
}

wxSize wxControl::DoGetBestSize() const
{
    return wxSize(DEFAULT_ITEM_WIDTH, DEFAULT_ITEM_HEIGHT);
}

// This is a helper for all wxControls made with UPDOWN native control.
// In wxMSW it was only wxSpinCtrl derived from wxSpinButton but in
// WinCE of Smartphones this happens also for native wxTextCtrl,
// wxChoice and others.
wxSize wxControl::GetBestSpinnerSize(const bool is_vertical) const
{
    // take size according to layout
    wxSize bestSize(
#if defined(__SMARTPHONE__) && defined(__WXWINCE__)
                    0,GetCharHeight()
#else
                    ::GetSystemMetrics(is_vertical ? SM_CXVSCROLL : SM_CXHSCROLL),
                    ::GetSystemMetrics(is_vertical ? SM_CYVSCROLL : SM_CYHSCROLL)
#endif
    );

    // correct size as for undocumented MSW variants cases (WinCE and perhaps others)
    if (bestSize.x==0)
        bestSize.x = bestSize.y;
    if (bestSize.y==0)
        bestSize.y = bestSize.x;

    // double size according to layout
    if (is_vertical)
        bestSize.y *= 2;
    else
        bestSize.x *= 2;

    return bestSize;
}

/* static */ wxVisualAttributes
wxControl::GetClassDefaultAttributes(wxWindowVariant WXUNUSED(variant))
{
    wxVisualAttributes attrs;

    // old school (i.e. not "common") controls use the standard dialog font
    // by default
    attrs.font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    // most, or at least many, of the controls use the same colours as the
    // buttons -- others will have to override this (and possibly simply call
    // GetCompositeControlsDefaultAttributes() from their versions)
    attrs.colFg = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
    attrs.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);

    return attrs;
}

// another version for the "composite", i.e. non simple controls
/* static */ wxVisualAttributes
wxControl::GetCompositeControlsDefaultAttributes(wxWindowVariant WXUNUSED(variant))
{
    wxVisualAttributes attrs;
    attrs.font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    attrs.colFg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    attrs.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

    return attrs;
}

// ----------------------------------------------------------------------------
// message handling
// ----------------------------------------------------------------------------

bool wxControl::ProcessCommand(wxCommandEvent& event)
{
    return GetEventHandler()->ProcessEvent(event);
}

bool wxControl::MSWOnNotify(int idCtrl,
                            WXLPARAM lParam,
                            WXLPARAM* result)
{
    wxEventType eventType wxDUMMY_INITIALIZE(wxEVT_NULL);

    NMHDR *hdr = (NMHDR*) lParam;
    switch ( hdr->code )
    {
        case NM_CLICK:
            eventType = wxEVT_COMMAND_LEFT_CLICK;
            break;

        case NM_DBLCLK:
            eventType = wxEVT_COMMAND_LEFT_DCLICK;
            break;

        case NM_RCLICK:
            eventType = wxEVT_COMMAND_RIGHT_CLICK;
            break;

        case NM_RDBLCLK:
            eventType = wxEVT_COMMAND_RIGHT_DCLICK;
            break;

        case NM_SETFOCUS:
            eventType = wxEVT_COMMAND_SET_FOCUS;
            break;

        case NM_KILLFOCUS:
            eventType = wxEVT_COMMAND_KILL_FOCUS;
            break;

        case NM_RETURN:
            eventType = wxEVT_COMMAND_ENTER;
            break;

        default:
            return wxWindow::MSWOnNotify(idCtrl, lParam, result);
    }

    wxCommandEvent event(wxEVT_NULL, m_windowId);
    event.SetEventType(eventType);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

WXHBRUSH wxControl::DoMSWControlColor(WXHDC pDC, wxColour colBg, WXHWND hWnd)
{
    HDC hdc = (HDC)pDC;
    if ( m_hasFgCol )
    {
        ::SetTextColor(hdc, wxColourToRGB(GetForegroundColour()));
    }

    WXHBRUSH hbr = 0;
    if ( !colBg.Ok() )
    {
        hbr = MSWGetBgBrush(pDC, hWnd);

        // if the control doesn't have any bg colour, foreground colour will be
        // ignored as the return value would be 0 -- so forcefully give it a
        // non default background brush in this case
        if ( !hbr && m_hasFgCol )
            colBg = GetBackgroundColour();
    }

    // use the background colour override if a valid colour is given
    if ( colBg.Ok() )
    {
        ::SetBkColor(hdc, wxColourToRGB(colBg));

        // draw children with the same colour as the parent
        wxBrush *brush = wxTheBrushList->FindOrCreateBrush(colBg, wxSOLID);

        hbr = (WXHBRUSH)brush->GetResourceHandle();

    }

    // if we use custom background, we should set foreground ourselves too
    if ( hbr && !m_hasFgCol )
    {
        ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
    }
    //else: already set above

    return hbr;
}

WXHBRUSH wxControl::MSWControlColor(WXHDC pDC, WXHWND hWnd)
{
    wxColour colBg;

    if ( HasTransparentBackground() )
        ::SetBkMode((HDC)pDC, TRANSPARENT);
    else // if the control is opaque it shouldn't use the parents background
        colBg = GetBackgroundColour();

    return DoMSWControlColor(pDC, colBg, hWnd);
}

WXHBRUSH wxControl::MSWControlColorDisabled(WXHDC pDC)
{
    return DoMSWControlColor(pDC,
                             wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE),
                             GetHWND());
}

// ---------------------------------------------------------------------------
// global functions
// ---------------------------------------------------------------------------

// this is used in radiobox.cpp and slider95.cpp and should be removed as soon
// as it is not needed there any more!
//
// Call this repeatedly for several wnds to find the overall size
// of the widget.
// Call it initially with wxDefaultCoord for all values in rect.
// Keep calling for other widgets, and rect will be modified
// to calculate largest bounding rectangle.
void wxFindMaxSize(WXHWND wnd, RECT *rect)
{
    int left = rect->left;
    int right = rect->right;
    int top = rect->top;
    int bottom = rect->bottom;

    GetWindowRect((HWND) wnd, rect);

    if (left < 0)
        return;

    if (left < rect->left)
        rect->left = left;

    if (right > rect->right)
        rect->right = right;

    if (top < rect->top)
        rect->top = top;

    if (bottom > rect->bottom)
        rect->bottom = bottom;
}

#endif // wxUSE_CONTROLS
