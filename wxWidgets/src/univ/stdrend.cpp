///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/stdrend.cpp
// Purpose:     implementation of wxStdRenderer
// Author:      Vadim Zeitlin
// Created:     2006-09-16
// RCS-ID:      $Id: stdrend.cpp 49996 2007-11-16 15:52:17Z CE $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
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
    #include "wx/settings.h"
    #include "wx/brush.h"
    #include "wx/dc.h"
    #include "wx/statusbr.h"
    #include "wx/toplevel.h"
#endif //WX_PRECOMP

#include "wx/univ/stdrend.h"
#include "wx/univ/colschem.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const int FRAME_TITLEBAR_HEIGHT             = 18;
static const int FRAME_BUTTON_WIDTH                = 16;
static const int FRAME_BUTTON_HEIGHT               = 14;

// the margin between listbox item text and its rectangle
static const int ITEM_MARGIN = 1;

// ============================================================================
// wxStdRenderer implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

wxStdRenderer::wxStdRenderer(const wxColourScheme *scheme)
             : m_scheme(scheme)
{
    m_penBlack = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_DARK));
    m_penDarkGrey = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_OUT));
    m_penLightGrey = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_IN));
    m_penHighlight = wxPen(wxSCHEME_COLOUR(scheme, SHADOW_HIGHLIGHT));

    m_titlebarFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_titlebarFont.SetWeight(wxFONTWEIGHT_BOLD);
}

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

void
wxStdRenderer::DrawSolidRect(wxDC& dc, const wxColour& col, const wxRect& rect)
{
    wxBrush brush(col, wxSOLID);
    dc.SetBrush(brush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
}

void wxStdRenderer::DrawRect(wxDC& dc, wxRect *rect, const wxPen& pen)
{
    // draw
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(*rect);

    // adjust the rect
    rect->Inflate(-1);
}

void wxStdRenderer::DrawShadedRect(wxDC& dc, wxRect *rect,
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

// ----------------------------------------------------------------------------
// translate various flags into corresponding renderer constants
// ----------------------------------------------------------------------------

/* static */
void wxStdRenderer::GetIndicatorsFromFlags(int flags,
                                           IndicatorState& state,
                                           IndicatorStatus& status)
{
    if ( flags & wxCONTROL_SELECTED )
        state = flags & wxCONTROL_DISABLED ? IndicatorState_SelectedDisabled
                                           : IndicatorState_Selected;
    else if ( flags & wxCONTROL_DISABLED )
        state = IndicatorState_Disabled;
    else if ( flags & wxCONTROL_PRESSED )
        state = IndicatorState_Pressed;
    else
        state = IndicatorState_Normal;

    status = flags & wxCONTROL_CHECKED ? IndicatorStatus_Checked
                                       : flags & wxCONTROL_UNDETERMINED
                                            ? IndicatorStatus_Undetermined
                                            : IndicatorStatus_Unchecked;
}

/* static */
wxStdRenderer::ArrowDirection wxStdRenderer::GetArrowDirection(wxDirection dir)
{
    switch ( dir )
    {
        case wxLEFT:
            return Arrow_Left;

        case wxRIGHT:
            return Arrow_Right;

        case wxUP:
            return Arrow_Up;

        case wxDOWN:
            return Arrow_Down;

        default:
            wxFAIL_MSG(_T("unknown arrow direction"));
    }

    return Arrow_Max;
}

// ----------------------------------------------------------------------------
// background
// ----------------------------------------------------------------------------

void wxStdRenderer::DrawBackground(wxDC& dc,
                                   const wxColour& col,
                                   const wxRect& rect,
                                   int WXUNUSED(flags),
                                   wxWindow *window)
{
    wxColour colBg;

    if (col.Ok())
    {
        colBg = col;
    }
    else if (window)
    {
        colBg = m_scheme->GetBackground(window);
    }
    else
    {
        colBg = wxSCHEME_COLOUR(m_scheme, CONTROL);
    }

    DrawSolidRect(dc, colBg, rect);
}


void wxStdRenderer::DrawButtonSurface(wxDC& dc,
                                      const wxColour& col,
                                      const wxRect& rect,
                                      int flags)
{
    DrawBackground(dc, col, rect, flags);
}

// ----------------------------------------------------------------------------
// text
// ----------------------------------------------------------------------------

void
wxStdRenderer::DrawFocusRect(wxDC& dc, const wxRect& rect, int WXUNUSED(flags))
{
    // draw the pixels manually because the "dots" in wxPen with wxDOT style
    // may be short traits and not really dots
    //
    // note that to behave in the same manner as DrawRect(), we must exclude
    // the bottom and right borders from the rectangle
    wxCoord x1 = rect.GetLeft(),
            y1 = rect.GetTop(),
            x2 = rect.GetRight(),
            y2 = rect.GetBottom();

    dc.SetPen(m_penBlack);

    // this seems to be closer than what Windows does than wxINVERT although
    // I'm still not sure if it's correct
    dc.SetLogicalFunction(wxAND_REVERSE);

    wxCoord z;
    for ( z = x1 + 1; z < x2; z += 2 )
        dc.DrawPoint(z, rect.GetTop());

    wxCoord shift = z == x2 ? 0 : 1;
    for ( z = y1 + shift; z < y2; z += 2 )
        dc.DrawPoint(x2, z);

    shift = z == y2 ? 0 : 1;
    for ( z = x2 - shift; z > x1; z -= 2 )
        dc.DrawPoint(z, y2);

    shift = z == x1 ? 0 : 1;
    for ( z = y2 - shift; z > y1; z -= 2 )
        dc.DrawPoint(x1, z);

    dc.SetLogicalFunction(wxCOPY);
}

void wxStdRenderer::DrawLabel(wxDC& dc,
                              const wxString& label,
                              const wxRect& rect,
                              int flags,
                              int alignment,
                              int indexAccel,
                              wxRect *rectBounds)
{
    DrawButtonLabel(dc, label, wxNullBitmap, rect, flags,
                    alignment, indexAccel, rectBounds);
}

void wxStdRenderer::DrawButtonLabel(wxDC& dc,
                                    const wxString& label,
                                    const wxBitmap& image,
                                    const wxRect& rect,
                                    int flags,
                                    int alignment,
                                    int indexAccel,
                                    wxRect *rectBounds)
{
    wxDCTextColourChanger clrChanger(dc);

    wxRect rectLabel = rect;
    if ( !label.empty() && (flags & wxCONTROL_DISABLED) )
    {
        if ( flags & wxCONTROL_PRESSED )
        {
            // shift the label if a button is pressed
            rectLabel.Offset(1, 1);
        }

        // draw shadow of the text
        clrChanger.Set(m_penHighlight.GetColour());
        wxRect rectShadow = rect;
        rectShadow.Offset(1, 1);
        dc.DrawLabel(label, rectShadow, alignment, indexAccel);

        // make the main label text grey
        clrChanger.Set(m_penDarkGrey.GetColour());

        if ( flags & wxCONTROL_FOCUSED )
        {
            // leave enough space for the focus rect
            rectLabel.Inflate(-2);
        }
    }

    dc.DrawLabel(label, image, rectLabel, alignment, indexAccel, rectBounds);

    if ( !label.empty() && (flags & wxCONTROL_FOCUSED) )
    {
        rectLabel.Inflate(-1);

        DrawFocusRect(dc, rectLabel);
    }
}

// ----------------------------------------------------------------------------
// borders
// ----------------------------------------------------------------------------

/*
   We implement standard-looking 3D borders which have the following appearance:

   The raised border:

   WWWWWWWWWWWWWWWWWWWWWWB
   WHHHHHHHHHHHHHHHHHHHHGB
   WH                   GB  W = white       (HILIGHT)
   WH                   GB  H = light grey  (LIGHT)
   WH                   GB  G = dark grey   (SHADOI)
   WH                   GB  B = black       (DKSHADOI)
   WH                   GB
   WH                   GB
   WGGGGGGGGGGGGGGGGGGGGGB
   BBBBBBBBBBBBBBBBBBBBBBB

   The sunken border looks like this:

   GGGGGGGGGGGGGGGGGGGGGGW
   GBBBBBBBBBBBBBBBBBBBBHW
   GB                   HW
   GB                   HW
   GB                   HW
   GB                   HW
   GB                   HW
   GB                   HW
   GHHHHHHHHHHHHHHHHHHHHHW
   WWWWWWWWWWWWWWWWWWWWWWW

   The static border (used for the controls which don't get focus) is like
   this:

   GGGGGGGGGGGGGGGGGGGGGGW
   G                     W
   G                     W
   G                     W
   G                     W
   G                     W
   G                     W
   G                     W
   WWWWWWWWWWWWWWWWWWWWWWW

   The most complicated is the double border which is a combination of special
   "anti-sunken" border and an extra border inside it:

   HHHHHHHHHHHHHHHHHHHHHHB
   HWWWWWWWWWWWWWWWWWWWWGB
   HWHHHHHHHHHHHHHHHHHHHGB
   HWH                 HGB
   HWH                 HGB
   HWH                 HGB
   HWH                 HGB
   HWHHHHHHHHHHHHHHHHHHHGB
   HGGGGGGGGGGGGGGGGGGGGGB
   BBBBBBBBBBBBBBBBBBBBBBB

   And the simple border is, well, simple:

   BBBBBBBBBBBBBBBBBBBBBBB
   B                     B
   B                     B
   B                     B
   B                     B
   B                     B
   B                     B
   B                     B
   B                     B
   BBBBBBBBBBBBBBBBBBBBBBB
*/

void wxStdRenderer::DrawRaisedBorder(wxDC& dc, wxRect *rect)
{
    DrawShadedRect(dc, rect, m_penHighlight, m_penBlack);
    DrawShadedRect(dc, rect, m_penLightGrey, m_penDarkGrey);
}

void wxStdRenderer::DrawSunkenBorder(wxDC& dc, wxRect *rect)
{
    DrawShadedRect(dc, rect, m_penDarkGrey, m_penHighlight);
    DrawShadedRect(dc, rect, m_penBlack, m_penLightGrey);
}

void wxStdRenderer::DrawAntiSunkenBorder(wxDC& dc, wxRect *rect)
{
    DrawShadedRect(dc, rect, m_penLightGrey, m_penBlack);
    DrawShadedRect(dc, rect, m_penHighlight, m_penDarkGrey);
}

void wxStdRenderer::DrawBoxBorder(wxDC& dc, wxRect *rect)
{
    DrawShadedRect(dc, rect, m_penDarkGrey, m_penHighlight);
    DrawShadedRect(dc, rect, m_penHighlight, m_penDarkGrey);
}

void wxStdRenderer::DrawStaticBorder(wxDC& dc, wxRect *rect)
{
    DrawShadedRect(dc, rect, m_penDarkGrey, m_penHighlight);
}

void wxStdRenderer::DrawExtraBorder(wxDC& dc, wxRect *rect)
{
    DrawRect(dc, rect, m_penLightGrey);
}

void wxStdRenderer::DrawBorder(wxDC& dc,
                               wxBorder border,
                               const wxRect& rectTotal,
                               int WXUNUSED(flags),
                               wxRect *rectIn)
{
    wxRect rect = rectTotal;

    switch ( border )
    {
        case wxBORDER_THEME:
        case wxBORDER_SUNKEN:
            DrawSunkenBorder(dc, &rect);
            break;

        // wxBORDER_DOUBLE is no longer supported since wxBORDER_THEME takes on the same value
#if 0
        case wxBORDER_DOUBLE:
            DrawAntiSunkenBorder(dc, &rect);
            DrawExtraBorder(dc, &rect);
            break;
#endif
        case wxBORDER_STATIC:
            DrawStaticBorder(dc, &rect);
            break;

        case wxBORDER_RAISED:
            DrawRaisedBorder(dc, &rect);
            break;

        case wxBORDER_SIMPLE:
            DrawRect(dc, &rect, m_penBlack);
            break;

        default:
            wxFAIL_MSG(_T("unknown border type"));
            // fall through

        case wxBORDER_DEFAULT:
        case wxBORDER_NONE:
            break;
    }

    if ( rectIn )
        *rectIn = rect;
}

wxRect wxStdRenderer::GetBorderDimensions(wxBorder border) const
{
    wxCoord width;
    switch ( border )
    {
        case wxBORDER_SIMPLE:
        case wxBORDER_STATIC:
            width = 1;
            break;

        case wxBORDER_RAISED:
        case wxBORDER_SUNKEN:
        case wxBORDER_THEME:
            width = 2;
            break;
#if 0
        case wxBORDER_DOUBLE:
            width = 3;
            break;
#endif
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

void wxStdRenderer::AdjustSize(wxSize *size, const wxWindow *window)
{
    // take into account the border width
    wxRect rectBorder = GetBorderDimensions(window->GetBorder());
    size->x += rectBorder.x + rectBorder.width;
    size->y += rectBorder.y + rectBorder.height;
}

bool wxStdRenderer::AreScrollbarsInsideBorder() const
{
    return false;
}

wxCoord wxStdRenderer::GetListboxItemHeight(wxCoord fontHeight)
{
    return fontHeight + 2*ITEM_MARGIN;
}

void wxStdRenderer::DrawTextBorder(wxDC& dc,
                                   wxBorder border,
                                   const wxRect& rect,
                                   int flags,
                                   wxRect *rectIn)
{
    DrawBorder(dc, border, rect, flags, rectIn);
}

// ----------------------------------------------------------------------------
// lines and boxes
// ----------------------------------------------------------------------------

void
wxStdRenderer::DrawHorizontalLine(wxDC& dc, wxCoord y, wxCoord x1, wxCoord x2)
{
    dc.SetPen(m_penDarkGrey);
    dc.DrawLine(x1, y, x2 + 1, y);

    dc.SetPen(m_penHighlight);
    y++;
    dc.DrawLine(x1, y, x2 + 1, y);
}

void
wxStdRenderer::DrawVerticalLine(wxDC& dc, wxCoord x, wxCoord y1, wxCoord y2)
{
    dc.SetPen(m_penDarkGrey);
    dc.DrawLine(x, y1, x, y2 + 1);

    dc.SetPen(m_penHighlight);
    x++;
    dc.DrawLine(x, y1, x, y2 + 1);
}

void wxStdRenderer::DrawFrameWithoutLabel(wxDC& dc,
                                          const wxRect& rectFrame,
                                          const wxRect& rectLabel)
{
    // draw left, bottom and right lines entirely
    DrawVerticalLine(dc, rectFrame.GetLeft(),
                     rectFrame.GetTop(), rectFrame.GetBottom() - 2);
    DrawHorizontalLine(dc, rectFrame.GetBottom() - 1,
                       rectFrame.GetLeft(), rectFrame.GetRight());
    DrawVerticalLine(dc, rectFrame.GetRight() - 1,
                     rectFrame.GetTop(), rectFrame.GetBottom() - 1);

    // and 2 parts of the top line
    DrawHorizontalLine(dc, rectFrame.GetTop(),
                       rectFrame.GetLeft() + 1, rectLabel.GetLeft());
    DrawHorizontalLine(dc, rectFrame.GetTop(),
                       rectLabel.GetRight(), rectFrame.GetRight() - 2);
}

void wxStdRenderer::DrawFrameWithLabel(wxDC& dc,
                                       const wxString& label,
                                       const wxRect& rectFrame,
                                       const wxRect& rectText,
                                       int flags,
                                       int alignment,
                                       int indexAccel)
{
    wxRect rectLabel;
    DrawLabel(dc, label, rectText, flags, alignment, indexAccel, &rectLabel);

    DrawFrameWithoutLabel(dc, rectFrame, rectLabel);
}

void wxStdRenderer::DrawFrame(wxDC& dc,
                              const wxString& label,
                              const wxRect& rect,
                              int flags,
                              int alignment,
                              int indexAccel)
{
    wxCoord height = 0; // of the label
    wxRect rectFrame = rect;
    if ( !label.empty() )
    {
        // the text should touch the top border of the rect, so the frame
        // itself should be lower
        dc.GetTextExtent(label, NULL, &height);
        rectFrame.y += height / 2;
        rectFrame.height -= height / 2;

        // we have to draw each part of the frame individually as we can't
        // erase the background beyond the label as it might contain some
        // pixmap already, so drawing everything and then overwriting part of
        // the frame with label doesn't work

        // TODO: the +5 shouldn't be hard coded
        wxRect rectText;
        rectText.x = rectFrame.x + 5;
        rectText.y = rect.y;
        rectText.width = rectFrame.width - 7; // +2 border width
        rectText.height = height;

        DrawFrameWithLabel(dc, label, rectFrame, rectText, flags,
                           alignment, indexAccel);
    }
    else // no label
    {
        DrawBoxBorder(dc, &rectFrame);
    }
}

void wxStdRenderer::DrawItem(wxDC& dc,
                             const wxString& label,
                             const wxRect& rect,
                             int flags)
{
    wxDCTextColourChanger colChanger(dc);

    if ( flags & wxCONTROL_SELECTED )
    {
        colChanger.Set(wxSCHEME_COLOUR(m_scheme, HIGHLIGHT_TEXT));

        const wxColour colBg = wxSCHEME_COLOUR(m_scheme, HIGHLIGHT);
        dc.SetBrush(colBg);
        dc.SetPen(colBg);
        dc.DrawRectangle(rect);
    }

    // horizontal adjustment is arbitrary
    wxRect rectText = rect;
    rectText.Deflate(2, ITEM_MARGIN);
    dc.DrawLabel(label, wxNullBitmap, rectText);

    if ( flags & wxCONTROL_FOCUSED )
    {
        DrawFocusRect(dc, rect, flags);
    }
}

void wxStdRenderer::DrawCheckItemBitmap(wxDC& dc,
                                        const wxBitmap& bitmap,
                                        const wxRect& rect,
                                        int flags)
{
    DrawCheckButton(dc, wxEmptyString, bitmap, rect, flags);
}

void wxStdRenderer::DrawCheckItem(wxDC& dc,
                                  const wxString& label,
                                  const wxBitmap& bitmap,
                                  const wxRect& rect,
                                  int flags)
{
    wxRect rectBitmap = rect;
    rectBitmap.width = GetCheckBitmapSize().x;
    DrawCheckItemBitmap(dc, bitmap, rectBitmap, flags);

    wxRect rectLabel = rect;
    wxCoord shift = rectBitmap.width + 2*GetCheckItemMargin();
    rectLabel.x += shift;
    rectLabel.width -= shift;
    DrawItem(dc, label, rectLabel, flags);
}

// ----------------------------------------------------------------------------
// check and radio bitmaps
// ----------------------------------------------------------------------------

void wxStdRenderer::DrawCheckButton(wxDC& dc,
                                    const wxString& label,
                                    const wxBitmap& bitmap,
                                    const wxRect& rect,
                                    int flags,
                                    wxAlignment align,
                                    int indexAccel)
{
    if (bitmap.Ok())
         DrawCheckOrRadioButton(dc, label, bitmap, rect, flags, align, indexAccel);
    else
         DrawCheckOrRadioButton(dc, label, GetCheckBitmap(flags), rect, flags, align, indexAccel);
}

void wxStdRenderer::DrawRadioButton(wxDC& dc,
                                    const wxString& label,
                                    const wxBitmap& bitmap,
                                    const wxRect& rect,
                                    int flags,
                                    wxAlignment align,
                                    int indexAccel)
{
    if (bitmap.Ok()) 
       DrawCheckOrRadioButton(dc, label, bitmap, rect, flags, align, indexAccel);
    else   
       DrawCheckOrRadioButton(dc, label, GetRadioBitmap(flags), rect, flags, align, indexAccel);
}

void wxStdRenderer::DrawCheckOrRadioButton(wxDC& dc,
                                           const wxString& label,
                                           const wxBitmap& bitmap,
                                           const wxRect& rect,
                                           int flags,
                                           wxAlignment align,
                                           int indexAccel)
{
    // calculate the position of the bitmap and of the label
    wxCoord heightBmp = bitmap.GetHeight();
    wxCoord xBmp,
            yBmp = rect.y + (rect.height - heightBmp) / 2;

    wxRect rectLabel;
    dc.GetMultiLineTextExtent(label, NULL, &rectLabel.height);
    rectLabel.y = rect.y + (rect.height - rectLabel.height) / 2;

    // align label vertically with the bitmap - looks nicer like this
    rectLabel.y -= (rectLabel.height - heightBmp) % 2;

    // calc horz position
    if ( align == wxALIGN_RIGHT )
    {
        xBmp = rect.GetRight() - bitmap.GetWidth();
        rectLabel.x = rect.x + 3;
        rectLabel.SetRight(xBmp);
    }
    else // normal (checkbox to the left of the text) case
    {
        xBmp = rect.x;
        rectLabel.x = xBmp + bitmap.GetWidth() + 5;
        rectLabel.SetRight(rect.GetRight());
    }

    dc.DrawBitmap(bitmap, xBmp, yBmp, true /* use mask */);

    DrawLabel(dc, label, rectLabel, flags,
              wxALIGN_LEFT | wxALIGN_TOP, indexAccel);
}

#if wxUSE_TEXTCTRL

void wxStdRenderer::DrawTextLine(wxDC& dc,
                                 const wxString& text,
                                 const wxRect& rect,
                                 int selStart,
                                 int selEnd,
                                 int flags)
{
    if ( (selStart == -1) || !(flags & wxCONTROL_FOCUSED) )
    {
        // just draw it as is
        dc.DrawText(text, rect.x, rect.y);
    }
    else // we have selection
    {
        wxCoord width,
                x = rect.x;

        // draw the part before selection
        wxString s(text, (size_t)selStart);
        if ( !s.empty() )
        {
            dc.DrawText(s, x, rect.y);

            dc.GetTextExtent(s, &width, NULL);
            x += width;
        }

        // draw the selection itself
        s = wxString(text.c_str() + selStart, text.c_str() + selEnd);
        if ( !s.empty() )
        {
            wxColour colFg = dc.GetTextForeground(),
                     colBg = dc.GetTextBackground();
            dc.SetTextForeground(wxSCHEME_COLOUR(m_scheme, HIGHLIGHT_TEXT));
            dc.SetTextBackground(wxSCHEME_COLOUR(m_scheme, HIGHLIGHT));
            dc.SetBackgroundMode(wxSOLID);

            dc.DrawText(s, x, rect.y);
            dc.GetTextExtent(s, &width, NULL);
            x += width;

            dc.SetBackgroundMode(wxTRANSPARENT);
            dc.SetTextBackground(colBg);
            dc.SetTextForeground(colFg);
        }

        // draw the final part
        s = text.c_str() + selEnd;
        if ( !s.empty() )
        {
            dc.DrawText(s, x, rect.y);
        }
    }
}

void wxStdRenderer::DrawLineWrapMark(wxDC& WXUNUSED(dc),
                                     const wxRect& WXUNUSED(rect))
{
    // nothing by default
}

int wxStdRenderer::GetTextBorderWidth(const wxTextCtrl * WXUNUSED(text)) const
{
    return 1;
}

wxRect
wxStdRenderer::GetTextTotalArea(const wxTextCtrl *text, const wxRect& rect) const
{
    wxRect rectTotal = rect;
    rectTotal.Inflate(GetTextBorderWidth(text));
    return rectTotal;
}

wxRect wxStdRenderer::GetTextClientArea(const wxTextCtrl *text,
                                        const wxRect& rect,
                                        wxCoord *extraSpaceBeyond) const
{
    wxRect rectText = rect;
    rectText.Deflate(GetTextBorderWidth(text));

    if ( extraSpaceBeyond )
        *extraSpaceBeyond = 0;

    return rectText;
}

#endif // wxUSE_TEXTCTRL

// ----------------------------------------------------------------------------
// scrollbars drawing
// ----------------------------------------------------------------------------

void wxStdRenderer::DrawScrollbarArrow(wxDC& dc,
                                       wxDirection dir,
                                       const wxRect& rect,
                                       int flags)
{
    DrawArrow(dc, dir, rect, flags);
}

void wxStdRenderer::DrawScrollCorner(wxDC& dc, const wxRect& rect)
{
    DrawSolidRect(dc, wxSCHEME_COLOUR(m_scheme, CONTROL), rect);
}

// ----------------------------------------------------------------------------
// status bar
// ----------------------------------------------------------------------------

#if wxUSE_STATUSBAR

wxSize wxStdRenderer::GetStatusBarBorders() const
{
    // Rendered border may be different depending on field's style, we use
    // the largest value so that any field certainly fits into the borders
    // we return:
    wxRect raised = GetBorderDimensions(wxBORDER_RAISED);
    wxRect flat = GetBorderDimensions(wxBORDER_STATIC);
    wxASSERT_MSG( raised.x == raised.width && raised.y == raised.height &&
                  flat.x == flat.width && flat.y == flat.height,
                  _T("this code expects uniform borders, you must override GetStatusBarBorders") );

    // take the larger of flat/raised values:
    wxSize border(wxMax(raised.x, flat.x), wxMax(raised.y, flat.y));

    return border;
}

wxCoord wxStdRenderer::GetStatusBarBorderBetweenFields() const
{
    return 2;
}

wxSize wxStdRenderer::GetStatusBarFieldMargins() const
{
    return wxSize(2, 2);
}

void wxStdRenderer::DrawStatusField(wxDC& dc,
                                    const wxRect& rect,
                                    const wxString& label,
                                    int flags,
                                    int style)
{
    wxRect rectIn;

    if ( style == wxSB_RAISED )
        DrawBorder(dc, wxBORDER_RAISED, rect, flags, &rectIn);
    else if ( style != wxSB_FLAT )
        DrawBorder(dc, wxBORDER_STATIC, rect, flags, &rectIn);

    rectIn.Deflate(GetStatusBarFieldMargins());

    wxDCClipper clipper(dc, rectIn);
    DrawLabel(dc, label, rectIn, flags, wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
}

#endif // wxUSE_STATUSBAR

// ----------------------------------------------------------------------------
// top level windows
// ----------------------------------------------------------------------------

int wxStdRenderer::HitTestFrame(const wxRect& rect, const wxPoint& pt, int flags) const
{
    wxRect client = GetFrameClientArea(rect, flags);

    if ( client.Contains(pt) )
        return wxHT_TOPLEVEL_CLIENT_AREA;

    if ( flags & wxTOPLEVEL_TITLEBAR )
    {
        wxRect client = GetFrameClientArea(rect, flags & ~wxTOPLEVEL_TITLEBAR);

        if ( flags & wxTOPLEVEL_ICON )
        {
            if ( wxRect(client.GetPosition(), GetFrameIconSize()).Contains(pt) )
                return wxHT_TOPLEVEL_ICON;
        }

        wxRect btnRect(client.GetRight() - 2 - FRAME_BUTTON_WIDTH,
                       client.GetTop() + (FRAME_TITLEBAR_HEIGHT-FRAME_BUTTON_HEIGHT)/2,
                       FRAME_BUTTON_WIDTH, FRAME_BUTTON_HEIGHT);

        if ( flags & wxTOPLEVEL_BUTTON_CLOSE )
        {
            if ( btnRect.Contains(pt) )
                return wxHT_TOPLEVEL_BUTTON_CLOSE;
            btnRect.x -= FRAME_BUTTON_WIDTH + 2;
        }
        if ( flags & wxTOPLEVEL_BUTTON_MAXIMIZE )
        {
            if ( btnRect.Contains(pt) )
                return wxHT_TOPLEVEL_BUTTON_MAXIMIZE;
            btnRect.x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_RESTORE )
        {
            if ( btnRect.Contains(pt) )
                return wxHT_TOPLEVEL_BUTTON_RESTORE;
            btnRect.x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_ICONIZE )
        {
            if ( btnRect.Contains(pt) )
                return wxHT_TOPLEVEL_BUTTON_ICONIZE;
            btnRect.x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_HELP )
        {
            if ( btnRect.Contains(pt) )
                return wxHT_TOPLEVEL_BUTTON_HELP;
            btnRect.x -= FRAME_BUTTON_WIDTH;
        }

        if ( pt.y >= client.y && pt.y < client.y + FRAME_TITLEBAR_HEIGHT )
            return wxHT_TOPLEVEL_TITLEBAR;
    }

    if ( (flags & wxTOPLEVEL_BORDER) && !(flags & wxTOPLEVEL_MAXIMIZED) )
    {
        // we are certainly at one of borders, let's decide which one:

        int border = 0;
        // dirty trick, relies on the way wxHT_TOPLEVEL_XXX are defined!
        if ( pt.x < client.x )
            border |= wxHT_TOPLEVEL_BORDER_W;
        else if ( pt.x >= client.width + client.x )
            border |= wxHT_TOPLEVEL_BORDER_E;
        if ( pt.y < client.y )
            border |= wxHT_TOPLEVEL_BORDER_N;
        else if ( pt.y >= client.height + client.y )
            border |= wxHT_TOPLEVEL_BORDER_S;
        return border;
    }

    return wxHT_NOWHERE;
}

void wxStdRenderer::DrawFrameTitleBar(wxDC& dc,
                                      const wxRect& rect,
                                      const wxString& title,
                                      const wxIcon& icon,
                                      int flags,
                                      int specialButton,
                                      int specialButtonFlags)
{
    if ( (flags & wxTOPLEVEL_BORDER) && !(flags & wxTOPLEVEL_MAXIMIZED) )
    {
        DrawFrameBorder(dc, rect, flags);
    }
    if ( flags & wxTOPLEVEL_TITLEBAR )
    {
        DrawFrameBackground(dc, rect, flags);
        if ( flags & wxTOPLEVEL_ICON )
            DrawFrameIcon(dc, rect, icon, flags);
        DrawFrameTitle(dc, rect, title, flags);

        wxRect client = GetFrameClientArea(rect, flags & ~wxTOPLEVEL_TITLEBAR);
        wxCoord x,y;
        x = client.GetRight() - 2 - FRAME_BUTTON_WIDTH;
        y = client.GetTop() + (FRAME_TITLEBAR_HEIGHT-FRAME_BUTTON_HEIGHT)/2;

        if ( flags & wxTOPLEVEL_BUTTON_CLOSE )
        {
            DrawFrameButton(dc, x, y, wxTOPLEVEL_BUTTON_CLOSE,
                            (specialButton == wxTOPLEVEL_BUTTON_CLOSE) ?
                            specialButtonFlags : 0);
            x -= FRAME_BUTTON_WIDTH + 2;
        }
        if ( flags & wxTOPLEVEL_BUTTON_MAXIMIZE )
        {
            DrawFrameButton(dc, x, y, wxTOPLEVEL_BUTTON_MAXIMIZE,
                            (specialButton == wxTOPLEVEL_BUTTON_MAXIMIZE) ?
                            specialButtonFlags : 0);
            x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_RESTORE )
        {
            DrawFrameButton(dc, x, y, wxTOPLEVEL_BUTTON_RESTORE,
                            (specialButton == wxTOPLEVEL_BUTTON_RESTORE) ?
                            specialButtonFlags : 0);
            x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_ICONIZE )
        {
            DrawFrameButton(dc, x, y, wxTOPLEVEL_BUTTON_ICONIZE,
                            (specialButton == wxTOPLEVEL_BUTTON_ICONIZE) ?
                            specialButtonFlags : 0);
            x -= FRAME_BUTTON_WIDTH;
        }
        if ( flags & wxTOPLEVEL_BUTTON_HELP )
        {
            DrawFrameButton(dc, x, y, wxTOPLEVEL_BUTTON_HELP,
                            (specialButton == wxTOPLEVEL_BUTTON_HELP) ?
                            specialButtonFlags : 0);
        }
    }
}

void wxStdRenderer::DrawFrameBorder(wxDC& dc, const wxRect& rect, int flags)
{
    if ( !(flags & wxTOPLEVEL_BORDER) )
        return;

    wxRect r(rect);

    DrawAntiSunkenBorder(dc, &r);
    DrawExtraBorder(dc, &r);
    if ( flags & wxTOPLEVEL_RESIZEABLE )
        DrawExtraBorder(dc, &r);
}

void wxStdRenderer::DrawFrameBackground(wxDC& dc, const wxRect& rect, int flags)
{
    if ( !(flags & wxTOPLEVEL_TITLEBAR) )
        return;

    wxColour col = m_scheme->Get(flags & wxTOPLEVEL_ACTIVE
                                    ? wxColourScheme::TITLEBAR_ACTIVE
                                    : wxColourScheme::TITLEBAR);

    wxRect r = GetFrameClientArea(rect, flags & ~wxTOPLEVEL_TITLEBAR);
    r.height = FRAME_TITLEBAR_HEIGHT;

    DrawBackground(dc, col, r);
}

void wxStdRenderer::DrawFrameTitle(wxDC& dc,
                                   const wxRect& rect,
                                   const wxString& title,
                                   int flags)
{
    wxColour col = m_scheme->Get(flags & wxTOPLEVEL_ACTIVE
                                    ? wxColourScheme::TITLEBAR_ACTIVE_TEXT
                                    : wxColourScheme::TITLEBAR_TEXT);
    dc.SetTextForeground(col);

    wxRect r = GetFrameClientArea(rect, flags & ~wxTOPLEVEL_TITLEBAR);
    r.height = FRAME_TITLEBAR_HEIGHT;
    if ( flags & wxTOPLEVEL_ICON )
    {
        r.x += FRAME_TITLEBAR_HEIGHT;
        r.width -= FRAME_TITLEBAR_HEIGHT + 2;
    }
    else
    {
        r.x += 1;
        r.width -= 3;
    }

    if ( flags & wxTOPLEVEL_BUTTON_CLOSE )
        r.width -= FRAME_BUTTON_WIDTH + 2;
    if ( flags & wxTOPLEVEL_BUTTON_MAXIMIZE )
        r.width -= FRAME_BUTTON_WIDTH;
    if ( flags & wxTOPLEVEL_BUTTON_RESTORE )
        r.width -= FRAME_BUTTON_WIDTH;
    if ( flags & wxTOPLEVEL_BUTTON_ICONIZE )
        r.width -= FRAME_BUTTON_WIDTH;
    if ( flags & wxTOPLEVEL_BUTTON_HELP )
        r.width -= FRAME_BUTTON_WIDTH;

    dc.SetFont(m_titlebarFont);

    wxString s;
    wxCoord textW;
    dc.GetTextExtent(title, &textW, NULL);
    if ( textW > r.width )
    {
        // text is too big, let's shorten it and add "..." after it:
        size_t len = title.length();
        wxCoord WSoFar, letterW;

        dc.GetTextExtent(wxT("..."), &WSoFar, NULL);
        if ( WSoFar > r.width )
        {
            // not enough space to draw anything
            return;
        }

        s.Alloc(len);
        for (size_t i = 0; i < len; i++)
        {
            dc.GetTextExtent(title[i], &letterW, NULL);
            if ( letterW + WSoFar > r.width )
                break;
            WSoFar += letterW;
            s << title[i];
        }
        s << wxT("...");
    }
    else // no need to truncate the title
    {
        s = title;
    }

    dc.DrawLabel(s, wxNullBitmap, r, wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
}

void wxStdRenderer::DrawFrameIcon(wxDC& dc,
                                  const wxRect& rect,
                                  const wxIcon& icon,
                                  int flags)
{
    if ( icon.Ok() )
    {
        wxRect r = GetFrameClientArea(rect, flags & ~wxTOPLEVEL_TITLEBAR);
        dc.DrawIcon(icon, r.x, r.y);
    }
}

void wxStdRenderer::DrawFrameButton(wxDC& dc,
                                    wxCoord x, wxCoord y,
                                    int button,
                                    int flags)
{
    FrameButtonType idx;
    switch (button)
    {
        case wxTOPLEVEL_BUTTON_CLOSE:    idx = FrameButton_Close; break;
        case wxTOPLEVEL_BUTTON_MAXIMIZE: idx = FrameButton_Maximize; break;
        case wxTOPLEVEL_BUTTON_ICONIZE:  idx = FrameButton_Minimize; break;
        case wxTOPLEVEL_BUTTON_RESTORE:  idx = FrameButton_Restore; break;
        case wxTOPLEVEL_BUTTON_HELP:     idx = FrameButton_Help; break;
        default:
            wxFAIL_MSG(wxT("incorrect button specification"));
            return;
    }

    wxBitmap bmp = GetFrameButtonBitmap(idx);
    if ( !bmp.Ok() )
        return;

    wxRect rectBtn(x, y, FRAME_BUTTON_WIDTH, FRAME_BUTTON_HEIGHT);
    if ( flags & wxCONTROL_PRESSED )
    {
        DrawSunkenBorder(dc, &rectBtn);

        rectBtn.Offset(1, 1);
    }
    else
    {
        DrawRaisedBorder(dc, &rectBtn);
    }

    DrawBackground(dc, wxSCHEME_COLOUR(m_scheme, CONTROL), rectBtn);

    wxRect rectBmp(0, 0, bmp.GetWidth(), bmp.GetHeight());
    dc.DrawBitmap(bmp, rectBmp.CentreIn(rectBtn).GetPosition(), true);
}

int wxStdRenderer::GetFrameBorderWidth(int flags) const
{
    return flags & wxTOPLEVEL_RESIZEABLE ? 4 : 3;
}


wxRect wxStdRenderer::GetFrameClientArea(const wxRect& rect, int flags) const
{
    wxRect r(rect);

    if ( (flags & wxTOPLEVEL_BORDER) && !(flags & wxTOPLEVEL_MAXIMIZED) )
    {
        r.Inflate(-GetFrameBorderWidth(flags));
    }

    if ( flags & wxTOPLEVEL_TITLEBAR )
    {
        r.y += FRAME_TITLEBAR_HEIGHT;
        r.height -= FRAME_TITLEBAR_HEIGHT;
    }

    return r;
}

wxSize
wxStdRenderer::GetFrameTotalSize(const wxSize& clientSize, int flags) const
{
    wxSize s(clientSize);

    if ( (flags & wxTOPLEVEL_BORDER) && !(flags & wxTOPLEVEL_MAXIMIZED) )
    {
        s.IncBy(2*GetFrameBorderWidth(flags));
    }

    if ( flags & wxTOPLEVEL_TITLEBAR )
        s.y += FRAME_TITLEBAR_HEIGHT;

    return s;
}

wxSize wxStdRenderer::GetFrameMinSize(int flags) const
{
    wxSize s;

    if ( (flags & wxTOPLEVEL_BORDER) && !(flags & wxTOPLEVEL_MAXIMIZED) )
    {
        s.IncBy(2*GetFrameBorderWidth(flags));
    }

    if ( flags & wxTOPLEVEL_TITLEBAR )
    {
        s.y += FRAME_TITLEBAR_HEIGHT;

        if ( flags & wxTOPLEVEL_ICON )
            s.x += FRAME_TITLEBAR_HEIGHT + 2;
        if ( flags & wxTOPLEVEL_BUTTON_CLOSE )
            s.x += FRAME_BUTTON_WIDTH + 2;
        if ( flags & wxTOPLEVEL_BUTTON_MAXIMIZE )
            s.x += FRAME_BUTTON_WIDTH;
        if ( flags & wxTOPLEVEL_BUTTON_RESTORE )
            s.x += FRAME_BUTTON_WIDTH;
        if ( flags & wxTOPLEVEL_BUTTON_ICONIZE )
            s.x += FRAME_BUTTON_WIDTH;
        if ( flags & wxTOPLEVEL_BUTTON_HELP )
            s.x += FRAME_BUTTON_WIDTH;
    }

    return s;
}

wxSize wxStdRenderer::GetFrameIconSize() const
{
    return wxSize(16, 16);
}
