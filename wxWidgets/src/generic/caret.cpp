///////////////////////////////////////////////////////////////////////////////
// Name:        generic/caret.cpp
// Purpose:     generic wxCaret class implementation
// Author:      Vadim Zeitlin (original code by Robert Roebling)
// Modified by:
// Created:     25.05.99
// RCS-ID:      $Id: caret.cpp 55170 2008-08-22 10:34:32Z JS $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_CARET

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
#endif //WX_PRECOMP

#include "wx/caret.h"

// ----------------------------------------------------------------------------
// global variables for this module
// ----------------------------------------------------------------------------

// the blink time (common to all carets for MSW compatibility)
static int gs_blinkTime = 500;  // in milliseconds

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// timer stuff
// ----------------------------------------------------------------------------

wxCaretTimer::wxCaretTimer(wxCaret *caret)
{
    m_caret = caret;
}

void wxCaretTimer::Notify()
{
    m_caret->OnTimer();
}

void wxCaret::OnTimer()
{
    // don't blink the caret when we don't have the focus
    if ( m_hasFocus )
        Blink();
}

// ----------------------------------------------------------------------------
// wxCaret static functions and data
// ----------------------------------------------------------------------------

int wxCaretBase::GetBlinkTime()
{
    return gs_blinkTime;
}

void wxCaretBase::SetBlinkTime(int milliseconds)
{
    gs_blinkTime = milliseconds;
}

// ----------------------------------------------------------------------------
// initialization and destruction
// ----------------------------------------------------------------------------

void wxCaret::InitGeneric()
{
    m_hasFocus = true;
    m_blinkedOut = true;
#ifndef wxHAS_CARET_USING_OVERLAYS
    m_xOld =
    m_yOld = -1;
    m_bmpUnderCaret.Create(m_width, m_height);
#endif
}

wxCaret::~wxCaret()
{
    if ( IsVisible() )
    {
        // stop blinking
        if ( m_timer.IsRunning() )
            m_timer.Stop();
    }
}

// ----------------------------------------------------------------------------
// showing/hiding/moving the caret (base class interface)
// ----------------------------------------------------------------------------

void wxCaret::DoShow()
{
    int blinkTime = GetBlinkTime();
    if ( blinkTime )
        m_timer.Start(blinkTime);

    if ( m_blinkedOut )
        Blink();
}

void wxCaret::DoHide()
{
    m_timer.Stop();

    if ( !m_blinkedOut )
    {
        Blink();
    }
}

void wxCaret::DoMove()
{
#ifdef wxHAS_CARET_USING_OVERLAYS
    m_overlay.Reset();
#endif
    if ( IsVisible() )
    {
        if ( !m_blinkedOut )
        {
            // hide it right now and it will be shown the next time it blinks
            Blink();

            // but if the caret is not blinking, we should blink it back into
            // visibility manually
            if ( !m_timer.IsRunning() )
                Blink();
        }
    }
    //else: will be shown at the correct location when it is shown
}

void wxCaret::DoSize()
{
    int countVisible = m_countVisible;
    if (countVisible > 0)
    {
        m_countVisible = 0;
        DoHide();
    }
#ifdef wxHAS_CARET_USING_OVERLAYS
    m_overlay.Reset();
#else
    // Change bitmap size
    m_bmpUnderCaret = wxBitmap(m_width, m_height);
#endif
    if (countVisible > 0)
    {
        m_countVisible = countVisible;
        DoShow();
    }
}

// ----------------------------------------------------------------------------
// handling the focus
// ----------------------------------------------------------------------------

void wxCaret::OnSetFocus()
{
    m_hasFocus = true;

    if ( IsVisible() )
        Refresh();
}

void wxCaret::OnKillFocus()
{
    m_hasFocus = false;

    if ( IsVisible() )
    {
        // the caret must be shown - otherwise, if it is hidden now, it will
        // stay so until the focus doesn't return because it won't blink any
        // more

        // hide it first if it isn't hidden ...
        if ( !m_blinkedOut )
            Blink();

        // .. and show it in the new style
        Blink();
    }
}

// ----------------------------------------------------------------------------
// drawing the caret
// ----------------------------------------------------------------------------

void wxCaret::Blink()
{
    m_blinkedOut = !m_blinkedOut;

    Refresh();
}

void wxCaret::Refresh()
{
    wxClientDC dcWin(GetWindow());
// this is the new code, switch to 0 if this gives problems
#ifdef wxHAS_CARET_USING_OVERLAYS
    wxDCOverlay dcOverlay( m_overlay, &dcWin, m_x, m_y, m_width , m_height );
    if ( m_blinkedOut )
    {
        dcOverlay.Clear();
    }
    else
    {
        DoDraw( &dcWin );
    }
#else
    wxMemoryDC dcMem;
    dcMem.SelectObject(m_bmpUnderCaret);
    if ( m_blinkedOut )
    {
        // restore the old image
        dcWin.Blit(m_xOld, m_yOld, m_width, m_height,
                   &dcMem, 0, 0);
        m_xOld =
        m_yOld = -1;
    }
    else
    {
        if ( m_xOld == -1 && m_yOld == -1 )
        {
            // save the part we're going to overdraw

            int x = m_x,
                y = m_y;
#if defined(__WXGTK__) && !defined(__WX_DC_BLIT_FIXED__)
            wxPoint pt = dcWin.GetDeviceOrigin();
            x += pt.x;
            y += pt.y;
#endif // broken wxGTK wxDC::Blit
            dcMem.Blit(0, 0, m_width, m_height,
                       &dcWin, x, y);

            m_xOld = m_x;
            m_yOld = m_y;
        }
        //else: we already saved the image below the caret, don't do it any
        //      more

        // and draw the caret there
        DoDraw(&dcWin);
    }
#endif
}

void wxCaret::DoDraw(wxDC *dc)
{
#if defined(__WXGTK__) || defined(__WXMAC__)
    wxClientDC* clientDC = wxDynamicCast(dc, wxClientDC);
    if (clientDC)
    {
        wxPen pen(*wxBLACK_PEN);
        wxBrush brush(*wxBLACK_BRUSH);
#ifdef __WXGTK__
        wxWindow* win = clientDC->m_owner;
#else
        wxWindow* win = clientDC->GetWindow();
#endif
        if (win)
        {
            wxColour backgroundColour(win->GetBackgroundColour());
            if (backgroundColour.Red() < 100 &&
                backgroundColour.Green() < 100 &&
                backgroundColour.Blue() < 100)
            {
                pen = *wxWHITE_PEN;
                brush = *wxWHITE_BRUSH;
            }
        }
        dc->SetPen( pen );
        dc->SetBrush(m_hasFocus ? brush : *wxTRANSPARENT_BRUSH);
    }
    else
#endif
    {
        dc->SetBrush(*(m_hasFocus ? wxBLACK_BRUSH : wxTRANSPARENT_BRUSH));
        dc->SetPen(*wxBLACK_PEN);
    }

    // VZ: unfortunately, the rectangle comes out a pixel smaller when this is
    //     done under wxGTK - no idea why
    //dc->SetLogicalFunction(wxINVERT);

    dc->DrawRectangle(m_x, m_y, m_width, m_height);
}

#endif // wxUSE_CARET
