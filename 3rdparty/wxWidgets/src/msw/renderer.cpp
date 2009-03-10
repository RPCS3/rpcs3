///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/renderer.cpp
// Purpose:     implementation of wxRendererNative for Windows
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.07.2003
// RCS-ID:      $Id: renderer.cpp 53524 2008-05-10 00:05:20Z RD $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/window.h"
    #include "wx/dc.h"
    #include "wx/settings.h"
#endif //WX_PRECOMP

#include "wx/splitter.h"
#include "wx/renderer.h"
#include "wx/msw/private.h"
#include "wx/msw/uxtheme.h"

#if wxUSE_GRAPHICS_CONTEXT
// TODO remove this dependency (gdiplus needs the macros)
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include "gdiplus.h"
using namespace Gdiplus;
#endif

// tmschema.h is in Win32 Platform SDK and might not be available with earlier
// compilers
#ifndef CP_DROPDOWNBUTTON
    #define BP_PUSHBUTTON      1
    #define BP_RADIOBUTTON     2
    #define BP_CHECKBOX        3
    #define RBS_UNCHECKEDNORMAL 1
    #define RBS_CHECKEDNORMAL   (RBS_UNCHECKEDNORMAL + 4)
    #define RBS_MIXEDNORMAL     (RBS_CHECKEDNORMAL + 4)
    #define CBS_UNCHECKEDNORMAL 1
    #define CBS_CHECKEDNORMAL   (CBS_UNCHECKEDNORMAL + 4)
    #define CBS_MIXEDNORMAL     (CBS_CHECKEDNORMAL + 4)

    #define PBS_NORMAL          1
    #define PBS_HOT             2
    #define PBS_PRESSED         3
    #define PBS_DISABLED        4
    #define PBS_DEFAULTED       5

    #define CP_DROPDOWNBUTTON  1

    #define CBXS_NORMAL        1
    #define CBXS_HOT           2
    #define CBXS_PRESSED       3
    #define CBXS_DISABLED      4

    #define TVP_GLYPH           2

    #define GLPS_CLOSED         1
    #define GLPS_OPENED         2

    #define HP_HEADERITEM       1

    #define HIS_NORMAL          1
    #define HIS_HOT             2
    #define HIS_PRESSED         3

    #define TMT_HEIGHT          2417

    #define HP_HEADERSORTARROW  4
    #define HSAS_SORTEDUP       1
    #define HSAS_SORTEDDOWN     2

    #define EP_EDITTEXT         1
    #define ETS_NORMAL          1
    #define ETS_HOT             2
    #define ETS_SELECTED        3
    #define ETS_DISABLED        4
    #define ETS_FOCUSED         5
    #define ETS_READONLY        6
    #define ETS_ASSIST          7
    #define TMT_FILLCOLOR       3802
    #define TMT_TEXTCOLOR       3803
    #define TMT_BORDERCOLOR     3801
    #define TMT_EDGEFILLCOLOR   3808
#endif

#if defined(__WXWINCE__) && !defined(DFCS_FLAT)
    #define DFCS_FLAT 0
#endif


// ----------------------------------------------------------------------------
// If the DC is a wxGCDC then pull out the HDC from the GraphicsContext when
// it is needed, and handle the Release when done.

class GraphicsHDC
{
public:
    GraphicsHDC(wxDC* dc)
    {
#if wxUSE_GRAPHICS_CONTEXT
        m_graphics = NULL;
        wxGCDC* gcdc = wxDynamicCast(dc, wxGCDC);
        if (gcdc) {
            m_graphics = (Graphics*)gcdc->GetGraphicsContext()->GetNativeContext();
            m_hdc = m_graphics->GetHDC();
        }
        else
#endif
            m_hdc = GetHdcOf(*dc);
    }

    ~GraphicsHDC()
    {
#if wxUSE_GRAPHICS_CONTEXT
        if (m_graphics)
            m_graphics->ReleaseHDC(m_hdc);
#endif
    }
    
    operator HDC() const { return m_hdc; }

private:
    HDC         m_hdc;
#if wxUSE_GRAPHICS_CONTEXT
    Graphics*   m_graphics;
#endif
};

// ----------------------------------------------------------------------------
// wxRendererMSW: wxRendererNative implementation for "old" Win32 systems
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxRendererMSW : public wxDelegateRendererNative
{
public:
    wxRendererMSW() { }

    static wxRendererNative& Get();

    virtual void DrawComboBoxDropButton(wxWindow *win,
                                        wxDC& dc,
                                        const wxRect& rect,
                                        int flags = 0);

    virtual void DrawPushButton(wxWindow *win,
                                wxDC& dc,
                                const wxRect& rect,
                                int flags = 0);

private:
    DECLARE_NO_COPY_CLASS(wxRendererMSW)
};

// ----------------------------------------------------------------------------
// wxRendererXP: wxRendererNative implementation for Windows XP and later
// ----------------------------------------------------------------------------

#if wxUSE_UXTHEME

class WXDLLEXPORT wxRendererXP : public wxDelegateRendererNative
{
public:
    wxRendererXP() : wxDelegateRendererNative(wxRendererMSW::Get()) { }

    static wxRendererNative& Get();

    virtual int DrawHeaderButton(wxWindow *win,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int flags = 0,
                                  wxHeaderSortIconType sortArrow = wxHDR_SORT_ICON_NONE,
                                  wxHeaderButtonParams* params = NULL);
    virtual int GetHeaderButtonHeight(wxWindow *win);
    
    virtual void DrawTreeItemButton(wxWindow *win,
                                    wxDC& dc,
                                    const wxRect& rect,
                                    int flags = 0);
    virtual void DrawSplitterBorder(wxWindow *win,
                                    wxDC& dc,
                                    const wxRect& rect,
                                    int flags = 0);
    virtual void DrawSplitterSash(wxWindow *win,
                                  wxDC& dc,
                                  const wxSize& size,
                                  wxCoord position,
                                  wxOrientation orient,
                                  int flags = 0);
    virtual void DrawComboBoxDropButton(wxWindow *win,
                                        wxDC& dc,
                                        const wxRect& rect,
                                        int flags = 0);
    virtual void DrawCheckBox(wxWindow *win,
                              wxDC& dc,
                              const wxRect& rect,
                              int flags = 0);

    virtual void DrawPushButton(wxWindow *win,
                                wxDC& dc,
                                const wxRect& rect,
                                int flags = 0);

    virtual wxSplitterRenderParams GetSplitterParams(const wxWindow *win);
private:
    DECLARE_NO_COPY_CLASS(wxRendererXP)
};

#endif // wxUSE_UXTHEME

// ============================================================================
// wxRendererNative and wxRendererMSW implementation
// ============================================================================

/* static */
wxRendererNative& wxRendererNative::GetDefault()
{
#if wxUSE_UXTHEME
    wxUxThemeEngine *themeEngine = wxUxThemeEngine::Get();
    if ( themeEngine && themeEngine->IsAppThemed() )
        return wxRendererXP::Get();
#endif // wxUSE_UXTHEME

    return wxRendererMSW::Get();
}

/* static */
wxRendererNative& wxRendererMSW::Get()
{
    static wxRendererMSW s_rendererMSW;

    return s_rendererMSW;
}

void
wxRendererMSW::DrawComboBoxDropButton(wxWindow * WXUNUSED(win),
                                      wxDC& dc,
                                      const wxRect& rect,
                                      int flags)
{
    RECT r;
    r.left = rect.GetLeft();
    r.top = rect.GetTop();
    r.bottom = rect.y + rect.height;
    r.right = rect.x + rect.width;

    int style = DFCS_SCROLLCOMBOBOX;
    if ( flags & wxCONTROL_DISABLED )
        style |= DFCS_INACTIVE;
    if ( flags & wxCONTROL_PRESSED )
        style |= DFCS_PUSHED | DFCS_FLAT;

    ::DrawFrameControl(GraphicsHDC(&dc), &r, DFC_SCROLL, style);
}

void
wxRendererMSW::DrawPushButton(wxWindow * WXUNUSED(win),
                              wxDC& dc,
                              const wxRect& rectOrig,
                              int flags)
{
    wxRect rect(rectOrig);

    int style = DFCS_BUTTONPUSH;
    if ( flags & wxCONTROL_DISABLED )
        style |= DFCS_INACTIVE;
    if ( flags & wxCONTROL_PRESSED )
        style |= DFCS_PUSHED | DFCS_FLAT;
    if ( flags & wxCONTROL_ISDEFAULT )
    {
        // DrawFrameControl() doesn't seem to support default buttons so we
        // have to draw the border ourselves
        wxDCPenChanger pen(dc, *wxBLACK_PEN);
        wxDCBrushChanger brush(dc, *wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
        rect.Deflate(1);
    }

    RECT rc;
    wxCopyRectToRECT(rect, rc);

    ::DrawFrameControl(GraphicsHDC(&dc), &rc, DFC_BUTTON, style);
}

// ============================================================================
// wxRendererXP implementation
// ============================================================================

#if wxUSE_UXTHEME

/* static */
wxRendererNative& wxRendererXP::Get()
{
    static wxRendererXP s_rendererXP;

    return s_rendererXP;
}

// NOTE: There is no guarantee that the button drawn fills the entire rect (XP
// default theme, for example), so the caller should have cleared button's
// background before this call. This is quite likely a wxMSW-specific thing.
void
wxRendererXP::DrawComboBoxDropButton(wxWindow * win,
                                      wxDC& dc,
                                      const wxRect& rect,
                                      int flags)
{
    wxUxThemeHandle hTheme(win, L"COMBOBOX");
    if ( !hTheme )
    {
        m_rendererNative.DrawComboBoxDropButton(win, dc, rect, flags);
        return;
    }

    RECT r;
    wxCopyRectToRECT(rect, r);

    int state;
    if ( flags & wxCONTROL_PRESSED )
        state = CBXS_PRESSED;
    else if ( flags & wxCONTROL_CURRENT )
        state = CBXS_HOT;
    else if ( flags & wxCONTROL_DISABLED )
        state = CBXS_DISABLED;
    else
        state = CBXS_NORMAL;

    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                CP_DROPDOWNBUTTON,
                                state,
                                &r,
                                NULL
                            );

}

int
wxRendererXP::DrawHeaderButton(wxWindow *win,
                               wxDC& dc,
                               const wxRect& rect,
                               int flags,
                               wxHeaderSortIconType sortArrow,
                               wxHeaderButtonParams* params)
{
    wxUxThemeHandle hTheme(win, L"HEADER");
    if ( !hTheme )
    {
        return m_rendererNative.DrawHeaderButton(win, dc, rect, flags, sortArrow, params);
    }

    RECT r;
    wxCopyRectToRECT(rect, r);

    int state;
    if ( flags & wxCONTROL_PRESSED )
        state = HIS_PRESSED;
    else if ( flags & wxCONTROL_CURRENT )
        state = HIS_HOT;
    else
        state = HIS_NORMAL;
    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                HP_HEADERITEM,
                                state,
                                &r,
                                NULL
                            );

    // NOTE: Using the theme to draw HP_HEADERSORTARROW doesn't do anything.
    // Why?  If this can be fixed then draw the sort arrows using the theme
    // and then clear those flags before calling DrawHeaderButtonContents.
    
    // Add any extras that are specified in flags and params
    return DrawHeaderButtonContents(win, dc, rect, flags, sortArrow, params);
}


int
wxRendererXP::GetHeaderButtonHeight(wxWindow *win)
{
    wxUxThemeHandle hTheme(win, L"HEADER");
    if ( !hTheme )
    {
        return m_rendererNative.GetHeaderButtonHeight(win);
    }
    
    HRESULT hr;
    int value = -1;
    
    hr = wxUxThemeEngine::Get()->GetThemeMetric( hTheme,
                                                 NULL,
                                                 HP_HEADERITEM,
                                                 HIS_NORMAL,
                                                 TMT_HEIGHT,
                                                 &value );
    if ( hr == S_OK )
        return value;
    else
        return 20;
}


void
wxRendererXP::DrawTreeItemButton(wxWindow *win,
                                 wxDC& dc,
                                 const wxRect& rect,
                                 int flags)
{
    wxUxThemeHandle hTheme(win, L"TREEVIEW");
    if ( !hTheme )
    {
        m_rendererNative.DrawTreeItemButton(win, dc, rect, flags);
        return;
    }

    RECT r;
    wxCopyRectToRECT(rect, r);

    int state = flags & wxCONTROL_EXPANDED ? GLPS_OPENED : GLPS_CLOSED;
    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                TVP_GLYPH,
                                state,
                                &r,
                                NULL
                            );
}

void
wxRendererXP::DrawCheckBox(wxWindow *win,
                           wxDC& dc,
                           const wxRect& rect,
                           int flags)
{
    wxUxThemeHandle hTheme(win, L"BUTTON");
    if ( !hTheme )
    {
        m_rendererNative.DrawCheckBox(win, dc, rect, flags);
        return;
    }

    RECT r;
    wxCopyRectToRECT(rect, r);

    int state;
    if ( flags & wxCONTROL_CHECKED )
        state = CBS_CHECKEDNORMAL;
    else if ( flags & wxCONTROL_UNDETERMINED )
        state = CBS_MIXEDNORMAL;
    else
        state = CBS_UNCHECKEDNORMAL;

    // CBS_XXX is followed by CBX_XXXGOT, then CBS_XXXPRESSED and DISABLED
    if ( flags & wxCONTROL_CURRENT )
        state += 1;
    else if ( flags & wxCONTROL_PRESSED )
        state += 2;
    else if ( flags & wxCONTROL_DISABLED )
        state += 3;

    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                BP_CHECKBOX,
                                state,
                                &r,
                                NULL
                            );
}



void
wxRendererXP::DrawPushButton(wxWindow * win,
                             wxDC& dc,
                             const wxRect& rect,
                             int flags)
{
    wxUxThemeHandle hTheme(win, L"BUTTON");
    if ( !hTheme )
    {
        m_rendererNative.DrawPushButton(win, dc, rect, flags);
        return;
    }

    RECT r;
    wxCopyRectToRECT(rect, r);

    int state;
    if ( flags & wxCONTROL_PRESSED )
        state = PBS_PRESSED;
    else if ( flags & wxCONTROL_CURRENT )
        state = PBS_HOT;
    else if ( flags & wxCONTROL_DISABLED )
        state = PBS_DISABLED;
    else if ( flags & wxCONTROL_ISDEFAULT )
        state = PBS_DEFAULTED;
    else
        state = PBS_NORMAL;

    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                BP_PUSHBUTTON,
                                state,
                                &r,
                                NULL
                            );
}

// ----------------------------------------------------------------------------
// splitter drawing
// ----------------------------------------------------------------------------

// the width of the sash: this is the same as used by Explorer...
static const wxCoord SASH_WIDTH = 4;

wxSplitterRenderParams
wxRendererXP::GetSplitterParams(const wxWindow * win)
{
    if ( win->HasFlag(wxSP_NO_XP_THEME) )
        return m_rendererNative.GetSplitterParams(win);
    else
        return wxSplitterRenderParams(SASH_WIDTH, 0, false);
}

void
wxRendererXP::DrawSplitterBorder(wxWindow * win,
                                 wxDC& dc,
                                 const wxRect& rect,
                                 int flags)
{
    if ( win->HasFlag(wxSP_NO_XP_THEME) )
    {
        m_rendererNative.DrawSplitterBorder(win, dc, rect, flags);
    }
}

void
wxRendererXP::DrawSplitterSash(wxWindow *win,
                               wxDC& dc,
                               const wxSize& size,
                               wxCoord position,
                               wxOrientation orient,
                               int flags)
{
    if ( !win->HasFlag(wxSP_NO_XP_THEME) )
    {
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
        if ( orient == wxVERTICAL )
        {
            dc.DrawRectangle(position, 0, SASH_WIDTH, size.y);
        }
        else // wxHORIZONTAL
        {
            dc.DrawRectangle(0, position, size.x, SASH_WIDTH);
        }

        return;
    }

    m_rendererNative.DrawSplitterSash(win, dc, size, position, orient, flags);
}

// ----------------------------------------------------------------------------
// Other renderer functions to be merged in to wxRenderer class in 2.9, but
// they are standalone functions here to protect the ABI.
// ----------------------------------------------------------------------------

// Uses the theme to draw the border and fill for something like a wxTextCtrl
void wxRenderer_DrawTextCtrl(wxWindow* win, wxDC& dc, const wxRect& rect, int flags)
{
    wxColour fill;
    wxColour bdr;
    COLORREF cref;
    
#if wxUSE_UXTHEME
    wxUxThemeHandle hTheme(win, L"EDIT");
    if (hTheme)
    {
        wxUxThemeEngine::Get()->GetThemeColor(hTheme, EP_EDITTEXT,
                                              ETS_NORMAL, TMT_FILLCOLOR, &cref);
        fill = wxRGBToColour(cref);

        int etsState;
        if ( flags & wxCONTROL_DISABLED )
            etsState = ETS_DISABLED;
        else
            etsState = ETS_NORMAL;
        
        wxUxThemeEngine::Get()->GetThemeColor(hTheme, EP_EDITTEXT,
                                              etsState, TMT_BORDERCOLOR, &cref);
        bdr = wxRGBToColour(cref);        
    }
    else
#endif
    {
        fill = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        bdr = *wxBLACK;
    }
    
    dc.SetPen( bdr );
    dc.SetBrush( fill );
    dc.DrawRectangle(rect);
}


// Draw the equivallent of a wxComboBox
void wxRenderer_DrawComboBox(wxWindow* win, wxDC& dc, const wxRect& rect, int flags)
{
    // Draw the main part of the control same as TextCtrl
    wxRenderer_DrawTextCtrl(win, dc, rect, flags);    
    
    // Draw the button inside the border, on the right side
    wxRect br(rect);
    br.height -= 2;
    br.x += br.width - br.height - 1;
    br.width = br.height;
    br.y += 1;

    wxRendererNative::Get().DrawComboBoxDropButton(win, dc, br, flags);
}


void wxRenderer_DrawChoice(wxWindow* win, wxDC& dc,
                           const wxRect& rect, int flags)
{
    wxRenderer_DrawComboBox(win, dc, rect, flags);
}

    
// Draw a themed radio button
void wxRenderer_DrawRadioButton(wxWindow* win, wxDC& dc, const wxRect& rect, int flags)
{
#if wxUSE_UXTHEME
    wxUxThemeHandle hTheme(win, L"BUTTON");
    if ( !hTheme )
#endif
    {
        // ??? m_rendererNative.DrawRadioButton(win, dc, rect, flags);
        return;
    }

#if wxUSE_UXTHEME
    RECT r;
    wxCopyRectToRECT(rect, r);

    int state;
    if ( flags & wxCONTROL_CHECKED )
        state = RBS_CHECKEDNORMAL;
    else if ( flags & wxCONTROL_UNDETERMINED )
        state = RBS_MIXEDNORMAL;
    else
        state = RBS_UNCHECKEDNORMAL;

    // RBS_XXX is followed by RBX_XXXGOT, then RBS_XXXPRESSED and DISABLED
    if ( flags & wxCONTROL_CURRENT )
        state += 1;
    else if ( flags & wxCONTROL_PRESSED )
        state += 2;
    else if ( flags & wxCONTROL_DISABLED )
        state += 3;

    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                hTheme,
                                GraphicsHDC(&dc),
                                BP_RADIOBUTTON,
                                state,
                                &r,
                                NULL
                            );
#endif
}

#endif // wxUSE_UXTHEME
