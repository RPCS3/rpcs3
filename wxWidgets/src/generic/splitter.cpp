/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/splitter.cpp
// Purpose:     wxSplitterWindow implementation
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: splitter.cpp 60837 2009-05-31 13:13:07Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SPLITTER

#include "wx/splitter.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/log.h"

    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"

    #include "wx/window.h"
    #include "wx/dialog.h"
    #include "wx/frame.h"

    #include "wx/settings.h"
#endif

#include "wx/renderer.h"

#include <stdlib.h>

DEFINE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_DOUBLECLICKED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SPLITTER_UNSPLIT)

IMPLEMENT_DYNAMIC_CLASS(wxSplitterWindow, wxWindow)

/*
    TODO PROPERTIES
        style wxSP_3D
        sashpos (long , 0 )
        minsize (long -1 )
        object, object_ref
        orientation
*/

IMPLEMENT_DYNAMIC_CLASS(wxSplitterEvent, wxNotifyEvent)

BEGIN_EVENT_TABLE(wxSplitterWindow, wxWindow)
    EVT_PAINT(wxSplitterWindow::OnPaint)
    EVT_SIZE(wxSplitterWindow::OnSize)
    EVT_MOUSE_EVENTS(wxSplitterWindow::OnMouseEvent)

#if defined( __WXMSW__ ) || defined( __WXMAC__)
    EVT_SET_CURSOR(wxSplitterWindow::OnSetCursor)
#endif // wxMSW

    WX_EVENT_TABLE_CONTROL_CONTAINER(wxSplitterWindow)
END_EVENT_TABLE()

WX_DELEGATE_TO_CONTROL_CONTAINER(wxSplitterWindow, wxWindow)

bool wxSplitterWindow::Create(wxWindow *parent, wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name)
{
    // allow TABbing from one window to the other
    style |= wxTAB_TRAVERSAL;

    // we draw our border ourselves to blend the sash with it
    style &= ~wxBORDER_MASK;
    style |= wxBORDER_NONE;

#if defined(__WXMAC__) && wxMAC_USE_CORE_GRAPHICS
    // CoreGraphics can't paint sash feedback
    style |= wxSP_LIVE_UPDATE;
#endif

    if ( !wxWindow::Create(parent, id, pos, size, style, name) )
        return false;

    if (size.x >= 0)
        m_lastSize.x = size.x;
    if (size.y >= 0)
        m_lastSize.y = size.y;

    m_permitUnsplitAlways = (style & wxSP_PERMIT_UNSPLIT) != 0;

    // FIXME: with this line the background is not erased at all under GTK1,
    //        so temporary avoid it there
#if !defined(__WXGTK__) || defined(__WXGTK20__)
    // don't erase the splitter background, it's pointless as we overwrite it
    // anyhow
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
#endif

    return true;
}

void wxSplitterWindow::Init()
{
    m_container.SetContainerWindow(this);

    m_splitMode = wxSPLIT_VERTICAL;
    m_permitUnsplitAlways = true;
    m_windowOne = (wxWindow *) NULL;
    m_windowTwo = (wxWindow *) NULL;
    m_dragMode = wxSPLIT_DRAG_NONE;
    m_oldX = 0;
    m_oldY = 0;
    m_firstX = 0;
    m_firstY = 0;
    m_sashPosition = m_requestedSashPosition = 0;
    m_sashGravity = 0.0;
    m_sashSize = -1; // -1 means use the native sash size
    m_lastSize = wxSize(0,0);
    m_checkRequestedSashPosition = false;
    m_minimumPaneSize = 0;
    m_sashCursorWE = wxCursor(wxCURSOR_SIZEWE);
    m_sashCursorNS = wxCursor(wxCURSOR_SIZENS);
    m_sashTrackerPen = new wxPen(*wxBLACK, 2, wxSOLID);

    m_needUpdating = false;
    m_isHot = false;
}

wxSplitterWindow::~wxSplitterWindow()
{
    delete m_sashTrackerPen;
}

// ----------------------------------------------------------------------------
// entering/leaving sash
// ----------------------------------------------------------------------------

void wxSplitterWindow::RedrawIfHotSensitive(bool isHot)
{
    if ( wxRendererNative::Get().GetSplitterParams(this).isHotSensitive )
    {
        m_isHot = isHot;

        wxClientDC dc(this);
        DrawSash(dc);
    }
    //else: we don't change our appearance, don't redraw to avoid flicker
}

void wxSplitterWindow::OnEnterSash()
{
    SetResizeCursor();

    RedrawIfHotSensitive(true);
}

void wxSplitterWindow::OnLeaveSash()
{
    SetCursor(*wxSTANDARD_CURSOR);

    RedrawIfHotSensitive(false);
}

void wxSplitterWindow::SetResizeCursor()
{
    SetCursor(m_splitMode == wxSPLIT_VERTICAL ? m_sashCursorWE
                                              : m_sashCursorNS);
}

// ----------------------------------------------------------------------------
// other event handlers
// ----------------------------------------------------------------------------

void wxSplitterWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    DrawSash(dc);
}

void wxSplitterWindow::OnInternalIdle()
{
    wxWindow::OnInternalIdle();

    // if this is the first idle time after a sash position has potentially
    // been set, allow SizeWindows to check for a requested size.
    if (!m_checkRequestedSashPosition)
    {
        m_checkRequestedSashPosition = true;
        SizeWindows();
        return; // it won't needUpdating in this case
    }

    if (m_needUpdating)
        SizeWindows();
}

void wxSplitterWindow::OnMouseEvent(wxMouseEvent& event)
{
    int x = (int)event.GetX(),
        y = (int)event.GetY();

    if ( GetWindowStyle() & wxSP_NOSASH )
    {
        event.Skip();
        return;
    }

    // with wxSP_LIVE_UPDATE style the splitter windows are always resized
    // following the mouse movement while it drags the sash, without it we only
    // draw the sash at the new position but only resize the windows when the
    // dragging is finished
#if defined( __WXMAC__ ) && defined(TARGET_API_MAC_OSX) && TARGET_API_MAC_OSX == 1
    bool isLive = true ; // FIXME: why?
#else
    bool isLive = HasFlag(wxSP_LIVE_UPDATE);
#endif
    if (event.LeftDown())
    {
        if ( SashHitTest(x, y) )
        {
            // Start the drag now
            m_dragMode = wxSPLIT_DRAG_DRAGGING;

            // Capture mouse and set the cursor
            CaptureMouse();
            SetResizeCursor();

            if ( !isLive )
            {
                // remember the initial sash position and draw the initial
                // shadow sash
                m_sashPositionCurrent = m_sashPosition;

                DrawSashTracker(x, y);
            }

            m_oldX = x;
            m_oldY = y;

            SetResizeCursor();
            return;
        }
    }
    else if (event.LeftUp() && m_dragMode == wxSPLIT_DRAG_DRAGGING)
    {
        // We can stop dragging now and see what we've got.
        m_dragMode = wxSPLIT_DRAG_NONE;

        // Release mouse and unset the cursor
        ReleaseMouse();
        SetCursor(* wxSTANDARD_CURSOR);

        // exit if unsplit after doubleclick
        if ( !IsSplit() )
        {
            return;
        }

        // Erase old tracker
        if ( !isLive )
        {
            DrawSashTracker(m_oldX, m_oldY);
        }

        // the position of the click doesn't exactly correspond to
        // m_sashPosition, rather it changes it by the distance by which the
        // mouse has moved
        int diff = m_splitMode == wxSPLIT_VERTICAL ? x - m_oldX : y - m_oldY;

        int posSashOld = isLive ? m_sashPosition : m_sashPositionCurrent;
        int posSashNew = OnSashPositionChanging(posSashOld + diff);
        if ( posSashNew == -1 )
        {
            // change not allowed
            return;
        }

        if ( m_permitUnsplitAlways || m_minimumPaneSize == 0 )
        {
            // Deal with possible unsplit scenarios
            if ( posSashNew == 0 )
            {
                // We remove the first window from the view
                wxWindow *removedWindow = m_windowOne;
                m_windowOne = m_windowTwo;
                m_windowTwo = (wxWindow *) NULL;
                OnUnsplit(removedWindow);
                wxSplitterEvent eventUnsplit(wxEVT_COMMAND_SPLITTER_UNSPLIT, this);
                eventUnsplit.m_data.win = removedWindow;
                (void)DoSendEvent(eventUnsplit);
                SetSashPositionAndNotify(0);
            }
            else if ( posSashNew == GetWindowSize() )
            {
                // We remove the second window from the view
                wxWindow *removedWindow = m_windowTwo;
                m_windowTwo = (wxWindow *) NULL;
                OnUnsplit(removedWindow);
                wxSplitterEvent eventUnsplit(wxEVT_COMMAND_SPLITTER_UNSPLIT, this);
                eventUnsplit.m_data.win = removedWindow;
                (void)DoSendEvent(eventUnsplit);
                SetSashPositionAndNotify(0);
            }
            else
            {
                SetSashPositionAndNotify(posSashNew);
            }
        }
        else
        {
            SetSashPositionAndNotify(posSashNew);
        }

        SizeWindows();
    }  // left up && dragging
    else if ((event.Moving() || event.Leaving() || event.Entering()) && (m_dragMode == wxSPLIT_DRAG_NONE))
    {
        if ( event.Leaving() || !SashHitTest(x, y) )
            OnLeaveSash();
        else
            OnEnterSash();
    }
    else if (event.Dragging() && (m_dragMode == wxSPLIT_DRAG_DRAGGING))
    {
        int diff = m_splitMode == wxSPLIT_VERTICAL ? x - m_oldX : y - m_oldY;
        if ( !diff )
        {
            // nothing to do, mouse didn't really move far enough
            return;
        }

        int posSashOld = isLive ? m_sashPosition : m_sashPositionCurrent;
        int posSashNew = OnSashPositionChanging(posSashOld + diff);
        if ( posSashNew == -1 )
        {
            // change not allowed
            return;
        }

        if ( posSashNew == m_sashPosition )
            return;

        // Erase old tracker
        if ( !isLive )
        {
            DrawSashTracker(m_oldX, m_oldY);
        }

        if (m_splitMode == wxSPLIT_VERTICAL)
            x = posSashNew;
        else
            y = posSashNew;

        // Remember old positions
        m_oldX = x;
        m_oldY = y;

#ifdef __WXMSW__
        // As we captured the mouse, we may get the mouse events from outside
        // our window - for example, negative values in x, y. This has a weird
        // consequence under MSW where we use unsigned values sometimes and
        // signed ones other times: the coordinates turn as big positive
        // numbers and so the sash is drawn on the *right* side of the window
        // instead of the left (or bottom instead of top). Correct this.
        if ( (short)m_oldX < 0 )
            m_oldX = 0;
        if ( (short)m_oldY < 0 )
            m_oldY = 0;
#endif // __WXMSW__

        // Draw new one
        if ( !isLive )
        {
            m_sashPositionCurrent = posSashNew;

            DrawSashTracker(m_oldX, m_oldY);
        }
        else
        {
            DoSetSashPosition(posSashNew);
            m_needUpdating = true;
        }
    }
    else if ( event.LeftDClick() && m_windowTwo )
    {
        OnDoubleClickSash(x, y);
    }
    else
    {
        event.Skip();
    }
}

void wxSplitterWindow::OnSize(wxSizeEvent& event)
{
    // only process this message if we're not iconized - otherwise iconizing
    // and restoring a window containing the splitter has a funny side effect
    // of changing the splitter position!
    wxWindow *parent = wxGetTopLevelParent(this);
    bool iconized;

    wxTopLevelWindow *winTop = wxDynamicCast(parent, wxTopLevelWindow);
    if ( winTop )
    {
        iconized = winTop->IsIconized();
    }
    else
    {
        wxFAIL_MSG(wxT("should have a top level parent!"));

        iconized = false;
    }

    if ( iconized )
    {
        m_lastSize = wxSize(0,0);

        event.Skip();

        return;
    }

    if ( m_windowTwo )
    {
        int w, h;
        GetClientSize(&w, &h);

        int size = m_splitMode == wxSPLIT_VERTICAL ? w : h;

        int old_size = m_splitMode == wxSPLIT_VERTICAL ? m_lastSize.x : m_lastSize.y;
        if ( old_size != 0 )
        {
            int delta = (int) ( (size - old_size)*m_sashGravity );
            if ( delta != 0 )
            {
                int newPosition = m_sashPosition + delta;
                if( newPosition < m_minimumPaneSize )
                    newPosition = m_minimumPaneSize;
                SetSashPositionAndNotify(newPosition);
            }
        }

        if ( m_sashPosition >= size - 5 )
            SetSashPositionAndNotify(wxMax(10, size - 40));
        m_lastSize = wxSize(w,h);
    }

    SizeWindows();
}

void wxSplitterWindow::SetSashGravity(double gravity)
{
    wxCHECK_RET( gravity >= 0. && gravity <= 1.,
                    _T("invalid gravity value") );

    m_sashGravity = gravity;
}

bool wxSplitterWindow::SashHitTest(int x, int y, int tolerance)
{
    if ( m_windowTwo == NULL || m_sashPosition == 0)
        return false; // No sash

    int z = m_splitMode == wxSPLIT_VERTICAL ? x : y;
    int hitMin = m_sashPosition - tolerance;
    int hitMax = m_sashPosition + GetSashSize() + tolerance;

    return z >=  hitMin && z <= hitMax;
}

int wxSplitterWindow::GetSashSize() const
{
    return m_sashSize > -1 ? m_sashSize : wxRendererNative::Get().GetSplitterParams(this).widthSash;
}

int wxSplitterWindow::GetBorderSize() const
{
    return wxRendererNative::Get().GetSplitterParams(this).border;
}

// Draw the sash
void wxSplitterWindow::DrawSash(wxDC& dc)
{
    if (HasFlag(wxSP_3DBORDER))
        wxRendererNative::Get().DrawSplitterBorder
                            (
                                this,
                                dc,
                                GetClientRect()
                            );

    // don't draw sash if we're not split
    if ( m_sashPosition == 0 || !m_windowTwo )
        return;

    // nor if we're configured to not show it
    if ( HasFlag(wxSP_NOSASH) )
        return;

    wxRendererNative::Get().DrawSplitterSash
                            (
                                this,
                                dc,
                                GetClientSize(),
                                m_sashPosition,
                                m_splitMode == wxSPLIT_VERTICAL ? wxVERTICAL
                                                                : wxHORIZONTAL,
                                m_isHot ? (int)wxCONTROL_CURRENT : 0
                            );
}

// Draw the sash tracker (for whilst moving the sash)
void wxSplitterWindow::DrawSashTracker(int x, int y)
{
    int w, h;
    GetClientSize(&w, &h);

    wxScreenDC screenDC;
    int x1, y1;
    int x2, y2;

    if ( m_splitMode == wxSPLIT_VERTICAL )
    {
        x1 = x; y1 = 2;
        x2 = x; y2 = h-2;

        if ( x1 > w )
        {
            x1 = w; x2 = w;
        }
        else if ( x1 < 0 )
        {
            x1 = 0; x2 = 0;
        }
    }
    else
    {
        x1 = 2; y1 = y;
        x2 = w-2; y2 = y;

        if ( y1 > h )
        {
            y1 = h;
            y2 = h;
        }
        else if ( y1 < 0 )
        {
            y1 = 0;
            y2 = 0;
        }
    }

    ClientToScreen(&x1, &y1);
    ClientToScreen(&x2, &y2);

    screenDC.SetLogicalFunction(wxINVERT);
    screenDC.SetPen(*m_sashTrackerPen);
    screenDC.SetBrush(*wxTRANSPARENT_BRUSH);

    screenDC.DrawLine(x1, y1, x2, y2);

    screenDC.SetLogicalFunction(wxCOPY);
}

int wxSplitterWindow::GetWindowSize() const
{
    wxSize size = GetClientSize();

    return m_splitMode == wxSPLIT_VERTICAL ? size.x : size.y;
}

int wxSplitterWindow::AdjustSashPosition(int sashPos) const
{
    wxWindow *win;

    win = GetWindow1();
    if ( win )
    {
        // the window shouldn't be smaller than its own minimal size nor
        // smaller than the minimual pane size specified for this splitter
        int minSize = m_splitMode == wxSPLIT_VERTICAL ? win->GetMinWidth()
                                                      : win->GetMinHeight();

        if ( minSize == -1 || m_minimumPaneSize > minSize )
            minSize = m_minimumPaneSize;

        minSize += GetBorderSize();

        if ( sashPos < minSize )
            sashPos = minSize;
    }

    win = GetWindow2();
    if ( win )
    {
        int minSize = m_splitMode == wxSPLIT_VERTICAL ? win->GetMinWidth()
                                                      : win->GetMinHeight();

        if ( minSize == -1 || m_minimumPaneSize > minSize )
            minSize = m_minimumPaneSize;

        int maxSize = GetWindowSize() - minSize - GetBorderSize() - GetSashSize();
        if ( maxSize > 0 && sashPos > maxSize && maxSize >= m_minimumPaneSize)
            sashPos = maxSize;
    }

    return sashPos;
}

bool wxSplitterWindow::DoSetSashPosition(int sashPos)
{
    int newSashPosition = AdjustSashPosition(sashPos);

    if ( newSashPosition == m_sashPosition )
        return false;

    m_sashPosition = newSashPosition;

    return true;
}

void wxSplitterWindow::SetSashPositionAndNotify(int sashPos)
{
    // we must reset the request here, otherwise the sash would be stuck at
    // old position if the user attempted to move the sash after invalid
    // (e.g. smaller than minsize) sash position was requested using
    // SetSashPosition():
    m_requestedSashPosition = INT_MAX;

    // note that we must send the event in any case, i.e. even if the sash
    // position hasn't changed and DoSetSashPosition() returns false because we
    // must generate a CHANGED event at the end of resizing
    DoSetSashPosition(sashPos);

    wxSplitterEvent event(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED, this);
    event.m_data.pos = m_sashPosition;

    (void)DoSendEvent(event);
}

// Position and size subwindows.
// Note that the border size applies to each subwindow, not
// including the edges next to the sash.
void wxSplitterWindow::SizeWindows()
{
    // check if we have delayed setting the real sash position
    if ( m_checkRequestedSashPosition && m_requestedSashPosition != INT_MAX )
    {
        int newSashPosition = ConvertSashPosition(m_requestedSashPosition);
        if ( newSashPosition != m_sashPosition )
        {
            DoSetSashPosition(newSashPosition);
        }

        if ( newSashPosition <= m_sashPosition
            && newSashPosition >= m_sashPosition - GetBorderSize() )
        {
            // don't update it any more
            m_requestedSashPosition = INT_MAX;
        }
    }

    int w, h;
    GetClientSize(&w, &h);

    if ( GetWindow1() && !GetWindow2() )
    {
        GetWindow1()->SetSize(GetBorderSize(), GetBorderSize(),
                              w - 2*GetBorderSize(), h - 2*GetBorderSize());
    }
    else if ( GetWindow1() && GetWindow2() )
    {
        const int border = GetBorderSize(),
                  sash = GetSashSize();

        int size1 = GetSashPosition() - border,
            size2 = GetSashPosition() + sash;

        int x2, y2, w1, h1, w2, h2;
        if ( GetSplitMode() == wxSPLIT_VERTICAL )
        {
            w1 = size1;
            w2 = w - 2*border - sash - w1;
            if (w2 < 0)
                w2 = 0;
            h2 = h - 2*border;
            if (h2 < 0)
                h2 = 0;
            h1 = h2;
            x2 = size2;
            y2 = border;
        }
        else // horz splitter
        {
            w2 = w - 2*border;
            if (w2 < 0)
                w2 = 0;
            w1 = w2;
            h1 = size1;
            h2 = h - 2*border - sash - h1;
            if (h2 < 0)
                h2 = 0;
            x2 = border;
            y2 = size2;
        }

        GetWindow2()->SetSize(x2, y2, w2, h2);
        GetWindow1()->SetSize(border, border, w1, h1);
    }

    wxClientDC dc(this);
    DrawSash(dc);

    SetNeedUpdating(false);
}

// Set pane for unsplit window
void wxSplitterWindow::Initialize(wxWindow *window)
{
    wxASSERT_MSG( (!window || (window && window->GetParent() == this)),
                  _T("windows in the splitter should have it as parent!") );

    if (window && !window->IsShown())
        window->Show();

    m_windowOne = window;
    m_windowTwo = (wxWindow *) NULL;
    DoSetSashPosition(0);
}

// Associates the given window with window 2, drawing the appropriate sash
// and changing the split mode.
// Does nothing and returns false if the window is already split.
bool wxSplitterWindow::DoSplit(wxSplitMode mode,
                               wxWindow *window1, wxWindow *window2,
                               int sashPosition)
{
    if ( IsSplit() )
        return false;

    wxCHECK_MSG( window1 && window2, false,
                 _T("can not split with NULL window(s)") );

    wxCHECK_MSG( window1->GetParent() == this && window2->GetParent() == this, false,
                  _T("windows in the splitter should have it as parent!") );

    if (! window1->IsShown())
        window1->Show();
    if (! window2->IsShown())
        window2->Show();

    m_splitMode = mode;
    m_windowOne = window1;
    m_windowTwo = window2;

    // remember the sash position we want to set for later if we can't set it
    // right now (e.g. because the window is too small)
    m_requestedSashPosition = sashPosition;
    m_checkRequestedSashPosition = false;

    DoSetSashPosition(ConvertSashPosition(sashPosition));

    SizeWindows();

    return true;
}

int wxSplitterWindow::ConvertSashPosition(int sashPosition) const
{
    if ( sashPosition > 0 )
    {
        return sashPosition;
    }
    else if ( sashPosition < 0 )
    {
        // It's negative so adding is subtracting
        return GetWindowSize() + sashPosition;
    }
    else // sashPosition == 0
    {
        // default, put it in the centre
        return GetWindowSize() / 2;
    }
}

// Remove the specified (or second) window from the view
// Doesn't actually delete the window.
bool wxSplitterWindow::Unsplit(wxWindow *toRemove)
{
    if ( ! IsSplit() )
        return false;

    wxWindow *win;
    if ( toRemove == NULL || toRemove == m_windowTwo)
    {
        win = m_windowTwo ;
        m_windowTwo = (wxWindow *) NULL;
    }
    else if ( toRemove == m_windowOne )
    {
        win = m_windowOne ;
        m_windowOne = m_windowTwo;
        m_windowTwo = (wxWindow *) NULL;
    }
    else
    {
        wxFAIL_MSG(wxT("splitter: attempt to remove a non-existent window"));

        return false;
    }

    OnUnsplit(win);
    DoSetSashPosition(0);
    SizeWindows();

    return true;
}

// Replace a window with another one
bool wxSplitterWindow::ReplaceWindow(wxWindow *winOld, wxWindow *winNew)
{
    wxCHECK_MSG( winOld, false, wxT("use one of Split() functions instead") );
    wxCHECK_MSG( winNew, false, wxT("use Unsplit() functions instead") );

    if ( winOld == m_windowTwo )
    {
        m_windowTwo = winNew;
    }
    else if ( winOld == m_windowOne )
    {
        m_windowOne = winNew;
    }
    else
    {
        wxFAIL_MSG(wxT("splitter: attempt to replace a non-existent window"));

        return false;
    }

    SizeWindows();

    return true;
}

void wxSplitterWindow::SetMinimumPaneSize(int min)
{
    m_minimumPaneSize = min;
    int pos = m_requestedSashPosition != INT_MAX ? m_requestedSashPosition : m_sashPosition;
    SetSashPosition(pos); // re-check limits
}

void wxSplitterWindow::SetSashPosition(int position, bool redraw)
{
    // remember the sash position we want to set for later if we can't set it
    // right now (e.g. because the window is too small)
    m_requestedSashPosition = position;
    m_checkRequestedSashPosition = false;

    DoSetSashPosition(ConvertSashPosition(position));

    if ( redraw )
    {
        SizeWindows();
    }
}

// Make sure the child window sizes are updated. This is useful
// for reducing flicker by updating the sizes before a
// window is shown, if you know the overall size is correct.
void wxSplitterWindow::UpdateSize()
{
    m_checkRequestedSashPosition = true;
    SizeWindows();
    m_checkRequestedSashPosition = false;
}

bool wxSplitterWindow::DoSendEvent(wxSplitterEvent& event)
{
    return !GetEventHandler()->ProcessEvent(event) || event.IsAllowed();
}

wxSize wxSplitterWindow::DoGetBestSize() const
{
    // get best sizes of subwindows
    wxSize size1, size2;
    if ( m_windowOne )
        size1 = m_windowOne->GetEffectiveMinSize();
    if ( m_windowTwo )
        size2 = m_windowTwo->GetEffectiveMinSize();

    // sum them
    //
    // pSash points to the size component to which sash size must be added
    int *pSash;
    wxSize sizeBest;
    if ( m_splitMode == wxSPLIT_VERTICAL )
    {
        sizeBest.y = wxMax(size1.y, size2.y);
        sizeBest.x = wxMax(size1.x, m_minimumPaneSize) +
                        wxMax(size2.x, m_minimumPaneSize);

        pSash = &sizeBest.x;
    }
    else // wxSPLIT_HORIZONTAL
    {
        sizeBest.x = wxMax(size1.x, size2.x);
        sizeBest.y = wxMax(size1.y, m_minimumPaneSize) +
                        wxMax(size2.y, m_minimumPaneSize);

        pSash = &sizeBest.y;
    }

    // account for the sash if the window is actually split
    if ( m_windowOne && m_windowTwo )
        *pSash += GetSashSize();

    // account for the border too
    int border = 2*GetBorderSize();
    sizeBest.x += border;
    sizeBest.y += border;

    return sizeBest;
}

// ---------------------------------------------------------------------------
// wxSplitterWindow virtual functions: they now just generate the events
// ---------------------------------------------------------------------------

bool wxSplitterWindow::OnSashPositionChange(int WXUNUSED(newSashPosition))
{
    // always allow by default
    return true;
}

int wxSplitterWindow::OnSashPositionChanging(int newSashPosition)
{
    // If within UNSPLIT_THRESHOLD from edge, set to edge to cause closure.
    const int UNSPLIT_THRESHOLD = 4;

    // first of all, check if OnSashPositionChange() doesn't forbid this change
    if ( !OnSashPositionChange(newSashPosition) )
    {
        // it does
        return -1;
    }

    // Obtain relevant window dimension for bottom / right threshold check
    int window_size = GetWindowSize();

    bool unsplit_scenario = false;
    if ( m_permitUnsplitAlways || m_minimumPaneSize == 0 )
    {
        // Do edge detection if unsplit premitted
        if ( newSashPosition <= UNSPLIT_THRESHOLD )
        {
            // threshold top / left check
            newSashPosition = 0;
            unsplit_scenario = true;
        }
        if ( newSashPosition >= window_size - UNSPLIT_THRESHOLD )
        {
            // threshold bottom/right check
            newSashPosition = window_size;
            unsplit_scenario = true;
        }
    }

    if ( !unsplit_scenario )
    {
        // If resultant pane would be too small, enlarge it
        newSashPosition = AdjustSashPosition(newSashPosition);
    }

    // If the result is out of bounds it means minimum size is too big,
    // so split window in half as best compromise.
    if ( newSashPosition < 0 || newSashPosition > window_size )
        newSashPosition = window_size / 2;

    // now let the event handler have it
    //
    // FIXME: shouldn't we do it before the adjustments above so as to ensure
    //        that the sash position is always reasonable?
    wxSplitterEvent event(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING, this);
    event.m_data.pos = newSashPosition;

    if ( !DoSendEvent(event) )
    {
        // the event handler vetoed the change
        newSashPosition = -1;
    }
    else
    {
        // it could have been changed by it
        newSashPosition = event.GetSashPosition();
    }

    return newSashPosition;
}

// Called when the sash is double-clicked. The default behaviour is to remove
// the sash if the minimum pane size is zero.
void wxSplitterWindow::OnDoubleClickSash(int x, int y)
{
    wxCHECK_RET(m_windowTwo, wxT("splitter: no window to remove"));

    // new code should handle events instead of using the virtual functions
    wxSplitterEvent event(wxEVT_COMMAND_SPLITTER_DOUBLECLICKED, this);
    event.m_data.pt.x = x;
    event.m_data.pt.y = y;
    if ( DoSendEvent(event) )
    {
        if ( GetMinimumPaneSize() == 0 || m_permitUnsplitAlways )
        {
            wxWindow* win = m_windowTwo;
            if ( Unsplit(win) )
            {
                wxSplitterEvent unsplitEvent(wxEVT_COMMAND_SPLITTER_UNSPLIT, this);
                unsplitEvent.m_data.win = win;
                (void)DoSendEvent(unsplitEvent);
            }
        }
    }
    //else: blocked by user
}

void wxSplitterWindow::OnUnsplit(wxWindow *winRemoved)
{
    // call this before calling the event handler which may delete the window
    winRemoved->Show(false);
}

#if defined( __WXMSW__ ) || defined( __WXMAC__)

// this is currently called (and needed) under MSW only...
void wxSplitterWindow::OnSetCursor(wxSetCursorEvent& event)
{
    // if we don't do it, the resizing cursor might be set for child window:
    // and like this we explicitly say that our cursor should not be used for
    // children windows which overlap us

    if ( SashHitTest(event.GetX(), event.GetY(), 0) )
    {
        // default processing is ok
        event.Skip();
    }
    //else: do nothing, in particular, don't call Skip()
}

#endif // wxMSW || wxMac

#endif // wxUSE_SPLITTER

