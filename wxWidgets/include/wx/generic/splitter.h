/////////////////////////////////////////////////////////////////////////////
// Name:        wx/splitter.h
// Purpose:     wxSplitterWindow class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: splitter.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_SPLITTER_H_
#define _WX_GENERIC_SPLITTER_H_

#include "wx/window.h"                      // base class declaration
#include "wx/containr.h"                    // wxControlContainer

class WXDLLIMPEXP_FWD_CORE wxSplitterEvent;

// ---------------------------------------------------------------------------
// splitter constants
// ---------------------------------------------------------------------------

enum wxSplitMode
{
    wxSPLIT_HORIZONTAL = 1,
    wxSPLIT_VERTICAL
};

enum
{
    wxSPLIT_DRAG_NONE,
    wxSPLIT_DRAG_DRAGGING,
    wxSPLIT_DRAG_LEFT_DOWN
};

// ---------------------------------------------------------------------------
// wxSplitterWindow maintains one or two panes, with
// an optional vertical or horizontal split which
// can be used with the mouse or programmatically.
// ---------------------------------------------------------------------------

// TODO:
// 1) Perhaps make the borders sensitive to dragging in order to create a split.
//    The MFC splitter window manages scrollbars as well so is able to
//    put sash buttons on the scrollbars, but we probably don't want to go down
//    this path.
// 2) for wxWidgets 2.0, we must find a way to set the WS_CLIPCHILDREN style
//    to prevent flickering. (WS_CLIPCHILDREN doesn't work in all cases so can't be
//    standard).

class WXDLLEXPORT wxSplitterWindow: public wxWindow
{
public:

////////////////////////////////////////////////////////////////////////////
// Public API

    // Default constructor
    wxSplitterWindow()
    {
        Init();
    }

    // Normal constructor
    wxSplitterWindow(wxWindow *parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxSP_3D,
                     const wxString& name = wxT("splitter"))
    {
        Init();
        Create(parent, id, pos, size, style, name);
    }

    virtual ~wxSplitterWindow();

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxSP_3D,
                     const wxString& name = wxT("splitter"));

    // Gets the only or left/top pane
    wxWindow *GetWindow1() const { return m_windowOne; }

    // Gets the right/bottom pane
    wxWindow *GetWindow2() const { return m_windowTwo; }

    // Sets the split mode
    void SetSplitMode(int mode)
    {
        wxASSERT_MSG( mode == wxSPLIT_VERTICAL || mode == wxSPLIT_HORIZONTAL,
                      wxT("invalid split mode") );

        m_splitMode = (wxSplitMode)mode;
    }

    // Gets the split mode
    wxSplitMode GetSplitMode() const { return m_splitMode; }

    // Initialize with one window
    void Initialize(wxWindow *window);

    // Associates the given window with window 2, drawing the appropriate sash
    // and changing the split mode.
    // Does nothing and returns false if the window is already split.
    // A sashPosition of 0 means choose a default sash position,
    // negative sashPosition specifies the size of right/lower pane as it's
    // absolute value rather than the size of left/upper pane.
    virtual bool SplitVertically(wxWindow *window1,
                                 wxWindow *window2,
                                 int sashPosition = 0)
        { return DoSplit(wxSPLIT_VERTICAL, window1, window2, sashPosition); }
    virtual bool SplitHorizontally(wxWindow *window1,
                                   wxWindow *window2,
                                   int sashPosition = 0)
        { return DoSplit(wxSPLIT_HORIZONTAL, window1, window2, sashPosition); }

    // Removes the specified (or second) window from the view
    // Doesn't actually delete the window.
    bool Unsplit(wxWindow *toRemove = (wxWindow *) NULL);

    // Replaces one of the windows with another one (neither old nor new
    // parameter should be NULL)
    bool ReplaceWindow(wxWindow *winOld, wxWindow *winNew);

    // Make sure the child window sizes are updated. This is useful
    // for reducing flicker by updating the sizes before a
    // window is shown, if you know the overall size is correct.
    void UpdateSize();

    // Is the window split?
    bool IsSplit() const { return (m_windowTwo != NULL); }

    // Sets the sash size
    void SetSashSize(int width) { m_sashSize = width; }

    // Sets the border size
    void SetBorderSize(int WXUNUSED(width)) { }

    // Gets the sash size
    int GetSashSize() const;

    // Gets the border size
    int GetBorderSize() const;

    // Set the sash position
    void SetSashPosition(int position, bool redraw = true);

    // Gets the sash position
    int GetSashPosition() const { return m_sashPosition; }

    // Set the sash gravity
    void SetSashGravity(double gravity);

    // Gets the sash gravity
    double GetSashGravity() const { return m_sashGravity; }

    // If this is zero, we can remove panes by dragging the sash.
    void SetMinimumPaneSize(int min);
    int GetMinimumPaneSize() const { return m_minimumPaneSize; }

    // NB: the OnXXX() functions below are for backwards compatibility only,
    //     don't use them in new code but handle the events instead!

    // called when the sash position is about to change, may return a new value
    // for the sash or -1 to prevent the change from happening at all
    virtual int OnSashPositionChanging(int newSashPosition);

    // Called when the sash position is about to be changed, return
    // false from here to prevent the change from taking place.
    // Repositions sash to minimum position if pane would be too small.
    // newSashPosition here is always positive or zero.
    virtual bool OnSashPositionChange(int newSashPosition);

    // If the sash is moved to an extreme position, a subwindow
    // is removed from the splitter window, and the app is
    // notified. The app should delete or hide the window.
    virtual void OnUnsplit(wxWindow *removed);

    // Called when the sash is double-clicked.
    // The default behaviour is to remove the sash if the
    // minimum pane size is zero.
    virtual void OnDoubleClickSash(int x, int y);

////////////////////////////////////////////////////////////////////////////
// Implementation

    // Paints the border and sash
    void OnPaint(wxPaintEvent& event);

    // Handles mouse events
    void OnMouseEvent(wxMouseEvent& ev);

    // Adjusts the panes
    void OnSize(wxSizeEvent& event);

    // In live mode, resize child windows in idle time
    void OnInternalIdle();

    // Draws the sash
    virtual void DrawSash(wxDC& dc);

    // Draws the sash tracker (for whilst moving the sash)
    virtual void DrawSashTracker(int x, int y);

    // Tests for x, y over sash
    virtual bool SashHitTest(int x, int y, int tolerance = 5);

    // Resizes subwindows
    virtual void SizeWindows();

    void SetNeedUpdating(bool needUpdating) { m_needUpdating = needUpdating; }
    bool GetNeedUpdating() const { return m_needUpdating ; }

#ifdef __WXMAC__
    virtual bool MacClipGrandChildren() const { return true ; }
#endif

protected:
    // event handlers
#if defined(__WXMSW__) || defined(__WXMAC__)
    void OnSetCursor(wxSetCursorEvent& event);
#endif // wxMSW

    // send the given event, return false if the event was processed and vetoed
    // by the user code
    bool DoSendEvent(wxSplitterEvent& event);

    // common part of all ctors
    void Init();

    // common part of SplitVertically() and SplitHorizontally()
    bool DoSplit(wxSplitMode mode,
                 wxWindow *window1, wxWindow *window2,
                 int sashPosition);

    // adjusts sash position with respect to min. pane and window sizes
    int AdjustSashPosition(int sashPos) const;

    // get either width or height depending on the split mode
    int GetWindowSize() const;

    // convert the user specified sash position which may be > 0 (as is), < 0
    // (specifying the size of the right pane) or 0 (use default) to the real
    // position to be passed to DoSetSashPosition()
    int ConvertSashPosition(int sashPos) const;

    // set the real sash position, sashPos here must be positive
    //
    // returns true if the sash position has been changed, false otherwise
    bool DoSetSashPosition(int sashPos);

    // set the sash position and send an event about it having been changed
    void SetSashPositionAndNotify(int sashPos);

    // callbacks executed when we detect that the mouse has entered or left
    // the sash
    virtual void OnEnterSash();
    virtual void OnLeaveSash();

    // set the cursor appropriate for the current split mode
    void SetResizeCursor();

    // redraw the splitter if its "hotness" changed if necessary
    void RedrawIfHotSensitive(bool isHot);

    // return the best size of the splitter equal to best sizes of its
    // subwindows
    virtual wxSize DoGetBestSize() const;


    wxSplitMode m_splitMode;
    wxWindow*   m_windowOne;
    wxWindow*   m_windowTwo;
    int         m_dragMode;
    int         m_oldX;
    int         m_oldY;
    int         m_sashPosition; // Number of pixels from left or top
    double      m_sashGravity;
    int         m_sashSize;
    wxSize      m_lastSize;
    int         m_requestedSashPosition;
    int         m_sashPositionCurrent; // while dragging
    int         m_firstX;
    int         m_firstY;
    int         m_minimumPaneSize;
    wxCursor    m_sashCursorWE;
    wxCursor    m_sashCursorNS;
    wxPen      *m_sashTrackerPen;

    // when in live mode, set this to true to resize children in idle
    bool        m_needUpdating:1;
    bool        m_permitUnsplitAlways:1;
    bool        m_isHot:1;
    bool        m_checkRequestedSashPosition:1;

private:
    WX_DECLARE_CONTROL_CONTAINER();

    DECLARE_DYNAMIC_CLASS(wxSplitterWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxSplitterWindow)
};

// ----------------------------------------------------------------------------
// event class and macros
// ----------------------------------------------------------------------------

// we reuse the same class for all splitter event types because this is the
// usual wxWin convention, but the three event types have different kind of
// data associated with them, so the accessors can be only used if the real
// event type matches with the one for which the accessors make sense
class WXDLLEXPORT wxSplitterEvent : public wxNotifyEvent
{
public:
    wxSplitterEvent(wxEventType type = wxEVT_NULL,
                    wxSplitterWindow *splitter = (wxSplitterWindow *)NULL)
        : wxNotifyEvent(type)
    {
        SetEventObject(splitter);
        if (splitter) m_id = splitter->GetId();
    }

    // SASH_POS_CHANGED methods

    // setting the sash position to -1 prevents the change from taking place at
    // all
    void SetSashPosition(int pos)
    {
        wxASSERT( GetEventType() == wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED
                || GetEventType() == wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING);

        m_data.pos = pos;
    }

    int GetSashPosition() const
    {
        wxASSERT( GetEventType() == wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED
                || GetEventType() == wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING);

        return m_data.pos;
    }

    // UNSPLIT event methods
    wxWindow *GetWindowBeingRemoved() const
    {
        wxASSERT( GetEventType() == wxEVT_COMMAND_SPLITTER_UNSPLIT );

        return m_data.win;
    }

    // DCLICK event methods
    int GetX() const
    {
        wxASSERT( GetEventType() == wxEVT_COMMAND_SPLITTER_DOUBLECLICKED );

        return m_data.pt.x;
    }

    int GetY() const
    {
        wxASSERT( GetEventType() == wxEVT_COMMAND_SPLITTER_DOUBLECLICKED );

        return m_data.pt.y;
    }

private:
    friend class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;

    // data for the different types of event
    union
    {
        int pos;            // position for SASH_POS_CHANGED event
        wxWindow *win;      // window being removed for UNSPLIT event
        struct
        {
            int x, y;
        } pt;               // position of double click for DCLICK event
    } m_data;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxSplitterEvent)
};

typedef void (wxEvtHandler::*wxSplitterEventFunction)(wxSplitterEvent&);

#define wxSplitterEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxSplitterEventFunction, &func)

#define wx__DECLARE_SPLITTEREVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_SPLITTER_ ## evt, id, wxSplitterEventHandler(fn))

#define EVT_SPLITTER_SASH_POS_CHANGED(id, fn) \
    wx__DECLARE_SPLITTEREVT(SASH_POS_CHANGED, id, fn)

#define EVT_SPLITTER_SASH_POS_CHANGING(id, fn) \
    wx__DECLARE_SPLITTEREVT(SASH_POS_CHANGING, id, fn)

#define EVT_SPLITTER_DCLICK(id, fn) \
    wx__DECLARE_SPLITTEREVT(DOUBLECLICKED, id, fn)

#define EVT_SPLITTER_UNSPLIT(id, fn) \
    wx__DECLARE_SPLITTEREVT(UNSPLIT, id, fn)

#endif // _WX_GENERIC_SPLITTER_H_
