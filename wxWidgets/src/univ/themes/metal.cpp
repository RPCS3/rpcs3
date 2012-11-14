///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/themes/metal.cpp
// Purpose:     wxUniversal theme implementing Win32-like LNF
// Author:      Vadim Zeitlin, Robert Roebling
// Modified by:
// Created:     06.08.00
// RCS-ID:      $Id: metal.cpp 42455 2006-10-26 15:33:10Z VS $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#include "wx/univ/theme.h"

#if wxUSE_THEME_METAL

#ifndef WX_PRECOMP
    #include "wx/timer.h"
    #include "wx/intl.h"
    #include "wx/dc.h"
    #include "wx/window.h"

    #include "wx/dcmemory.h"

    #include "wx/button.h"
    #include "wx/listbox.h"
    #include "wx/checklst.h"
    #include "wx/combobox.h"
    #include "wx/scrolbar.h"
    #include "wx/slider.h"
    #include "wx/textctrl.h"
    #include "wx/toolbar.h"

    #include "wx/menu.h"
    #include "wx/settings.h"
    #include "wx/toplevel.h"
#endif // WX_PRECOMP

#include "wx/notebook.h"
#include "wx/spinbutt.h"
#include "wx/artprov.h"

#include "wx/univ/scrtimer.h"
#include "wx/univ/renderer.h"
#include "wx/univ/inpcons.h"
#include "wx/univ/inphand.h"
#include "wx/univ/colschem.h"

// ----------------------------------------------------------------------------
// wxMetalRenderer: draw the GUI elements in Metal style
// ----------------------------------------------------------------------------

class wxMetalRenderer : public wxDelegateRenderer
{
    // FIXME cut'n'paste from Win32
    enum wxArrowDirection
    {
        Arrow_Left,
        Arrow_Right,
        Arrow_Up,
        Arrow_Down,
        Arrow_Max
    };

    enum wxArrowStyle
    {
        Arrow_Normal,
        Arrow_Disabled,
        Arrow_Pressed,
        Arrow_Inverted,
        Arrow_InvertedDisabled,
        Arrow_StateMax
    };
public:
    wxMetalRenderer(wxRenderer *renderer, wxColourScheme* scheme);

    virtual void DrawButtonSurface(wxDC& dc,
                                   const wxColour& WXUNUSED(col),
                                   const wxRect& rect,
                                   int WXUNUSED(flags))
        { DrawMetal(dc, rect); }

    virtual void DrawScrollbarThumb(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rect,
                                    int flags);

    virtual void DrawScrollbarShaft(wxDC& dc,
                                    wxOrientation orient,
                                    const wxRect& rectBar,
                                    int flags);

    virtual void GetComboBitmaps(wxBitmap *bmpNormal,
                                 wxBitmap *bmpFocus,
                                 wxBitmap *bmpPressed,
                                 wxBitmap *bmpDisabled);

    virtual void DrawArrow(wxDC& dc,
                           wxDirection dir,
                           const wxRect& rect,
                           int flags = 0);
protected:
    void DrawArrowButton(wxDC& dc,
                         const wxRect& rectAll,
                         wxArrowDirection arrowDir,
                         wxArrowStyle arrowStyle);

    void DrawRect(wxDC& dc, wxRect *rect, const wxPen& pen);

    void DrawShadedRect(wxDC& dc, wxRect *rect,
                        const wxPen& pen1, const wxPen& pen2);

    void DrawArrowBorder(wxDC& dc, wxRect *rect, bool isPressed = false);

    void DrawArrow(wxDC& dc, const wxRect& rect,
                   wxArrowDirection arrowDir, wxArrowStyle arrowStyle);

    void DrawMetal(wxDC &dc, const wxRect &rect );
private:
    wxPen m_penBlack,
          m_penDarkGrey,
          m_penLightGrey,
          m_penHighlight;

    wxBitmap m_bmpArrows[Arrow_StateMax][Arrow_Max];
};

// ----------------------------------------------------------------------------
// wxMetalTheme
// ----------------------------------------------------------------------------

class wxMetalTheme : public wxDelegateTheme
{
public:
    wxMetalTheme() : wxDelegateTheme(_T("win32")), m_renderer(NULL) {}
    ~wxMetalTheme() { delete m_renderer; }

protected:
    virtual wxRenderer *GetRenderer()
    {
        if ( !m_renderer )
        {
            m_renderer = new wxMetalRenderer(m_theme->GetRenderer(),
                                             GetColourScheme());
        }

        return m_renderer;
    }

    wxRenderer *m_renderer;

    WX_DECLARE_THEME(Metal)
};

WX_IMPLEMENT_THEME(wxMetalTheme, Metal, wxTRANSLATE("Metal theme"));


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMetalRenderer
// ----------------------------------------------------------------------------

wxMetalRenderer::wxMetalRenderer(wxRenderer *renderer, wxColourScheme *scheme)
               : wxDelegateRenderer(renderer)
{
    // init colours and pens
    m_penBlack = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_DARK), 0, wxSOLID);
    m_penDarkGrey = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_OUT), 0, wxSOLID);
    m_penLightGrey = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_IN), 0, wxSOLID);
    m_penHighlight = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_HIGHLIGHT), 0, wxSOLID);

    // init the arrow bitmaps
    static const size_t ARROW_WIDTH = 7;
    static const size_t ARROW_LENGTH = 4;

    wxMask *mask;
    wxMemoryDC dcNormal,
               dcDisabled,
               dcInverse;
    for ( size_t n = 0; n < Arrow_Max; n++ )
    {
        bool isVertical = n > Arrow_Right;
        int w, h;
        if ( isVertical )
        {
            w = ARROW_WIDTH;
            h = ARROW_LENGTH;
        }
        else
        {
            h = ARROW_WIDTH;
            w = ARROW_LENGTH;
        }

        // disabled arrow is larger because of the shadow
        m_bmpArrows[Arrow_Normal][n].Create(w, h);
        m_bmpArrows[Arrow_Disabled][n].Create(w + 1, h + 1);

        dcNormal.SelectObject(m_bmpArrows[Arrow_Normal][n]);
        dcDisabled.SelectObject(m_bmpArrows[Arrow_Disabled][n]);

        dcNormal.SetBackground(*wxWHITE_BRUSH);
        dcDisabled.SetBackground(*wxWHITE_BRUSH);
        dcNormal.Clear();
        dcDisabled.Clear();

        dcNormal.SetPen(m_penBlack);
        dcDisabled.SetPen(m_penDarkGrey);

        // calculate the position of the point of the arrow
        wxCoord x1, y1;
        if ( isVertical )
        {
            x1 = (ARROW_WIDTH - 1)/2;
            y1 = n == Arrow_Up ? 0 : ARROW_LENGTH - 1;
        }
        else // horizontal
        {
            x1 = n == Arrow_Left ? 0 : ARROW_LENGTH - 1;
            y1 = (ARROW_WIDTH - 1)/2;
        }

        wxCoord x2 = x1,
                y2 = y1;

        if ( isVertical )
            x2++;
        else
            y2++;

        for ( size_t i = 0; i < ARROW_LENGTH; i++ )
        {
            dcNormal.DrawLine(x1, y1, x2, y2);
            dcDisabled.DrawLine(x1, y1, x2, y2);

            if ( isVertical )
            {
                x1--;
                x2++;

                if ( n == Arrow_Up )
                {
                    y1++;
                    y2++;
                }
                else // down arrow
                {
                    y1--;
                    y2--;
                }
            }
            else // left or right arrow
            {
                y1--;
                y2++;

                if ( n == Arrow_Left )
                {
                    x1++;
                    x2++;
                }
                else
                {
                    x1--;
                    x2--;
                }
            }
        }

        // draw the shadow for the disabled one
        dcDisabled.SetPen(m_penHighlight);
        switch ( n )
        {
            case Arrow_Left:
                y1 += 2;
                dcDisabled.DrawLine(x1, y1, x2, y2);
                break;

            case Arrow_Right:
                x1 = ARROW_LENGTH - 1;
                y1 = (ARROW_WIDTH - 1)/2 + 1;
                x2 = 0;
                y2 = ARROW_WIDTH;
                dcDisabled.DrawLine(x1, y1, x2, y2);
                dcDisabled.DrawLine(++x1, y1, x2, ++y2);
                break;

            case Arrow_Up:
                x1 += 2;
                dcDisabled.DrawLine(x1, y1, x2, y2);
                break;

            case Arrow_Down:
                x1 = ARROW_WIDTH - 1;
                y1 = 1;
                x2 = (ARROW_WIDTH - 1)/2;
                y2 = ARROW_LENGTH;
                dcDisabled.DrawLine(x1, y1, x2, y2);
                dcDisabled.DrawLine(++x1, y1, x2, ++y2);
                break;

        }

        // create the inverted bitmap but only for the right arrow as we only
        // use it for the menus
        if ( n == Arrow_Right )
        {
            m_bmpArrows[Arrow_Inverted][n].Create(w, h);
            dcInverse.SelectObject(m_bmpArrows[Arrow_Inverted][n]);
            dcInverse.Clear();
            dcInverse.Blit(0, 0, w, h,
                          &dcNormal, 0, 0,
                          wxXOR);
            dcInverse.SelectObject(wxNullBitmap);

            mask = new wxMask(m_bmpArrows[Arrow_Inverted][n], *wxBLACK);
            m_bmpArrows[Arrow_Inverted][n].SetMask(mask);

            m_bmpArrows[Arrow_InvertedDisabled][n].Create(w, h);
            dcInverse.SelectObject(m_bmpArrows[Arrow_InvertedDisabled][n]);
            dcInverse.Clear();
            dcInverse.Blit(0, 0, w, h,
                          &dcDisabled, 0, 0,
                          wxXOR);
            dcInverse.SelectObject(wxNullBitmap);

            mask = new wxMask(m_bmpArrows[Arrow_InvertedDisabled][n], *wxBLACK);
            m_bmpArrows[Arrow_InvertedDisabled][n].SetMask(mask);
        }

        dcNormal.SelectObject(wxNullBitmap);
        dcDisabled.SelectObject(wxNullBitmap);

        mask = new wxMask(m_bmpArrows[Arrow_Normal][n], *wxWHITE);
        m_bmpArrows[Arrow_Normal][n].SetMask(mask);
        mask = new wxMask(m_bmpArrows[Arrow_Disabled][n], *wxWHITE);
        m_bmpArrows[Arrow_Disabled][n].SetMask(mask);

        m_bmpArrows[Arrow_Pressed][n] = m_bmpArrows[Arrow_Normal][n];
    }
}

void wxMetalRenderer::DrawScrollbarThumb(wxDC& dc,
                                         wxOrientation WXUNUSED(orient),
                                         const wxRect& rect,
                                         int WXUNUSED(flags))
{
    // we don't use the flags, the thumb never changes appearance
    wxRect rectThumb = rect;
    DrawArrowBorder(dc, &rectThumb);
    DrawMetal(dc, rectThumb);
}

void wxMetalRenderer::DrawScrollbarShaft(wxDC& dc,
                                         wxOrientation WXUNUSED(orient),
                                         const wxRect& rectBar,
                                         int WXUNUSED(flags))
{
    DrawMetal(dc, rectBar);
}

void wxMetalRenderer::GetComboBitmaps(wxBitmap *bmpNormal,
                                      wxBitmap * WXUNUSED(bmpFocus),
                                      wxBitmap *bmpPressed,
                                      wxBitmap *bmpDisabled)
{
    static const wxCoord widthCombo = 16;
    static const wxCoord heightCombo = 17;

    wxMemoryDC dcMem;

    if ( bmpNormal )
    {
        bmpNormal->Create(widthCombo, heightCombo);
        dcMem.SelectObject(*bmpNormal);
        DrawArrowButton(dcMem, wxRect(0, 0, widthCombo, heightCombo),
                        Arrow_Down, Arrow_Normal);
    }

    if ( bmpPressed )
    {
        bmpPressed->Create(widthCombo, heightCombo);
        dcMem.SelectObject(*bmpPressed);
        DrawArrowButton(dcMem, wxRect(0, 0, widthCombo, heightCombo),
                        Arrow_Down, Arrow_Pressed);
    }

    if ( bmpDisabled )
    {
        bmpDisabled->Create(widthCombo, heightCombo);
        dcMem.SelectObject(*bmpDisabled);
        DrawArrowButton(dcMem, wxRect(0, 0, widthCombo, heightCombo),
                        Arrow_Down, Arrow_Disabled);
    }
}

void wxMetalRenderer::DrawArrow(wxDC& dc,
                                wxDirection dir,
                                const wxRect& rect,
                                int flags)
{
    // get the bitmap for this arrow
    wxArrowDirection arrowDir;
    switch ( dir )
    {
        case wxLEFT:    arrowDir = Arrow_Left; break;
        case wxRIGHT:   arrowDir = Arrow_Right; break;
        case wxUP:      arrowDir = Arrow_Up; break;
        case wxDOWN:    arrowDir = Arrow_Down; break;

        default:
            wxFAIL_MSG(_T("unknown arrow direction"));
            return;
    }

    wxArrowStyle arrowStyle;
    if ( flags & wxCONTROL_PRESSED )
    {
        // can't be pressed and disabled
        arrowStyle = Arrow_Pressed;
    }
    else
    {
        arrowStyle = flags & wxCONTROL_DISABLED ? Arrow_Disabled : Arrow_Normal;
    }

    DrawArrowButton(dc, rect, arrowDir, arrowStyle);
}

//
// protected functions
//

void wxMetalRenderer::DrawArrowButton(wxDC& dc,
                                      const wxRect& rectAll,
                                      wxArrowDirection arrowDir,
                                      wxArrowStyle arrowStyle)
{
    wxRect rect = rectAll;
    DrawMetal( dc, rect );
    DrawArrowBorder(dc, &rect, arrowStyle == Arrow_Pressed);
    DrawArrow(dc, rect, arrowDir, arrowStyle);
}

void wxMetalRenderer::DrawRect(wxDC& dc, wxRect *rect, const wxPen& pen)
{
    // draw
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(*rect);

    // adjust the rect
    rect->Inflate(-1);
}

void wxMetalRenderer::DrawShadedRect(wxDC& dc, wxRect *rect,
                                     const wxPen& pen1, const wxPen& pen2)
{
    // draw the rectangle
    dc.SetPen(pen1);
    dc.DrawLine(rect->GetLeft(), rect->GetTop(),
                rect->GetLeft(), rect->GetBottom());
    dc.DrawLine(rect->GetLeft() + 1, rect->GetTop(),
                rect->GetRight(), rect->GetTop());
    dc.SetPen(pen2);
    dc.DrawLine(rect->GetRight(), rect->GetTop(),
                rect->GetRight(), rect->GetBottom());
    dc.DrawLine(rect->GetLeft(), rect->GetBottom(),
                rect->GetRight() + 1, rect->GetBottom());

    // adjust the rect
    rect->Inflate(-1);
}

void wxMetalRenderer::DrawArrowBorder(wxDC& dc, wxRect *rect, bool isPressed)
{
    if ( isPressed )
    {
        DrawRect(dc, rect, m_penDarkGrey);

        // the arrow is usually drawn inside border of width 2 and is offset by
        // another pixel in both directions when it's pressed - as the border
        // in this case is more narrow as well, we have to adjust rect like
        // this:
        rect->Inflate(-1);
        rect->x++;
        rect->y++;
    }
    else
    {
        DrawShadedRect(dc, rect, m_penLightGrey, m_penBlack);
        DrawShadedRect(dc, rect, m_penHighlight, m_penDarkGrey);
    }
}

void wxMetalRenderer::DrawArrow(wxDC& dc,
                                const wxRect& rect,
                                wxArrowDirection arrowDir,
                                wxArrowStyle arrowStyle)
{
    const wxBitmap& bmp = m_bmpArrows[arrowStyle][arrowDir];

    // under Windows the arrows always have the same size so just centre it in
    // the provided rectangle
    wxCoord x = rect.x + (rect.width - bmp.GetWidth()) / 2,
            y = rect.y + (rect.height - bmp.GetHeight()) / 2;

    // Windows does it like this...
    if ( arrowDir == Arrow_Left )
        x--;

    // draw it
    dc.DrawBitmap(bmp, x, y, true /* use mask */);
}

// ----------------------------------------------------------------------------
// metal gradient
// ----------------------------------------------------------------------------

void wxMetalRenderer::DrawMetal(wxDC &dc, const wxRect &rect )
{
    dc.SetPen(*wxTRANSPARENT_PEN);
    for (int y = rect.y; y < rect.height+rect.y; y++)
    {
       unsigned char intens = (unsigned char)(230 + 80 * (rect.y-y) / rect.height);
       dc.SetBrush( wxBrush( wxColour(intens,intens,intens), wxSOLID ) );
       dc.DrawRectangle( rect.x, y, rect.width, 1 );
    }
}

#endif // wxUSE_THEME_METAL
