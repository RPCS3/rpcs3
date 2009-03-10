/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/sashwin.cpp
// Purpose:     wxSashWindow implementation. A sash window has an optional
//              sash on each edge, allowing it to be dragged. An event
//              is generated when the sash is released.
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: sashwin.cpp 51249 2008-01-16 13:50:23Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SASH

#include "wx/sashwin.h"

#ifndef WX_PRECOMP
    #include "wx/dialog.h"
    #include "wx/frame.h"
    #include "wx/settings.h"
    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"
    #include "wx/math.h"
#endif

#include <stdlib.h>

#include "wx/laywin.h"

DEFINE_EVENT_TYPE(wxEVT_SASH_DRAGGED)

IMPLEMENT_DYNAMIC_CLASS(wxSashWindow, wxWindow)
IMPLEMENT_DYNAMIC_CLASS(wxSashEvent, wxCommandEvent)

BEGIN_EVENT_TABLE(wxSashWindow, wxWindow)
    EVT_PAINT(wxSashWindow::OnPaint)
    EVT_SIZE(wxSashWindow::OnSize)
    EVT_MOUSE_EVENTS(wxSashWindow::OnMouseEvent)
#if defined( __WXMSW__ ) || defined( __WXMAC__)
    EVT_SET_CURSOR(wxSashWindow::OnSetCursor)
#endif // __WXMSW__ || __WXMAC__

END_EVENT_TABLE()

bool wxSashWindow::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style, const wxString& name)
{
    return wxWindow::Create(parent, id, pos, size, style, name);
}

wxSashWindow::~wxSashWindow()
{
    delete m_sashCursorWE;
    delete m_sashCursorNS;
}

void wxSashWindow::Init()
{
    m_draggingEdge = wxSASH_NONE;
    m_dragMode = wxSASH_DRAG_NONE;
    m_oldX = 0;
    m_oldY = 0;
    m_firstX = 0;
    m_firstY = 0;
    m_borderSize = 3;
    m_extraBorderSize = 0;
    m_minimumPaneSizeX = 0;
    m_minimumPaneSizeY = 0;
    m_maximumPaneSizeX = 10000;
    m_maximumPaneSizeY = 10000;
    m_sashCursorWE = new wxCursor(wxCURSOR_SIZEWE);
    m_sashCursorNS = new wxCursor(wxCURSOR_SIZENS);
    m_mouseCaptured = false;
    m_currentCursor = NULL;

    // Eventually, we'll respond to colour change messages
    InitColours();
}

void wxSashWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    DrawBorders(dc);
    DrawSashes(dc);
}

void wxSashWindow::OnMouseEvent(wxMouseEvent& event)
{
    wxCoord x = 0, y = 0;
    event.GetPosition(&x, &y);

    wxSashEdgePosition sashHit = SashHitTest(x, y);

    if (event.LeftDown())
    {
        CaptureMouse();
        m_mouseCaptured = true;

        if ( sashHit != wxSASH_NONE )
        {
            // Required for X to specify that
            // that we wish to draw on top of all windows
            // - and we optimise by specifying the area
            // for creating the overlap window.
            // Find the first frame or dialog and use this to specify
            // the area to draw on.
            wxWindow* parent = this;

            while (parent && !parent->IsKindOf(CLASSINFO(wxDialog)) &&
                             !parent->IsKindOf(CLASSINFO(wxFrame)))
              parent = parent->GetParent();

            wxScreenDC::StartDrawingOnTop(parent);

            // We don't say we're dragging yet; we leave that
            // decision for the Dragging() branch, to ensure
            // the user has dragged a little bit.
            m_dragMode = wxSASH_DRAG_LEFT_DOWN;
            m_draggingEdge = sashHit;
            m_firstX = x;
            m_firstY = y;

            if ( (sashHit == wxSASH_LEFT) || (sashHit == wxSASH_RIGHT) )
            {
                if (m_currentCursor != m_sashCursorWE)
                {
                    SetCursor(*m_sashCursorWE);
                }
                m_currentCursor = m_sashCursorWE;
            }
            else
            {
                if (m_currentCursor != m_sashCursorNS)
                {
                    SetCursor(*m_sashCursorNS);
                }
                m_currentCursor = m_sashCursorNS;
            }
        }
    }
    else if ( event.LeftUp() && m_dragMode == wxSASH_DRAG_LEFT_DOWN )
    {
        // Wasn't a proper drag
        if (m_mouseCaptured)
            ReleaseMouse();
        m_mouseCaptured = false;

        wxScreenDC::EndDrawingOnTop();
        m_dragMode = wxSASH_DRAG_NONE;
        m_draggingEdge = wxSASH_NONE;
    }
    else if (event.LeftUp() && m_dragMode == wxSASH_DRAG_DRAGGING)
    {
        // We can stop dragging now and see what we've got.
        m_dragMode = wxSASH_DRAG_NONE;
        if (m_mouseCaptured)
            ReleaseMouse();
        m_mouseCaptured = false;

        // Erase old tracker
        DrawSashTracker(m_draggingEdge, m_oldX, m_oldY);

        // End drawing on top (frees the window used for drawing
        // over the screen)
        wxScreenDC::EndDrawingOnTop();

        int w, h;
        GetSize(&w, &h);
        int xp, yp;
        GetPosition(&xp, &yp);

        wxSashEdgePosition edge = m_draggingEdge;
        m_draggingEdge = wxSASH_NONE;

        wxRect dragRect;
        wxSashDragStatus status = wxSASH_STATUS_OK;

        // the new height and width of the window - if -1, it didn't change
        int newHeight = wxDefaultCoord,
            newWidth = wxDefaultCoord;

        // NB: x and y may be negative and they're relative to the sash window
        //     upper left corner, while xp and yp are expressed in the parent
        //     window system of coordinates, so adjust them! After this
        //     adjustment, all coordinates are relative to the parent window.
        y += yp;
        x += xp;

        switch (edge)
        {
            case wxSASH_TOP:
                if ( y > yp + h )
                {
                    // top sash shouldn't get below the bottom one
                    status = wxSASH_STATUS_OUT_OF_RANGE;
                }
                else
                {
                    newHeight = h - (y - yp);
                }
                break;

            case wxSASH_BOTTOM:
                if ( y < yp )
                {
                    // bottom sash shouldn't get above the top one
                    status = wxSASH_STATUS_OUT_OF_RANGE;
                }
                else
                {
                    newHeight = y - yp;
                }
                break;

            case wxSASH_LEFT:
                if ( x > xp + w )
                {
                    // left sash shouldn't get beyond the right one
                    status = wxSASH_STATUS_OUT_OF_RANGE;
                }
                else
                {
                    newWidth = w - (x - xp);
                }
                break;

            case wxSASH_RIGHT:
                if ( x < xp )
                {
                    // and the right sash, finally, shouldn't be beyond the
                    // left one
                    status = wxSASH_STATUS_OUT_OF_RANGE;
                }
                else
                {
                    newWidth = x - xp;
                }
                break;

            case wxSASH_NONE:
                // can this happen at all?
                break;
        }

        if ( newHeight == wxDefaultCoord )
        {
            // didn't change
            newHeight = h;
        }
        else
        {
            // make sure it's in m_minimumPaneSizeY..m_maximumPaneSizeY range
            newHeight = wxMax(newHeight, m_minimumPaneSizeY);
            newHeight = wxMin(newHeight, m_maximumPaneSizeY);
        }

        if ( newWidth == wxDefaultCoord )
        {
            // didn't change
            newWidth = w;
        }
        else
        {
            // make sure it's in m_minimumPaneSizeY..m_maximumPaneSizeY range
            newWidth = wxMax(newWidth, m_minimumPaneSizeX);
            newWidth = wxMin(newWidth, m_maximumPaneSizeX);
        }

        dragRect = wxRect(x, y, newWidth, newHeight);

        wxSashEvent eventSash(GetId(), edge);
        eventSash.SetEventObject(this);
        eventSash.SetDragStatus(status);
        eventSash.SetDragRect(dragRect);
        GetEventHandler()->ProcessEvent(eventSash);
    }
    else if ( event.LeftUp() )
    {
        if (m_mouseCaptured)
           ReleaseMouse();
        m_mouseCaptured = false;
    }
    else if ((event.Moving() || event.Leaving()) && !event.Dragging())
    {
        // Just change the cursor if required
        if ( sashHit != wxSASH_NONE )
        {
            if ( (sashHit == wxSASH_LEFT) || (sashHit == wxSASH_RIGHT) )
            {
                if (m_currentCursor != m_sashCursorWE)
                {
                    SetCursor(*m_sashCursorWE);
                }
                m_currentCursor = m_sashCursorWE;
            }
            else
            {
                if (m_currentCursor != m_sashCursorNS)
                {
                    SetCursor(*m_sashCursorNS);
                }
                m_currentCursor = m_sashCursorNS;
            }
        }
        else
        {
            SetCursor(wxNullCursor);
            m_currentCursor = NULL;
        }
    }
    else if ( event.Dragging() &&
              ((m_dragMode == wxSASH_DRAG_DRAGGING) ||
               (m_dragMode == wxSASH_DRAG_LEFT_DOWN)) )
    {
        if ( (m_draggingEdge == wxSASH_LEFT) || (m_draggingEdge == wxSASH_RIGHT) )
        {
            if (m_currentCursor != m_sashCursorWE)
            {
                SetCursor(*m_sashCursorWE);
            }
            m_currentCursor = m_sashCursorWE;
        }
        else
        {
            if (m_currentCursor != m_sashCursorNS)
            {
                SetCursor(*m_sashCursorNS);
            }
            m_currentCursor = m_sashCursorNS;
        }

        if (m_dragMode == wxSASH_DRAG_LEFT_DOWN)
        {
            m_dragMode = wxSASH_DRAG_DRAGGING;
            DrawSashTracker(m_draggingEdge, x, y);
        }
        else
        {
            if ( m_dragMode == wxSASH_DRAG_DRAGGING )
            {
                // Erase old tracker
                DrawSashTracker(m_draggingEdge, m_oldX, m_oldY);

                // Draw new one
                DrawSashTracker(m_draggingEdge, x, y);
            }
        }
        m_oldX = x;
        m_oldY = y;
    }
    else if ( event.LeftDClick() )
    {
        // Nothing
    }
    else
    {
    }
}

void wxSashWindow::OnSize(wxSizeEvent& WXUNUSED(event))
{
    SizeWindows();
}

wxSashEdgePosition wxSashWindow::SashHitTest(int x, int y, int WXUNUSED(tolerance))
{
    int cx, cy;
    GetClientSize(& cx, & cy);

    int i;
    for (i = 0; i < 4; i++)
    {
        wxSashEdge& edge = m_sashes[i];
        wxSashEdgePosition position = (wxSashEdgePosition) i ;

        if (edge.m_show)
        {
            switch (position)
            {
                case wxSASH_TOP:
                {
                    if (y >= 0 && y <= GetEdgeMargin(position))
                        return wxSASH_TOP;
                    break;
                }
                case wxSASH_RIGHT:
                {
                    if ((x >= cx - GetEdgeMargin(position)) && (x <= cx))
                        return wxSASH_RIGHT;
                    break;
                }
                case wxSASH_BOTTOM:
                {
                    if ((y >= cy - GetEdgeMargin(position)) && (y <= cy))
                        return wxSASH_BOTTOM;
                    break;
                }
                case wxSASH_LEFT:
                {
                    if ((x <= GetEdgeMargin(position)) && (x >= 0))
                        return wxSASH_LEFT;
                    break;
                }
                case wxSASH_NONE:
                {
                    break;
                }
            }
        }
    }
    return wxSASH_NONE;
}

// Draw 3D effect borders
void wxSashWindow::DrawBorders(wxDC& dc)
{
    int w, h;
    GetClientSize(&w, &h);

    wxPen mediumShadowPen(m_mediumShadowColour, 1, wxSOLID);
    wxPen darkShadowPen(m_darkShadowColour, 1, wxSOLID);
    wxPen lightShadowPen(m_lightShadowColour, 1, wxSOLID);
    wxPen hilightPen(m_hilightColour, 1, wxSOLID);

    if ( GetWindowStyleFlag() & wxSW_3DBORDER )
    {
        dc.SetPen(mediumShadowPen);
        dc.DrawLine(0, 0, w-1, 0);
        dc.DrawLine(0, 0, 0, h - 1);

        dc.SetPen(darkShadowPen);
        dc.DrawLine(1, 1, w-2, 1);
        dc.DrawLine(1, 1, 1, h-2);

        dc.SetPen(hilightPen);
        dc.DrawLine(0, h-1, w-1, h-1);
        dc.DrawLine(w-1, 0, w-1, h); // Surely the maximum y pos. should be h - 1.
                                     /// Anyway, h is required for MSW.

        dc.SetPen(lightShadowPen);
        dc.DrawLine(w-2, 1, w-2, h-2); // Right hand side
        dc.DrawLine(1, h-2, w-1, h-2);     // Bottom
    }
    else if ( GetWindowStyleFlag() & wxSW_BORDER )
    {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawRectangle(0, 0, w-1, h-1);
    }

    dc.SetPen(wxNullPen);
    dc.SetBrush(wxNullBrush);
}

void wxSashWindow::DrawSashes(wxDC& dc)
{
    int i;
    for (i = 0; i < 4; i++)
        if (m_sashes[i].m_show)
            DrawSash((wxSashEdgePosition) i, dc);
}

// Draw the sash
void wxSashWindow::DrawSash(wxSashEdgePosition edge, wxDC& dc)
{
    int w, h;
    GetClientSize(&w, &h);

    wxPen facePen(m_faceColour, 1, wxSOLID);
    wxBrush faceBrush(m_faceColour, wxSOLID);
    wxPen mediumShadowPen(m_mediumShadowColour, 1, wxSOLID);
    wxPen darkShadowPen(m_darkShadowColour, 1, wxSOLID);
    wxPen lightShadowPen(m_lightShadowColour, 1, wxSOLID);
    wxPen hilightPen(m_hilightColour, 1, wxSOLID);
    wxColour blackClr(0, 0, 0);
    wxColour whiteClr(255, 255, 255);
    wxPen blackPen(blackClr, 1, wxSOLID);
    wxPen whitePen(whiteClr, 1, wxSOLID);

    if ( edge == wxSASH_LEFT || edge == wxSASH_RIGHT )
    {
        int sashPosition = (edge == wxSASH_LEFT) ? 0 : ( w - GetEdgeMargin(edge) );

        dc.SetPen(facePen);
        dc.SetBrush(faceBrush);
        dc.DrawRectangle(sashPosition, 0, GetEdgeMargin(edge), h);

        if (GetWindowStyleFlag() & wxSW_3DSASH)
        {
            if (edge == wxSASH_LEFT)
            {
                // Draw a dark grey line on the left to indicate that the
                // sash is raised
                dc.SetPen(mediumShadowPen);
                dc.DrawLine(GetEdgeMargin(edge), 0, GetEdgeMargin(edge), h);
            }
            else
            {
                // Draw a highlight line on the right to indicate that the
                // sash is raised
                dc.SetPen(hilightPen);
                dc.DrawLine(w - GetEdgeMargin(edge), 0, w - GetEdgeMargin(edge), h);
            }
        }
    }
    else // top or bottom
    {
        int sashPosition = (edge == wxSASH_TOP) ? 0 : ( h - GetEdgeMargin(edge) );

        dc.SetPen(facePen);
        dc.SetBrush(faceBrush);
        dc.DrawRectangle(0, sashPosition, w, GetEdgeMargin(edge));

        if (GetWindowStyleFlag() & wxSW_3DSASH)
        {
            if (edge == wxSASH_BOTTOM)
            {
                // Draw a highlight line on the bottom to indicate that the
                // sash is raised
                dc.SetPen(hilightPen);
                dc.DrawLine(0, h - GetEdgeMargin(edge), w, h - GetEdgeMargin(edge));
            }
            else
            {
                // Draw a drak grey line on the top to indicate that the
                // sash is raised
                dc.SetPen(mediumShadowPen);
                dc.DrawLine(1, GetEdgeMargin(edge), w-1, GetEdgeMargin(edge));
            }
        }
    }

    dc.SetPen(wxNullPen);
    dc.SetBrush(wxNullBrush);
}

// Draw the sash tracker (for whilst moving the sash)
void wxSashWindow::DrawSashTracker(wxSashEdgePosition edge, int x, int y)
{
    int w, h;
    GetClientSize(&w, &h);

    wxScreenDC screenDC;
    int x1, y1;
    int x2, y2;

    if ( edge == wxSASH_LEFT || edge == wxSASH_RIGHT )
    {
        x1 = x; y1 = 2;
        x2 = x; y2 = h-2;

        if ( (edge == wxSASH_LEFT) && (x1 > w) )
        {
            x1 = w; x2 = w;
        }
        else if ( (edge == wxSASH_RIGHT) && (x1 < 0) )
        {
            x1 = 0; x2 = 0;
        }
    }
    else
    {
        x1 = 2; y1 = y;
        x2 = w-2; y2 = y;

        if ( (edge == wxSASH_TOP) && (y1 > h) )
        {
            y1 = h;
            y2 = h;
        }
        else if ( (edge == wxSASH_BOTTOM) && (y1 < 0) )
        {
            y1 = 0;
            y2 = 0;
        }
    }

    ClientToScreen(&x1, &y1);
    ClientToScreen(&x2, &y2);

    wxPen sashTrackerPen(*wxBLACK, 2, wxSOLID);

    screenDC.SetLogicalFunction(wxINVERT);
    screenDC.SetPen(sashTrackerPen);
    screenDC.SetBrush(*wxTRANSPARENT_BRUSH);

    screenDC.DrawLine(x1, y1, x2, y2);

    screenDC.SetLogicalFunction(wxCOPY);

    screenDC.SetPen(wxNullPen);
    screenDC.SetBrush(wxNullBrush);
}

// Position and size subwindows.
// Note that the border size applies to each subwindow, not
// including the edges next to the sash.
void wxSashWindow::SizeWindows()
{
    int cw, ch;
    GetClientSize(&cw, &ch);

    if (GetChildren().GetCount() == 1)
    {
        wxWindow* child = GetChildren().GetFirst()->GetData();

        int x = 0;
        int y = 0;
        int width = cw;
        int height = ch;

        // Top
        if (m_sashes[0].m_show)
        {
            y = m_borderSize;
            height -= m_borderSize;
        }
        y += m_extraBorderSize;

        // Left
        if (m_sashes[3].m_show)
        {
            x = m_borderSize;
            width -= m_borderSize;
        }
        x += m_extraBorderSize;

        // Right
        if (m_sashes[1].m_show)
        {
            width -= m_borderSize;
        }
        width -= 2*m_extraBorderSize;

        // Bottom
        if (m_sashes[2].m_show)
        {
            height -= m_borderSize;
        }
        height -= 2*m_extraBorderSize;

        child->SetSize(x, y, width, height);
    }
    else if (GetChildren().GetCount() > 1)
    {
        // Perhaps multiple children are themselves sash windows.
        // TODO: this doesn't really work because the subwindows sizes/positions
        // must be set to leave a gap for the parent's sash (hit-test and decorations).
        // Perhaps we can allow for this within LayoutWindow, testing whether the parent
        // is a sash window, and if so, allowing some space for the edges.
        wxLayoutAlgorithm layout;
        layout.LayoutWindow(this);
    }

    wxClientDC dc(this);
    DrawBorders(dc);
    DrawSashes(dc);
}

// Initialize colours
void wxSashWindow::InitColours()
{
    // Shadow colours
    m_faceColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
    m_mediumShadowColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW);
    m_darkShadowColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW);
    m_lightShadowColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
    m_hilightColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DHILIGHT);
}

void wxSashWindow::SetSashVisible(wxSashEdgePosition edge, bool sash)
{
     m_sashes[edge].m_show = sash;
     if (sash)
        m_sashes[edge].m_margin = m_borderSize;
     else
        m_sashes[edge].m_margin = 0;
}

#if defined( __WXMSW__ ) || defined( __WXMAC__)

// this is currently called (and needed) under MSW only...
void wxSashWindow::OnSetCursor(wxSetCursorEvent& event)
{
    // if we don't do it, the resizing cursor might be set for child window:
    // and like this we explicitly say that our cursor should not be used for
    // children windows which overlap us

    if ( SashHitTest(event.GetX(), event.GetY()) != wxSASH_NONE)
    {
        // default processing is ok
        event.Skip();
    }
    //else: do nothing, in particular, don't call Skip()
}

#endif // __WXMSW__ || __WXMAC__

#endif // wxUSE_SASH
