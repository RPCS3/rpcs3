///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/ctrlrend.cpp
// Purpose:     wxControlRenderer implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.08.00
// RCS-ID:      $Id: ctrlrend.cpp 42716 2006-10-30 12:33:25Z VS $
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

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/control.h"
    #include "wx/checklst.h"
    #include "wx/listbox.h"
    #include "wx/scrolbar.h"
    #include "wx/dc.h"
    #include "wx/log.h"
    #include "wx/gauge.h"
    #include "wx/image.h"
#endif // WX_PRECOMP

#include "wx/univ/theme.h"
#include "wx/univ/renderer.h"
#include "wx/univ/colschem.h"

// ============================================================================
// implementation
// ============================================================================

wxRenderer::~wxRenderer()
{
}

// ----------------------------------------------------------------------------
// wxControlRenderer
// ----------------------------------------------------------------------------

wxControlRenderer::wxControlRenderer(wxWindow *window,
                                     wxDC& dc,
                                     wxRenderer *renderer)
                : m_dc(dc)
{
    m_window = window;
    m_renderer = renderer;

    wxSize size = m_window->GetClientSize();
    m_rect.x =
    m_rect.y = 0;
    m_rect.width = size.x;
    m_rect.height = size.y;
}

void wxControlRenderer::DrawLabel(const wxBitmap& bitmap,
                                  wxCoord marginX, wxCoord marginY)
{
    m_dc.SetBackgroundMode(wxTRANSPARENT);
    m_dc.SetFont(m_window->GetFont());
    m_dc.SetTextForeground(m_window->GetForegroundColour());

    wxString label = m_window->GetLabel();
    if ( !label.empty() || bitmap.Ok() )
    {
        wxRect rectLabel = m_rect;
        if ( bitmap.Ok() )
        {
            rectLabel.Inflate(-marginX, -marginY);
        }

        wxControl *ctrl = wxStaticCast(m_window, wxControl);

        m_renderer->DrawButtonLabel(m_dc,
                                    label,
                                    bitmap,
                                    rectLabel,
                                    m_window->GetStateFlags(),
                                    ctrl->GetAlignment(),
                                    ctrl->GetAccelIndex());
    }
}

void wxControlRenderer::DrawFrame()
{
    m_dc.SetFont(m_window->GetFont());
    m_dc.SetTextForeground(m_window->GetForegroundColour());
    m_dc.SetTextBackground(m_window->GetBackgroundColour());

    wxControl *ctrl = wxStaticCast(m_window, wxControl);

    m_renderer->DrawFrame(m_dc,
                          m_window->GetLabel(),
                          m_rect,
                          m_window->GetStateFlags(),
                          ctrl->GetAlignment(),
                          ctrl->GetAccelIndex());
}

void wxControlRenderer::DrawButtonBorder()
{
    int flags = m_window->GetStateFlags();

    m_renderer->DrawButtonBorder(m_dc, m_rect, flags, &m_rect);

    // Why do this here?
    // m_renderer->DrawButtonSurface(m_dc, wxTHEME_BG_COLOUR(m_window), m_rect, flags );
}

void wxControlRenderer::DrawBitmap(const wxBitmap& bitmap)
{
    int style = m_window->GetWindowStyle();
    DrawBitmap(m_dc, bitmap, m_rect,
               style & wxALIGN_MASK,
               style & wxBI_EXPAND ? wxEXPAND : wxSTRETCH_NOT);
}

/* static */
void wxControlRenderer::DrawBitmap(wxDC &dc,
                                   const wxBitmap& bitmap,
                                   const wxRect& rect,
                                   int alignment,
                                   wxStretch stretch)
{
    // we may change the bitmap if we stretch it
    wxBitmap bmp = bitmap;
    if ( !bmp.Ok() )
        return;

    int width = bmp.GetWidth(),
        height = bmp.GetHeight();

    wxCoord x = 0,
            y = 0;
    if ( stretch & wxTILE )
    {
        // tile the bitmap
        for ( ; x < rect.width; x += width )
        {
            for ( y = 0; y < rect.height; y += height )
            {
                // no need to use mask here as we cover the entire window area
                dc.DrawBitmap(bmp, x, y);
            }
        }
    }
#if wxUSE_IMAGE
    else if ( stretch & wxEXPAND )
    {
        // stretch bitmap to fill the entire control
        bmp = wxBitmap(wxImage(bmp.ConvertToImage()).Scale(rect.width, rect.height));
    }
#endif // wxUSE_IMAGE
    else // not stretched, not tiled
    {
        if ( alignment & wxALIGN_RIGHT )
        {
            x = rect.GetRight() - width;
        }
        else if ( alignment & wxALIGN_CENTRE )
        {
            x = (rect.GetLeft() + rect.GetRight() - width + 1) / 2;
        }
        else // alignment & wxALIGN_LEFT
        {
            x = rect.GetLeft();
        }

        if ( alignment & wxALIGN_BOTTOM )
        {
            y = rect.GetBottom() - height;
        }
        else if ( alignment & wxALIGN_CENTRE_VERTICAL )
        {
            y = (rect.GetTop() + rect.GetBottom() - height + 1) / 2;
        }
        else // alignment & wxALIGN_TOP
        {
            y = rect.GetTop();
        }
    }

    // do draw it
    dc.DrawBitmap(bmp, x, y, true /* use mask */);
}

#if wxUSE_SCROLLBAR

void wxControlRenderer::DrawScrollbar(const wxScrollBar *scrollbar,
                                      int WXUNUSED(thumbPosOld))
{
    // we will only redraw the parts which must be redrawn and not everything
    wxRegion rgnUpdate = scrollbar->GetUpdateRegion();

    {
        wxRect rectUpdate = rgnUpdate.GetBox();
        wxLogTrace(_T("scrollbar"),
                   _T("%s redraw: update box is (%d, %d)-(%d, %d)"),
                   scrollbar->IsVertical() ? _T("vert") : _T("horz"),
                   rectUpdate.GetLeft(),
                   rectUpdate.GetTop(),
                   rectUpdate.GetRight(),
                   rectUpdate.GetBottom());

#if 0 //def WXDEBUG_SCROLLBAR
        static bool s_refreshDebug = false;
        if ( s_refreshDebug )
        {
            wxClientDC dc(wxConstCast(scrollbar, wxScrollBar));
            dc.SetBrush(*wxRED_BRUSH);
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(rectUpdate);

            // under Unix we use "--sync" X option for this
            #ifdef __WXMSW__
                ::GdiFlush();
                ::Sleep(200);
            #endif // __WXMSW__
        }
#endif // WXDEBUG_SCROLLBAR
    }

    wxOrientation orient = scrollbar->IsVertical() ? wxVERTICAL
                                                   : wxHORIZONTAL;

    // the shaft
    for ( int nBar = 0; nBar < 2; nBar++ )
    {
        wxScrollBar::Element elem =
            (wxScrollBar::Element)(wxScrollBar::Element_Bar_1 + nBar);

        wxRect rectBar = scrollbar->GetScrollbarRect(elem);

        if ( rgnUpdate.Contains(rectBar) )
        {
            wxLogTrace(_T("scrollbar"),
                       _T("drawing bar part %d at (%d, %d)-(%d, %d)"),
                       nBar + 1,
                       rectBar.GetLeft(),
                       rectBar.GetTop(),
                       rectBar.GetRight(),
                       rectBar.GetBottom());

            m_renderer->DrawScrollbarShaft(m_dc,
                                           orient,
                                           rectBar,
                                           scrollbar->GetState(elem));
        }
    }

    // arrows
    for ( int nArrow = 0; nArrow < 2; nArrow++ )
    {
        wxScrollBar::Element elem =
            (wxScrollBar::Element)(wxScrollBar::Element_Arrow_Line_1 + nArrow);

        wxRect rectArrow = scrollbar->GetScrollbarRect(elem);
        if ( rgnUpdate.Contains(rectArrow) )
        {
            wxLogTrace(_T("scrollbar"),
                       _T("drawing arrow %d at (%d, %d)-(%d, %d)"),
                       nArrow + 1,
                       rectArrow.GetLeft(),
                       rectArrow.GetTop(),
                       rectArrow.GetRight(),
                       rectArrow.GetBottom());

            scrollbar->GetArrows().DrawArrow
            (
                (wxScrollArrows::Arrow)nArrow,
                m_dc,
                rectArrow,
                true // draw a scrollbar arrow, not just an arrow
            );
        }
    }

    // TODO: support for page arrows

    // and the thumb
    wxScrollBar::Element elem = wxScrollBar::Element_Thumb;
    wxRect rectThumb = scrollbar->GetScrollbarRect(elem);
    if ( rectThumb.width && rectThumb.height && rgnUpdate.Contains(rectThumb) )
    {
        wxLogTrace(_T("scrollbar"),
                   _T("drawing thumb at (%d, %d)-(%d, %d)"),
                   rectThumb.GetLeft(),
                   rectThumb.GetTop(),
                   rectThumb.GetRight(),
                   rectThumb.GetBottom());

        m_renderer->DrawScrollbarThumb(m_dc,
                                       orient,
                                       rectThumb,
                                       scrollbar->GetState(elem));
    }
}

#endif // wxUSE_SCROLLBAR

void wxControlRenderer::DrawLine(wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2)
{
    wxASSERT_MSG( x1 == x2 || y1 == y2,
                  _T("line must be either horizontal or vertical") );

    if ( x1 == x2 )
        m_renderer->DrawVerticalLine(m_dc, x1, y1, y2);
    else // horizontal
        m_renderer->DrawHorizontalLine(m_dc, y1, x1, x2);
}

#if wxUSE_LISTBOX

void wxControlRenderer::DrawItems(const wxListBox *lbox,
                                  size_t itemFirst, size_t itemLast)
{
    DoDrawItems(lbox, itemFirst, itemLast);
}

void wxControlRenderer::DoDrawItems(const wxListBox *lbox,
                                    size_t itemFirst, size_t itemLast,
#if wxUSE_CHECKLISTBOX
                                    bool isCheckLbox
#else
                                    bool WXUNUSED(isCheckLbox)
#endif
                                    )
{
    // prepare for the drawing: calc the initial position
    wxCoord lineHeight = lbox->GetLineHeight();

    // note that SetClippingRegion() needs the physical (unscrolled)
    // coordinates while we use the logical (scrolled) ones for the drawing
    // itself
    wxRect rect;
    wxSize size = lbox->GetClientSize();
    rect.width = size.x;
    rect.height = size.y;

    // keep the text inside the client rect or we will overwrite the vertical
    // scrollbar for the long strings
    m_dc.SetClippingRegion(rect.x, rect.y, rect.width + 1, rect.height + 1);

    // adjust the rect position now
    lbox->CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
    rect.y += itemFirst*lineHeight;
    rect.height = lineHeight;

    // the rect should go to the right visible border so adjust the width if x
    // is shifted (rightmost point should stay the same)
    rect.width -= rect.x;

    // we'll keep the text colour unchanged
    m_dc.SetTextForeground(lbox->GetForegroundColour());

    // an item should have the focused rect only when the lbox has focus, so
    // make sure that we never set wxCONTROL_FOCUSED flag if it doesn't
    int itemCurrent = wxWindow::FindFocus() == (wxWindow *)lbox // cast needed
                        ? lbox->GetCurrentItem()
                        : -1;
    for ( size_t n = itemFirst; n < itemLast; n++ )
    {
        int flags = 0;
        if ( (int)n == itemCurrent )
            flags |= wxCONTROL_FOCUSED;
        if ( lbox->IsSelected(n) )
            flags |= wxCONTROL_SELECTED;

#if wxUSE_CHECKLISTBOX
        if ( isCheckLbox )
        {
            wxCheckListBox *checklstbox = wxStaticCast(lbox, wxCheckListBox);
            if ( checklstbox->IsChecked(n) )
                flags |= wxCONTROL_CHECKED;

            m_renderer->DrawCheckItem(m_dc, lbox->GetString(n),
                                      wxNullBitmap,
                                      rect,
                                      flags);
        }
        else
#endif // wxUSE_CHECKLISTBOX
        {
            m_renderer->DrawItem(m_dc, lbox->GetString(n), rect, flags);
        }

        rect.y += lineHeight;
    }
}

#endif // wxUSE_LISTBOX

#if wxUSE_CHECKLISTBOX

void wxControlRenderer::DrawCheckItems(const wxCheckListBox *lbox,
                                       size_t itemFirst, size_t itemLast)
{
    DoDrawItems(lbox, itemFirst, itemLast, true);
}

#endif // wxUSE_CHECKLISTBOX

#if wxUSE_GAUGE

void wxControlRenderer::DrawProgressBar(const wxGauge *gauge)
{
    // draw background
    m_dc.SetBrush(wxBrush(m_window->GetBackgroundColour(), wxSOLID));
    m_dc.SetPen(*wxTRANSPARENT_PEN);
    m_dc.DrawRectangle(m_rect);

    int max = gauge->GetRange();
    if ( !max )
    {
        // nothing to draw
        return;
    }

    // calc the filled rect
    int pos = gauge->GetValue();
    int left = max - pos;

    wxRect rect = m_rect;
    rect.Deflate(1); // FIXME this depends on the border width

    wxColour col = m_window->UseFgCol() ? m_window->GetForegroundColour()
                                        : wxTHEME_COLOUR(GAUGE);
    m_dc.SetBrush(wxBrush(col, wxSOLID));

    if ( gauge->IsSmooth() )
    {
        // just draw the rectangle in one go
        if ( gauge->IsVertical() )
        {
            // vert bars grow from bottom to top
            wxCoord dy = ((rect.height - 1) * left) / max;
            rect.y += dy;
            rect.height -= dy;
        }
        else // horizontal
        {
            // grow from left to right
            rect.width -= ((rect.width - 1) * left) / max;
        }

        m_dc.DrawRectangle(rect);
    }
    else // discrete
    {
        wxSize sizeStep = m_renderer->GetProgressBarStep();
        int step = gauge->IsVertical() ? sizeStep.y : sizeStep.x;

        // we divide by it below!
        wxCHECK_RET( step, _T("invalid wxGauge step") );

        // round up to make the progress appear to start faster
        int lenTotal = gauge->IsVertical() ? rect.height : rect.width;
        int steps = ((lenTotal + step - 1) * pos) / (max * step);

        // calc the coords of one small rect
        wxCoord *px;
        wxCoord dx, dy;
        if ( gauge->IsVertical() )
        {
            // draw from bottom to top: so first set y to the bottom
            rect.y += rect.height - 1;

            // then adjust the height
            rect.height = step;

            // and then adjust y again to be what it should for the first rect
            rect.y -= rect.height;

            // we are going up
            step = -step;

            // remember that this will be the coord which will change
            px = &rect.y;

            dy = 1;
            dx = 0;
        }
        else // horizontal
        {
            // don't leave 2 empty pixels in the beginning
            rect.x--;

            px = &rect.x;
            rect.width = step;

            dy = 0;
            dx = 1;
        }

        for ( int n = 0; n < steps; n++ )
        {
            wxRect rectSegment = rect;
            rectSegment.Deflate(dx, dy);

            m_dc.DrawRectangle(rectSegment);

            *px += step;
            if ( *px < 1 )
            {
                // this can only happen for the last step of vertical gauge
                rect.height = *px - step - 1;
                *px = 1;
            }
            else if ( *px > lenTotal - step )
            {
                // this can only happen for the last step of horizontal gauge
                rect.width = lenTotal - *px - 1;
            }
        }
    }
}

#endif // wxUSE_GAUGE
