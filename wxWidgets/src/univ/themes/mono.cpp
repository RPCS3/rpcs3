///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/themes/mono.cpp
// Purpose:     wxUniversal theme for monochrome displays
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-08-27
// RCS-ID:      $Id: mono.cpp 54496 2008-07-05 18:25:33Z SN $
// Copyright:   (c) 2006 REA Elektronik GmbH
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/univ/theme.h"

#if wxUSE_THEME_MONO

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/dc.h"
#endif // WX_PRECOMP

#include "wx/artprov.h"
#include "wx/univ/stdrend.h"
#include "wx/univ/inphand.h"
#include "wx/univ/colschem.h"

class wxMonoColourScheme;

#define wxMONO_BG_COL   (*wxWHITE)
#define wxMONO_FG_COL   (*wxBLACK)

// ----------------------------------------------------------------------------
// wxMonoRenderer: draw the GUI elements in simplest possible way
// ----------------------------------------------------------------------------

// Warning: many of the methods here are not implemented, the code won't work
// if any but a few wxUSE_XXXs are on
class wxMonoRenderer : public wxStdRenderer
{
public:
    wxMonoRenderer(const wxColourScheme *scheme);

    virtual void DrawButtonLabel(wxDC& dc,
                                 const wxString& label,
                                 const wxBitmap& image,
                                 const wxRect& rect,
                                 int flags = 0,
                                 int alignment = wxALIGN_LEFT | wxALIGN_TOP,
                                 int indexAccel = -1,
                                 wxRect *rectBounds = NULL);

    virtual void DrawFocusRect(wxDC& dc, const wxRect& rect, int flags = 0);

    virtual void DrawButtonBorder(wxDC& dc,
                                  const wxRect& rect,
                                  int flags = 0,
                                  wxRect *rectIn = NULL);

    virtual void DrawHorizontalLine(wxDC& dc,
                                    wxCoord y, wxCoord x1, wxCoord x2);

    virtual void DrawVerticalLine(wxDC& dc,
                                  wxCoord x, wxCoord y1, wxCoord y2);

    virtual void DrawArrow(wxDC& dc,
                           wxDirection dir,
                           const wxRect& rect,
                           int flags = 0);
    virtual void DrawScrollbarThumb(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0);
    virtual void DrawScrollbarShaft(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags = 0);

#if wxUSE_TOOLBAR
    virtual void DrawToolBarButton(wxDC& dc,
                                   const wxString& label,
                                   const wxBitmap& bitmap,
                                   const wxRect& rect,
                                   int flags = 0,
                                   long style = 0,
                                   int tbarStyle = 0);
#endif // wxUSE_TOOLBAR

#if wxUSE_NOTEBOOK
    virtual void DrawTab(wxDC& dc,
                         const wxRect& rect,
                         wxDirection dir,
                         const wxString& label,
                         const wxBitmap& bitmap = wxNullBitmap,
                         int flags = 0,
                         int indexAccel = -1);
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER
    virtual void DrawSliderShaft(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0,
                                 wxRect *rectShaft = NULL);

    virtual void DrawSliderThumb(wxDC& dc,
                                 const wxRect& rect,
                                 wxOrientation orient,
                                 int flags = 0,
                                 long style = 0);

    virtual void DrawSliderTicks(wxDC& dc,
                                 const wxRect& rect,
                                 int lenThumb,
                                 wxOrientation orient,
                                 int start,
                                 int end,
                                 int step = 1,
                                 int flags = 0,
                                 long style = 0);
#endif // wxUSE_SLIDER

#if wxUSE_MENUS
    virtual void DrawMenuBarItem(wxDC& dc,
                                 const wxRect& rect,
                                 const wxString& label,
                                 int flags = 0,
                                 int indexAccel = -1);

    virtual void DrawMenuItem(wxDC& dc,
                              wxCoord y,
                              const wxMenuGeometryInfo& geometryInfo,
                              const wxString& label,
                              const wxString& accel,
                              const wxBitmap& bitmap = wxNullBitmap,
                              int flags = 0,
                              int indexAccel = -1);

    virtual void DrawMenuSeparator(wxDC& dc,
                                   wxCoord y,
                                   const wxMenuGeometryInfo& geomInfo);
#endif // wxUSE_MENUS

#if wxUSE_COMBOBOX
    virtual void GetComboBitmaps(wxBitmap *bmpNormal,
                                 wxBitmap *bmpFocus,
                                 wxBitmap *bmpPressed,
                                 wxBitmap *bmpDisabled);
#endif // wxUSE_COMBOBOX


    virtual wxRect GetBorderDimensions(wxBorder border) const;

#if wxUSE_SCROLLBAR
    virtual wxSize GetScrollbarArrowSize() const { return GetStdBmpSize(); }
#endif // wxUSE_SCROLLBAR

    virtual wxSize GetCheckBitmapSize() const { return GetStdBmpSize(); }
    virtual wxSize GetRadioBitmapSize() const { return GetStdBmpSize(); }

#if wxUSE_TOOLBAR
    virtual wxSize GetToolBarButtonSize(wxCoord *separator) const;

    virtual wxSize GetToolBarMargin() const;
#endif // wxUSE_TOOLBAR

#if wxUSE_NOTEBOOK
    virtual wxSize GetTabIndent() const;

    virtual wxSize GetTabPadding() const;
#endif // wxUSE_NOTEBOOK

#if wxUSE_SLIDER
    virtual wxCoord GetSliderDim() const;

    virtual wxCoord GetSliderTickLen() const;

    virtual wxRect GetSliderShaftRect(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient,
                                      long style = 0) const;

    virtual wxSize GetSliderThumbSize(const wxRect& rect,
                                      int lenThumb,
                                      wxOrientation orient) const;
#endif // wxUSE_SLIDER

    virtual wxSize GetProgressBarStep() const;

#if wxUSE_MENUS
    virtual wxSize GetMenuBarItemSize(const wxSize& sizeText) const;

    virtual wxMenuGeometryInfo *GetMenuGeometry(wxWindow *win,
                                                const wxMenu& menu) const;
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    virtual wxCoord GetStatusBarBorderBetweenFields() const;

    virtual wxSize GetStatusBarFieldMargins() const;
#endif // wxUSE_STATUSBAR

protected:
    // override base class border drawing routines: we always draw just a
    // single simple border
    void DrawSimpleBorder(wxDC& dc, wxRect *rect)
        { DrawRect(dc, rect, m_penFg); }

    virtual void DrawRaisedBorder(wxDC& dc, wxRect *rect)
        { DrawSimpleBorder(dc, rect); }
    virtual void DrawSunkenBorder(wxDC& dc, wxRect *rect)
        { DrawSimpleBorder(dc, rect); }
    virtual void DrawAntiSunkenBorder(wxDC& dc, wxRect *rect)
        { DrawSimpleBorder(dc, rect); }
    virtual void DrawBoxBorder(wxDC& dc, wxRect *rect)
        { DrawSimpleBorder(dc, rect); }
    virtual void DrawStaticBorder(wxDC& dc, wxRect *rect)
        { DrawSimpleBorder(dc, rect); }
    virtual void DrawExtraBorder(wxDC& WXUNUSED(dc), wxRect * WXUNUSED(rect))
        { /* no extra borders for us */ }

    // all our XPMs are of this size
    static wxSize GetStdBmpSize() { return wxSize(8, 8); }

    wxBitmap GetIndicator(IndicatorType indType, int flags);
    virtual wxBitmap GetCheckBitmap(int flags)
        { return GetIndicator(IndicatorType_Check, flags); }
    virtual wxBitmap GetRadioBitmap(int flags)
        { return GetIndicator(IndicatorType_Radio, flags); }

    virtual wxBitmap GetFrameButtonBitmap(FrameButtonType type);
    virtual int GetFrameBorderWidth(int flags) const;

private:
    // the bitmaps returned by GetIndicator()
    wxBitmap m_bmpIndicators[IndicatorType_MaxCtrl]
                            [IndicatorState_MaxCtrl]
                            [IndicatorStatus_Max];

    static const char **ms_xpmIndicators[IndicatorType_MaxCtrl]
                                        [IndicatorState_MaxCtrl]
                                        [IndicatorStatus_Max];

    // the arrow bitmaps used by DrawArrow()
    wxBitmap m_bmpArrows[Arrow_Max];

    static const char **ms_xpmArrows[Arrow_Max];

    // the close bitmap for the frame for GetFrameButtonBitmap()
    wxBitmap m_bmpFrameClose;

    // pen used for foreground drawing
    wxPen m_penFg;
};

// ----------------------------------------------------------------------------
// standard bitmaps
// ----------------------------------------------------------------------------

static const char *xpmUnchecked[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"XXXXXXXX",
"X      X",
"X      X",
"X      X",
"X      X",
"X      X",
"X      X",
"XXXXXXXX",
};

static const char *xpmChecked[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"XXXXXXXX",
"X      X",
"X X  X X",
"X  XX  X",
"X  XX  X",
"X X  X X",
"X      X",
"XXXXXXXX",
};

static const char *xpmUndeterminate[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"XXXXXXXX",
"X X X XX",
"XX X X X",
"X X X XX",
"XX X X X",
"X X X XX",
"XX X X X",
"XXXXXXXX",
};

static const char *xpmRadioUnchecked[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"XXXXXXXX",
"X      X",
"X  XX  X",
"X X  X X",
"X X  X X",
"X  XX  X",
"X      X",
"XXXXXXXX",
};

static const char *xpmRadioChecked[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"XXXXXXXX",
"X      X",
"X  XX  X",
"X XXXX X",
"X XXXX X",
"X  XX  X",
"X      X",
"XXXXXXXX",
};

const char **wxMonoRenderer::ms_xpmIndicators[IndicatorType_MaxCtrl]
                                             [IndicatorState_MaxCtrl]
                                             [IndicatorStatus_Max] =
{
    // checkboxes first
    {
        // normal state
        { xpmChecked, xpmUnchecked, xpmUndeterminate },

        // pressed state
        { xpmUndeterminate, xpmUndeterminate, xpmUndeterminate },

        // disabled state
        { xpmUndeterminate, xpmUndeterminate, xpmUndeterminate },
    },

    // radio
    {
        // normal state
        { xpmRadioChecked, xpmRadioUnchecked, xpmUndeterminate },

        // pressed state
        { xpmUndeterminate, xpmUndeterminate, xpmUndeterminate },

        // disabled state
        { xpmUndeterminate, xpmUndeterminate, xpmUndeterminate },
    },
};

static const char *xpmLeftArrow[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"   X    ",
"  XX    ",
" XXX    ",
"XXXX    ",
"XXXX    ",
" XXX    ",
"  XX    ",
"   X    ",
};

static const char *xpmRightArrow[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"    X   ",
"    XX  ",
"    XXX ",
"    XXXX",
"    XXXX",
"    XXX ",
"    XX  ",
"    X   ",
};

static const char *xpmUpArrow[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"        ",
"   XX   ",
"  XXXX  ",
" XXXXXX ",
"XXXXXXXX",
"        ",
"        ",
"        ",
};

static const char *xpmDownArrow[] = {
/* columns rows colors chars-per-pixel */
"8 8 2 1",
"  c white",
"X c black",
/* pixels */
"        ",
"        ",
"        ",
"XXXXXXXX",
" XXXXXX ",
"  XXXX  ",
"   XX   ",
"        ",
};

const char **wxMonoRenderer::ms_xpmArrows[Arrow_Max] =
{
    xpmLeftArrow, xpmRightArrow, xpmUpArrow, xpmDownArrow,
};

// ----------------------------------------------------------------------------
// wxMonoColourScheme: uses just white and black
// ----------------------------------------------------------------------------

class wxMonoColourScheme : public wxColourScheme
{
public:
    // we use only 2 colours, white and black, but we avoid referring to them
    // like this, instead use the functions below
    wxColour GetFg() const { return wxMONO_FG_COL; }
    wxColour GetBg() const { return wxMONO_BG_COL; }

    // implement base class pure virtuals
    virtual wxColour Get(StdColour col) const;
    virtual wxColour GetBackground(wxWindow *win) const;
};

// ----------------------------------------------------------------------------
// wxMonoArtProvider
// ----------------------------------------------------------------------------

class wxMonoArtProvider : public wxArtProvider
{
protected:
    virtual wxBitmap CreateBitmap(const wxArtID& id,
                                  const wxArtClient& client,
                                  const wxSize& size);
};

// ----------------------------------------------------------------------------
// wxMonoTheme
// ----------------------------------------------------------------------------

class wxMonoTheme : public wxTheme
{
public:
    wxMonoTheme();
    virtual ~wxMonoTheme();

    virtual wxRenderer *GetRenderer();
    virtual wxArtProvider *GetArtProvider();
    virtual wxInputHandler *GetInputHandler(const wxString& control,
                                            wxInputConsumer *consumer);
    virtual wxColourScheme *GetColourScheme();

private:
    wxMonoRenderer *m_renderer;
    wxMonoArtProvider *m_artProvider;
    wxMonoColourScheme *m_scheme;

    WX_DECLARE_THEME(mono)
};

// ============================================================================
// implementation
// ============================================================================

WX_IMPLEMENT_THEME(wxMonoTheme, mono, wxTRANSLATE("Simple monochrome theme"));

// ----------------------------------------------------------------------------
// wxMonoTheme
// ----------------------------------------------------------------------------

wxMonoTheme::wxMonoTheme()
{
    m_scheme = NULL;
    m_renderer = NULL;
    m_artProvider = NULL;
}

wxMonoTheme::~wxMonoTheme()
{
    delete m_renderer;
    delete m_scheme;
    delete m_artProvider;
}

wxRenderer *wxMonoTheme::GetRenderer()
{
    if ( !m_renderer )
    {
        m_renderer = new wxMonoRenderer(GetColourScheme());
    }

    return m_renderer;
}

wxArtProvider *wxMonoTheme::GetArtProvider()
{
    if ( !m_artProvider )
    {
        m_artProvider = new wxMonoArtProvider;
    }

    return m_artProvider;
}

wxColourScheme *wxMonoTheme::GetColourScheme()
{
    if ( !m_scheme )
    {
        m_scheme = new wxMonoColourScheme;
    }

    return m_scheme;
}

wxInputHandler *wxMonoTheme::GetInputHandler(const wxString& WXUNUSED(control),
                                             wxInputConsumer *consumer)
{
    // no special input handlers so far
    return consumer->DoGetStdInputHandler(NULL);
}

// ============================================================================
// wxMonoColourScheme
// ============================================================================

wxColour wxMonoColourScheme::GetBackground(wxWindow *win) const
{
    wxColour col;
    if ( win->UseBgCol() )
    {
        // use the user specified colour
        col = win->GetBackgroundColour();
    }

    // doesn't depend on the state
    if ( !col.Ok() )
    {
        col = GetBg();
    }

    return col;
}

wxColour wxMonoColourScheme::Get(wxMonoColourScheme::StdColour col) const
{
    switch ( col )
    {
        case WINDOW:
        case CONTROL:
        case CONTROL_PRESSED:
        case CONTROL_CURRENT:
        case SCROLLBAR:
        case SCROLLBAR_PRESSED:
        case GAUGE:
        case TITLEBAR:
        case TITLEBAR_ACTIVE:
        case HIGHLIGHT_TEXT:
        case DESKTOP:
        case FRAME:
            return GetBg();

        case MAX:
        default:
            wxFAIL_MSG(_T("invalid standard colour"));
            // fall through

        case SHADOW_DARK:
        case SHADOW_HIGHLIGHT:
        case SHADOW_IN:
        case SHADOW_OUT:
        case CONTROL_TEXT:
        case CONTROL_TEXT_DISABLED:
        case CONTROL_TEXT_DISABLED_SHADOW:
        case TITLEBAR_TEXT:
        case TITLEBAR_ACTIVE_TEXT:
        case HIGHLIGHT:
            return GetFg();

    }
}

// ============================================================================
// wxMonoRenderer
// ============================================================================

// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------

wxMonoRenderer::wxMonoRenderer(const wxColourScheme *scheme)
              : wxStdRenderer(scheme)
{
    m_penFg = wxPen(wxMONO_FG_COL);
}

// ----------------------------------------------------------------------------
// borders
// ----------------------------------------------------------------------------

wxRect wxMonoRenderer::GetBorderDimensions(wxBorder border) const
{
    wxCoord width;
    switch ( border )
    {
        case wxBORDER_SIMPLE:
        case wxBORDER_STATIC:
        case wxBORDER_RAISED:
        case wxBORDER_SUNKEN:
            width = 1;
            break;

        case wxBORDER_DOUBLE:
            width = 2;
            break;

        default:
            wxFAIL_MSG(_T("unknown border type"));
            // fall through

        case wxBORDER_DEFAULT:
        case wxBORDER_NONE:
            width = 0;
            break;
    }

    wxRect rect;
    rect.x =
    rect.y =
    rect.width =
    rect.height = width;

    return rect;
}

void wxMonoRenderer::DrawButtonBorder(wxDC& dc,
                                     const wxRect& rect,
                                     int flags,
                                     wxRect *rectIn)
{
    DrawBorder(dc, wxBORDER_SIMPLE, rect, flags, rectIn);
}

// ----------------------------------------------------------------------------
// lines and frames
// ----------------------------------------------------------------------------

void
wxMonoRenderer::DrawHorizontalLine(wxDC& dc, wxCoord y, wxCoord x1, wxCoord x2)
{
    dc.SetPen(m_penFg);
    dc.DrawLine(x1, y, x2 + 1, y);
}

void
wxMonoRenderer::DrawVerticalLine(wxDC& dc, wxCoord x, wxCoord y1, wxCoord y2)
{
    dc.SetPen(m_penFg);
    dc.DrawLine(x, y1, x, y2 + 1);
}

void wxMonoRenderer::DrawFocusRect(wxDC& dc, const wxRect& rect, int flags)
{
    // no need to draw the focus rect for selected items, it would be invisible
    // anyhow
    if ( !(flags & wxCONTROL_SELECTED) )
    {
        dc.SetPen(m_penFg);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
    }
}

// ----------------------------------------------------------------------------
// label
// ----------------------------------------------------------------------------

void wxMonoRenderer::DrawButtonLabel(wxDC& dc,
                                     const wxString& label,
                                     const wxBitmap& image,
                                     const wxRect& rect,
                                     int flags,
                                     int alignment,
                                     int indexAccel,
                                     wxRect *rectBounds)
{
    dc.DrawLabel(label, image, rect, alignment, indexAccel, rectBounds);

    if ( flags & wxCONTROL_DISABLED )
    {
        // this is ugly but I don't know how to show disabled button visually
        // in monochrome theme otherwise, so cross it out
        dc.SetPen(m_penFg);
        dc.DrawLine(rect.GetTopLeft(), rect.GetBottomRight());
        dc.DrawLine(rect.GetTopRight(), rect.GetBottomLeft());
    }
}

// ----------------------------------------------------------------------------
// bitmaps
// ----------------------------------------------------------------------------

wxBitmap wxMonoRenderer::GetIndicator(IndicatorType indType, int flags)
{
    IndicatorState indState;
    IndicatorStatus indStatus;
    GetIndicatorsFromFlags(flags, indState, indStatus);

    wxBitmap& bmp = m_bmpIndicators[indType][indState][indStatus];
    if ( !bmp.Ok() )
    {
        const char **xpm = ms_xpmIndicators[indType][indState][indStatus];
        if ( xpm )
        {
            // create and cache it
            bmp = wxBitmap(xpm);
        }
    }

    return bmp;
}

wxBitmap wxMonoRenderer::GetFrameButtonBitmap(FrameButtonType type)
{
    if ( type == FrameButton_Close )
    {
        if ( !m_bmpFrameClose.Ok() )
        {
            static const char *xpmFrameClose[] = {
            /* columns rows colors chars-per-pixel */
            "8 8 2 1",
            "  c white",
            "X c black",
            /* pixels */
            "        ",
            " XX  XX ",
            "  X  X  ",
            "   XX   ",
            "   XX   ",
            "  X  X  ",
            " XX  XX ",
            "        ",
            };

            m_bmpFrameClose = wxBitmap(xpmFrameClose);
        }

        return m_bmpFrameClose;
    }

    // we don't show any other buttons than close
    return wxNullBitmap;
}

// ----------------------------------------------------------------------------
// toolbar
// ----------------------------------------------------------------------------

#if wxUSE_TOOLBAR

void wxMonoRenderer::DrawToolBarButton(wxDC& WXUNUSED(dc),
                                       const wxString& WXUNUSED(label),
                                       const wxBitmap& WXUNUSED(bitmap),
                                       const wxRect& WXUNUSED(rect),
                                       int WXUNUSED(flags),
                                       long WXUNUSED(style),
                                       int WXUNUSED(tbarStyle))
{
    wxFAIL_MSG(_T("TODO"));
}

wxSize wxMonoRenderer::GetToolBarButtonSize(wxCoord *WXUNUSED(separator)) const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

wxSize wxMonoRenderer::GetToolBarMargin() const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

#endif // wxUSE_TOOLBAR

// ----------------------------------------------------------------------------
// notebook
// ----------------------------------------------------------------------------

#if wxUSE_NOTEBOOK

void wxMonoRenderer::DrawTab(wxDC& WXUNUSED(dc),
                             const wxRect& WXUNUSED(rect),
                             wxDirection WXUNUSED(dir),
                             const wxString& WXUNUSED(label),
                             const wxBitmap& WXUNUSED(bitmap),
                             int WXUNUSED(flags),
                             int WXUNUSED(indexAccel))
{
    wxFAIL_MSG(_T("TODO"));
}

wxSize wxMonoRenderer::GetTabIndent() const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

wxSize wxMonoRenderer::GetTabPadding() const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

#endif // wxUSE_NOTEBOOK

// ----------------------------------------------------------------------------
// combobox
// ----------------------------------------------------------------------------

#if wxUSE_COMBOBOX

void wxMonoRenderer::GetComboBitmaps(wxBitmap *WXUNUSED(bmpNormal),
                                     wxBitmap *WXUNUSED(bmpFocus),
                                     wxBitmap *WXUNUSED(bmpPressed),
                                     wxBitmap *WXUNUSED(bmpDisabled))
{
    wxFAIL_MSG(_T("TODO"));
}

#endif // wxUSE_COMBOBOX

// ----------------------------------------------------------------------------
// menus
// ----------------------------------------------------------------------------

#if wxUSE_MENUS

void wxMonoRenderer::DrawMenuBarItem(wxDC& WXUNUSED(dc),
                                     const wxRect& WXUNUSED(rect),
                                     const wxString& WXUNUSED(label),
                                     int WXUNUSED(flags),
                                     int WXUNUSED(indexAccel))
{
    wxFAIL_MSG(_T("TODO"));
}

void wxMonoRenderer::DrawMenuItem(wxDC& WXUNUSED(dc),
                                  wxCoord WXUNUSED(y),
                                  const wxMenuGeometryInfo& WXUNUSED(geometryInfo),
                                  const wxString& WXUNUSED(label),
                                  const wxString& WXUNUSED(accel),
                                  const wxBitmap& WXUNUSED(bitmap),
                                  int WXUNUSED(flags),
                                  int WXUNUSED(indexAccel))
{
    wxFAIL_MSG(_T("TODO"));
}

void wxMonoRenderer::DrawMenuSeparator(wxDC& WXUNUSED(dc),
                                       wxCoord WXUNUSED(y),
                                       const wxMenuGeometryInfo& WXUNUSED(geomInfo))
{
    wxFAIL_MSG(_T("TODO"));
}

wxSize wxMonoRenderer::GetMenuBarItemSize(const wxSize& WXUNUSED(sizeText)) const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

wxMenuGeometryInfo *wxMonoRenderer::GetMenuGeometry(wxWindow *WXUNUSED(win),
                                                    const wxMenu& WXUNUSED(menu)) const
{
    wxFAIL_MSG(_T("TODO"));

    return NULL;
}

#endif // wxUSE_MENUS

// ----------------------------------------------------------------------------
// slider
// ----------------------------------------------------------------------------

#if wxUSE_SLIDER

void wxMonoRenderer::DrawSliderShaft(wxDC& WXUNUSED(dc),
                                     const wxRect& WXUNUSED(rect),
                                     int WXUNUSED(lenThumb),
                                     wxOrientation WXUNUSED(orient),
                                     int WXUNUSED(flags),
                                     long WXUNUSED(style),
                                     wxRect *WXUNUSED(rectShaft))
{
    wxFAIL_MSG(_T("TODO"));
}


void wxMonoRenderer::DrawSliderThumb(wxDC& WXUNUSED(dc),
                                     const wxRect& WXUNUSED(rect),
                                     wxOrientation WXUNUSED(orient),
                                     int WXUNUSED(flags),
                                     long WXUNUSED(style))
{
    wxFAIL_MSG(_T("TODO"));
}

void wxMonoRenderer::DrawSliderTicks(wxDC& WXUNUSED(dc),
                                     const wxRect& WXUNUSED(rect),
                                     int WXUNUSED(lenThumb),
                                     wxOrientation WXUNUSED(orient),
                                     int WXUNUSED(start),
                                     int WXUNUSED(end),
                                     int WXUNUSED(step),
                                     int WXUNUSED(flags),
                                     long WXUNUSED(style))
{
    wxFAIL_MSG(_T("TODO"));
}

wxCoord wxMonoRenderer::GetSliderDim() const
{
    wxFAIL_MSG(_T("TODO"));

    return 0;
}

wxCoord wxMonoRenderer::GetSliderTickLen() const
{
    wxFAIL_MSG(_T("TODO"));

    return 0;
}


wxRect wxMonoRenderer::GetSliderShaftRect(const wxRect& WXUNUSED(rect),
                                          int WXUNUSED(lenThumb),
                                          wxOrientation WXUNUSED(orient),
                                          long WXUNUSED(style)) const
{
    wxFAIL_MSG(_T("TODO"));

    return wxRect();
}

wxSize wxMonoRenderer::GetSliderThumbSize(const wxRect& WXUNUSED(rect),
                                          int WXUNUSED(lenThumb),
                                          wxOrientation WXUNUSED(orient)) const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}

#endif // wxUSE_SLIDER

wxSize wxMonoRenderer::GetProgressBarStep() const
{
    wxFAIL_MSG(_T("TODO"));

    return wxSize();
}


// ----------------------------------------------------------------------------
// scrollbar
// ----------------------------------------------------------------------------

void wxMonoRenderer::DrawArrow(wxDC& dc,
                               wxDirection dir,
                               const wxRect& rect,
                               int WXUNUSED(flags))
{
    ArrowDirection arrowDir = GetArrowDirection(dir);
    wxCHECK_RET( arrowDir != Arrow_Max, _T("invalid arrow direction") );

    wxBitmap& bmp = m_bmpArrows[arrowDir];
    if ( !bmp.Ok() )
    {
        bmp = wxBitmap(ms_xpmArrows[arrowDir]);
    }

    wxRect rectArrow(0, 0, bmp.GetWidth(), bmp.GetHeight());
    dc.DrawBitmap(bmp, rectArrow.CentreIn(rect).GetPosition(), true /* use mask */);
}

void wxMonoRenderer::DrawScrollbarThumb(wxDC& dc,
                                        wxOrientation WXUNUSED(orient),
                                        const wxRect& rect,
                                        int WXUNUSED(flags))
{
    DrawSolidRect(dc, wxMONO_BG_COL, rect);

    // manually draw stipple pattern (wxDFB doesn't implement the wxSTIPPLE
    // brush style):
    dc.SetPen(m_penFg);
    for ( wxCoord y = rect.GetTop(); y <= rect.GetBottom(); y++ )
    {
        for ( wxCoord x = rect.GetLeft() + (y % 2); x <= rect.GetRight(); x+=2 )
        {
            dc.DrawPoint(x, y);
        }
    }
}

void wxMonoRenderer::DrawScrollbarShaft(wxDC& dc,
                                        wxOrientation WXUNUSED(orient),
                                        const wxRect& rect,
                                        int WXUNUSED(flags))
{
    DrawSolidRect(dc, wxMONO_BG_COL, rect);
}

// ----------------------------------------------------------------------------
// status bar
// ----------------------------------------------------------------------------

#if wxUSE_STATUSBAR

wxCoord wxMonoRenderer::GetStatusBarBorderBetweenFields() const
{
    return 1;
}

wxSize wxMonoRenderer::GetStatusBarFieldMargins() const
{
    return wxSize(1, 1);
}

#endif // wxUSE_STATUSBAR

// ----------------------------------------------------------------------------
// top level windows
// ----------------------------------------------------------------------------

int wxMonoRenderer::GetFrameBorderWidth(int WXUNUSED(flags)) const
{
    // all our borders are simple
    return 1;
}

// ----------------------------------------------------------------------------
// wxMonoArtProvider
// ----------------------------------------------------------------------------

wxBitmap wxMonoArtProvider::CreateBitmap(const wxArtID& WXUNUSED(id),
                                         const wxArtClient& WXUNUSED(client),
                                         const wxSize& WXUNUSED(size))
{
    return wxNullBitmap;
}

#endif // wxUSE_THEME_MONO
