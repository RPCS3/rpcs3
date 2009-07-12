/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/vscroll.cpp
// Purpose:     wxVScrolledWindow implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.05.03
// RCS-ID:      $Id: vscroll.cpp 57359 2008-12-15 19:09:31Z BP $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
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

#ifndef WX_PRECOMP
    #include "wx/sizer.h"
#endif

#include "wx/vscroll.h"

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxVScrolledWindow, wxPanel)
    EVT_SIZE(wxVScrolledWindow::OnSize)
    EVT_SCROLLWIN(wxVScrolledWindow::OnScroll)
#if wxUSE_MOUSEWHEEL
    EVT_MOUSEWHEEL(wxVScrolledWindow::OnMouseWheel)
#endif
END_EVENT_TABLE()


// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_ABSTRACT_CLASS(wxVScrolledWindow, wxPanel)

// ----------------------------------------------------------------------------
// initialization
// ----------------------------------------------------------------------------

void wxVScrolledWindow::Init()
{
    // we're initially empty
    m_lineMax =
    m_lineFirst = 0;

    // this one should always be strictly positive
    m_nVisible = 1;

    m_heightTotal = 0;

#if wxUSE_MOUSEWHEEL
    m_sumWheelRotation = 0;
#endif
}

// ----------------------------------------------------------------------------
// various helpers
// ----------------------------------------------------------------------------

wxCoord wxVScrolledWindow::EstimateTotalHeight() const
{
    // estimate the total height: it is impossible to call
    // OnGetLineHeight() for every line because there may be too many of
    // them, so we just make a guess using some lines in the beginning,
    // some in the end and some in the middle
    static const size_t NUM_LINES_TO_SAMPLE = 10;

    wxCoord heightTotal;
    if ( m_lineMax < 3*NUM_LINES_TO_SAMPLE )
    {
        // in this case calculating exactly is faster and more correct than
        // guessing
        heightTotal = GetLinesHeight(0, m_lineMax);
    }
    else // too many lines to calculate exactly
    {
        // look at some lines in the beginning/middle/end
        heightTotal =
            GetLinesHeight(0, NUM_LINES_TO_SAMPLE) +
                GetLinesHeight(m_lineMax - NUM_LINES_TO_SAMPLE, m_lineMax) +
                    GetLinesHeight(m_lineMax/2 - NUM_LINES_TO_SAMPLE/2,
                                   m_lineMax/2 + NUM_LINES_TO_SAMPLE/2);

        // use the height of the lines we looked as the average
        heightTotal = (wxCoord)
                (((float)heightTotal / (3*NUM_LINES_TO_SAMPLE)) * m_lineMax);
    }

    return heightTotal;
}

wxCoord wxVScrolledWindow::GetLinesHeight(size_t lineMin, size_t lineMax) const
{
    if ( lineMin == lineMax )
        return 0;
    else if ( lineMin > lineMax )
        return -GetLinesHeight(lineMax, lineMin);
    //else: lineMin < lineMax

    // let the user code know that we're going to need all these lines
    OnGetLinesHint(lineMin, lineMax);

    // do sum up their heights
    wxCoord height = 0;
    for ( size_t line = lineMin; line < lineMax; line++ )
    {
        height += OnGetLineHeight(line);
    }

    return height;
}

size_t wxVScrolledWindow::FindFirstFromBottom(size_t lineLast, bool full)
{
    const wxCoord hWindow = GetClientSize().y;

    // go upwards until we arrive at a line such that lineLast is not visible
    // any more when it is shown
    size_t lineFirst = lineLast;
    wxCoord h = 0;
    for ( ;; )
    {
        h += OnGetLineHeight(lineFirst);

        if ( h > hWindow )
        {
            // for this line to be fully visible we need to go one line
            // down, but if it is enough for it to be only partly visible then
            // this line will do as well
            if ( full )
            {
                lineFirst++;
            }

            break;
        }

        if ( !lineFirst )
            break;

        lineFirst--;
    }

    return lineFirst;
}

void wxVScrolledWindow::RemoveScrollbar()
{
    m_lineFirst = 0;
    m_nVisible = m_lineMax;
    SetScrollbar(wxVERTICAL, 0, 0, 0);
}

void wxVScrolledWindow::UpdateScrollbar()
{
    // see how many lines can we fit on screen
    const wxCoord hWindow = GetClientSize().y;

    wxCoord h = 0;
    size_t line;
    for ( line = m_lineFirst; line < m_lineMax; line++ )
    {
        if ( h > hWindow )
            break;

        h += OnGetLineHeight(line);
    }

    // if we still have remaining space below, maybe we can fit everything?
    if ( h < hWindow )
    {
        wxCoord hAll = h;
        for ( size_t lineFirst = m_lineFirst; lineFirst > 0; lineFirst-- )
        {
            hAll += OnGetLineHeight(m_lineFirst - 1);
            if ( hAll > hWindow )
                break;
        }

        if ( hAll < hWindow )
        {
            // we don't need scrollbar at all
            RemoveScrollbar();
            return;
        }
    }

    m_nVisible = line - m_lineFirst;

    int pageSize = m_nVisible;
    if ( h > hWindow )
    {
        // last line is only partially visible, we still need the scrollbar and
        // so we have to "fix" pageSize because if it is equal to m_lineMax the
        // scrollbar is not shown at all under MSW
        pageSize--;
    }

    // set the scrollbar parameters to reflect this
    SetScrollbar(wxVERTICAL, m_lineFirst, pageSize, m_lineMax);
}

// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------

void wxVScrolledWindow::SetLineCount(size_t count)
{
    // save the number of lines
    m_lineMax = count;

    // and our estimate for their total height
    m_heightTotal = EstimateTotalHeight();

    // recalculate the scrollbars parameters
    if ( count )
    {
        m_lineFirst = 1;    // make sure it is != 0
        ScrollToLine(0);
    }
    else // no items
    {
        RemoveScrollbar();
    }
}

void wxVScrolledWindow::RefreshLine(size_t line)
{
    // is this line visible?
    if ( !IsVisible(line) )
    {
        // no, it is useless to do anything
        return;
    }

    // calculate the rect occupied by this line on screen
    wxRect rect;
    rect.width = GetClientSize().x;
    rect.height = OnGetLineHeight(line);
    for ( size_t n = GetVisibleBegin(); n < line; n++ )
    {
        rect.y += OnGetLineHeight(n);
    }

    // do refresh it
    RefreshRect(rect);
}

void wxVScrolledWindow::RefreshLines(size_t from, size_t to)
{
    wxASSERT_MSG( from <= to, _T("RefreshLines(): empty range") );

    // clump the range to just the visible lines -- it is useless to refresh
    // the other ones
    if ( from < GetVisibleBegin() )
        from = GetVisibleBegin();

    if ( to >= GetVisibleEnd() )
        to = GetVisibleEnd();
    else
        to++;

    // calculate the rect occupied by these lines on screen
    wxRect rect;
    rect.width = GetClientSize().x;
    for ( size_t nBefore = GetVisibleBegin(); nBefore < from; nBefore++ )
    {
        rect.y += OnGetLineHeight(nBefore);
    }

    for ( size_t nBetween = from; nBetween < to; nBetween++ )
    {
        rect.height += OnGetLineHeight(nBetween);
    }

    // do refresh it
    RefreshRect(rect);
}

void wxVScrolledWindow::RefreshAll()
{
    UpdateScrollbar();

    Refresh();
}

bool wxVScrolledWindow::Layout()
{
    if ( GetSizer() )
    {
        // adjust the sizer dimensions/position taking into account the
        // virtual size and scrolled position of the window.

        int w = 0, h = 0;
        GetVirtualSize(&w, &h);

        // x is always 0 so no variable needed
        int y = -GetLinesHeight(0, GetFirstVisibleLine());

        GetSizer()->SetDimension(0, y, w, h);
        return true;
    }

    // fall back to default for LayoutConstraints
    return wxPanel::Layout();
}

int wxVScrolledWindow::HitTest(wxCoord WXUNUSED(x), wxCoord y) const
{
    const size_t lineMax = GetVisibleEnd();
    for ( size_t line = GetVisibleBegin(); line < lineMax; line++ )
    {
        y -= OnGetLineHeight(line);
        if ( y < 0 )
            return line;
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

bool wxVScrolledWindow::ScrollToLine(size_t line)
{
    if ( !m_lineMax )
    {
        // we're empty, code below doesn't make sense in this case
        return false;
    }

    // determine the real first line to scroll to: we shouldn't scroll beyond
    // the end
    size_t lineFirstLast = FindFirstFromBottom(m_lineMax - 1, true);
    if ( line > lineFirstLast )
        line = lineFirstLast;

    // anything to do?
    if ( line == m_lineFirst )
    {
        // no
        return false;
    }


    // remember the currently shown lines for the refresh code below
    size_t lineFirstOld = GetVisibleBegin(),
           lineLastOld = GetVisibleEnd();

    m_lineFirst = line;


    // the size of scrollbar thumb could have changed
    UpdateScrollbar();


    // finally refresh the display -- but only redraw as few lines as possible
    // to avoid flicker
    if ( GetChildren().empty() &&
         (GetVisibleBegin() >= lineLastOld || GetVisibleEnd() <= lineFirstOld ) )
    {
        // the simplest case: we don't have any old lines left, just redraw
        // everything
        Refresh();
    }
    else // overlap between the lines we showed before and should show now
    {
        // Avoid scrolling visible parts of the screen on Mac
#ifdef __WXMAC__
        if (!IsShownOnScreen())
            Refresh();
        else
#endif
        ScrollWindow(0, GetLinesHeight(GetVisibleBegin(), lineFirstOld));
    }

    return true;
}

bool wxVScrolledWindow::ScrollLines(int lines)
{
    lines += m_lineFirst;
    if ( lines < 0 )
        lines = 0;

    return ScrollToLine(lines);
}

bool wxVScrolledWindow::ScrollPages(int pages)
{
    bool didSomething = false;

    while ( pages )
    {
        int line;
        if ( pages > 0 )
        {
            line = GetVisibleEnd();
            if ( line )
                line--;
            pages--;
        }
        else // pages < 0
        {
            line = FindFirstFromBottom(GetVisibleBegin());
            pages++;
        }

        didSomething = ScrollToLine(line);
    }

    return didSomething;
}

// ----------------------------------------------------------------------------
// event handling
// ----------------------------------------------------------------------------

void wxVScrolledWindow::OnSize(wxSizeEvent& event)
{
    UpdateScrollbar();

    event.Skip();
}

void wxVScrolledWindow::OnScroll(wxScrollWinEvent& event)
{
    size_t lineFirstNew;

    const wxEventType evtType = event.GetEventType();

    if ( evtType == wxEVT_SCROLLWIN_TOP )
    {
        lineFirstNew = 0;
    }
    else if ( evtType == wxEVT_SCROLLWIN_BOTTOM )
    {
        lineFirstNew = m_lineMax;
    }
    else if ( evtType == wxEVT_SCROLLWIN_LINEUP )
    {
        lineFirstNew = m_lineFirst ? m_lineFirst - 1 : 0;
    }
    else if ( evtType == wxEVT_SCROLLWIN_LINEDOWN )
    {
        lineFirstNew = m_lineFirst + 1;
    }
    else if ( evtType == wxEVT_SCROLLWIN_PAGEUP )
    {
        lineFirstNew = FindFirstFromBottom(m_lineFirst);
    }
    else if ( evtType == wxEVT_SCROLLWIN_PAGEDOWN )
    {
        lineFirstNew = GetVisibleEnd();
        if ( lineFirstNew )
            lineFirstNew--;
    }
    else if ( evtType == wxEVT_SCROLLWIN_THUMBRELEASE )
    {
        lineFirstNew = event.GetPosition();
    }
    else if ( evtType == wxEVT_SCROLLWIN_THUMBTRACK )
    {
        lineFirstNew = event.GetPosition();
    }

    else // unknown scroll event?
    {
        wxFAIL_MSG( _T("unknown scroll event type?") );
        return;
    }

    ScrollToLine(lineFirstNew);

#ifdef __WXMAC__
    Update();
#endif // __WXMAC__
}

#if wxUSE_MOUSEWHEEL

void wxVScrolledWindow::OnMouseWheel(wxMouseEvent& event)
{
    m_sumWheelRotation += event.GetWheelRotation();
    int delta = event.GetWheelDelta();

    // how much to scroll this time
    int units_to_scroll = -(m_sumWheelRotation/delta);
    if ( !units_to_scroll )
        return;

    m_sumWheelRotation += units_to_scroll*delta;

    if ( !event.IsPageScroll() )
        ScrollLines( units_to_scroll*event.GetLinesPerAction() );
    else
        // scroll pages instead of lines
        ScrollPages( units_to_scroll );
}

#endif // wxUSE_MOUSEWHEEL

