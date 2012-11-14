/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/gauge95.cpp
// Purpose:     wxGauge95 class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: gauge95.cpp 62070 2009-09-24 10:18:25Z JS $
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

#if wxUSE_GAUGE

#include "wx/gauge.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"

    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
#endif

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// old commctrl.h (< 4.71) don't have those
#ifndef PBS_SMOOTH
    #define PBS_SMOOTH 0x01
#endif

#ifndef PBS_VERTICAL
    #define PBS_VERTICAL 0x04
#endif

#ifndef PBM_SETBARCOLOR
    #define PBM_SETBARCOLOR         (WM_USER+9)
#endif

#ifndef PBM_SETBKCOLOR
    #define PBM_SETBKCOLOR          0x2001
#endif

#ifndef PBS_MARQUEE
    #define PBS_MARQUEE             0x08
#endif

#ifndef PBM_SETMARQUEE
    #define PBM_SETMARQUEE          (WM_USER+10)
#endif

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxGaugeStyle )

wxBEGIN_FLAGS( wxGaugeStyle )
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

    wxFLAGS_MEMBER(wxGA_HORIZONTAL)
    wxFLAGS_MEMBER(wxGA_VERTICAL)
#if WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxGA_PROGRESSBAR)
#endif // WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxGA_SMOOTH)

wxEND_FLAGS( wxGaugeStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxGauge, wxControl,"wx/gauge.h")

wxBEGIN_PROPERTIES_TABLE(wxGauge95)
    wxPROPERTY( Value , int , SetValue, GetValue, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Range , int , SetRange, GetRange, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( ShadowWidth , int , SetShadowWidth, GetShadowWidth, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( BezelFace , int , SetBezelFace, GetBezelFace, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxGaugeStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxGauge95)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxGauge95 , wxWindow* , Parent , wxWindowID , Id , int , Range , wxPoint , Position , wxSize , Size , long , WindowStyle )
#else
IMPLEMENT_DYNAMIC_CLASS(wxGauge95, wxControl)
#endif

// ============================================================================
// wxGauge95 implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxGauge95 creation
// ----------------------------------------------------------------------------

bool wxGauge95::Create(wxWindow *parent,
                       wxWindowID id,
                       int range,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxValidator& validator,
                       const wxString& name)
{
    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    if ( !MSWCreateControl(PROGRESS_CLASS, wxEmptyString, pos, size) )
        return false;

    SetRange(range);

    // in case we need to emulate indeterminate mode...
    m_nDirection = wxRIGHT;

    return true;
}

WXDWORD wxGauge95::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD msStyle = wxControl::MSWGetStyle(style, exstyle);

    if ( style & wxGA_VERTICAL )
        msStyle |= PBS_VERTICAL;

    if ( style & wxGA_SMOOTH )
        msStyle |= PBS_SMOOTH;

    return msStyle;
}

// ----------------------------------------------------------------------------
// wxGauge95 geometry
// ----------------------------------------------------------------------------

wxSize wxGauge95::DoGetBestSize() const
{
    // VZ: no idea where does 28 come from, it was there before my changes and
    //     as nobody ever complained I guess we can leave it...
    if (HasFlag(wxGA_VERTICAL))
        return wxSize(28, 100);
    else
        return wxSize(100, 28);
}

// ----------------------------------------------------------------------------
// wxGauge95 setters
// ----------------------------------------------------------------------------

void wxGauge95::SetRange(int r)
{
    // switch to determinate mode if required
    SetDeterminateMode();

    m_rangeMax = r;

#ifdef PBM_SETRANGE32
    ::SendMessage(GetHwnd(), PBM_SETRANGE32, 0, r);
#else // !PBM_SETRANGE32
    // fall back to PBM_SETRANGE (limited to 16 bits)
    ::SendMessage(GetHwnd(), PBM_SETRANGE, 0, MAKELPARAM(0, r));
#endif // PBM_SETRANGE32/!PBM_SETRANGE32
}

void wxGauge95::SetValue(int pos)
{
    // Setting the (same) position produces flicker on Vista,
    // especially noticable if ownerdrawn
    if (GetValue() == pos) return;

    // switch to determinate mode if required
    SetDeterminateMode();

    m_gaugePos = pos;

    ::SendMessage(GetHwnd(), PBM_SETPOS, pos, 0);
}

bool wxGauge95::SetForegroundColour(const wxColour& col)
{
    if ( !wxControl::SetForegroundColour(col) )
        return false;

    ::SendMessage(GetHwnd(), PBM_SETBARCOLOR, 0, (LPARAM)wxColourToRGB(col));

    return true;
}

bool wxGauge95::SetBackgroundColour(const wxColour& col)
{
    if ( !wxControl::SetBackgroundColour(col) )
        return false;

    ::SendMessage(GetHwnd(), PBM_SETBKCOLOR, 0, (LPARAM)wxColourToRGB(col));

    return true;
}

void wxGauge95::SetIndeterminateMode()
{
    // add the PBS_MARQUEE style to the progress bar
    LONG style = ::GetWindowLong(GetHwnd(), GWL_STYLE);
    if ((style & PBS_MARQUEE) == 0)
        ::SetWindowLong(GetHwnd(), GWL_STYLE, style|PBS_MARQUEE);

    // now the control can only run in indeterminate mode
}

void wxGauge95::SetDeterminateMode()
{
    // remove the PBS_MARQUEE style to the progress bar
    LONG style = ::GetWindowLong(GetHwnd(), GWL_STYLE);
    if ((style & PBS_MARQUEE) != 0)
        ::SetWindowLong(GetHwnd(), GWL_STYLE, style & ~PBS_MARQUEE);

    // now the control can only run in determinate mode
}

void wxGauge95::Pulse()
{
    if (wxApp::GetComCtl32Version() >= 600)
    {
        // switch to indeterminate mode if required
        SetIndeterminateMode();

        // NOTE: when in indeterminate mode, the PBM_SETPOS message will just make
        //       the bar's blocks move a bit and the WPARAM value is just ignored
        //       so that we can safely use zero
        SendMessage(GetHwnd(), (UINT) PBM_SETPOS, (WPARAM)0, (LPARAM)0);
    }
    else
    {
        // emulate indeterminate mode
        wxGaugeBase::Pulse();
    }
}

#endif // wxUSE_GAUGE
