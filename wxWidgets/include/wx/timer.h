/////////////////////////////////////////////////////////////////////////////
// Name:        wx/timer.h
// Purpose:     wxTimer, wxStopWatch and global time-related functions
// Author:      Julian Smart
// Modified by: Vadim Zeitlin (wxTimerBase)
//              Guillermo Rodriguez (global clean up)
// Created:     04/01/98
// RCS-ID:      $Id: timer.h 44244 2007-01-18 17:00:52Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TIMER_H_BASE_
#define _WX_TIMER_H_BASE_

#include "wx/defs.h"

#if wxUSE_GUI && wxUSE_TIMER

#include "wx/object.h"
#include "wx/longlong.h"
#include "wx/event.h"
#include "wx/stopwatch.h" // for backwards compatibility
#include "wx/window.h"    // only for NewControlId()


// more readable flags for Start():

// generate notifications periodically until the timer is stopped (default)
#define wxTIMER_CONTINUOUS false

// only send the notification once and then stop the timer
#define wxTIMER_ONE_SHOT true

// the interface of wxTimer class
class WXDLLEXPORT wxTimerBase : public wxEvtHandler
{
public:
    // ctors and initializers
    // ----------------------

    // default: if you don't call SetOwner(), your only chance to get timer
    // notifications is to override Notify() in the derived class
    wxTimerBase()
        { Init(); SetOwner(this); }

    // ctor which allows to avoid having to override Notify() in the derived
    // class: the owner will get timer notifications which can be handled with
    // EVT_TIMER
    wxTimerBase(wxEvtHandler *owner, int timerid = wxID_ANY)
        { Init(); SetOwner(owner, timerid); }

    // same as ctor above
    void SetOwner(wxEvtHandler *owner, int timerid = wxID_ANY)
    {
        m_owner = owner;
        m_idTimer = timerid == wxID_ANY ? wxWindow::NewControlId() : timerid;
    }

    wxEvtHandler *GetOwner() const { return m_owner; }

    virtual ~wxTimerBase();

    // working with the timer
    // ----------------------

    // start the timer: if milliseconds == -1, use the same value as for the
    // last Start()
    //
    // it is now valid to call Start() multiple times: this just restarts the
    // timer if it is already running
    virtual bool Start(int milliseconds = -1, bool oneShot = false);

    // stop the timer
    virtual void Stop() = 0;

    // override this in your wxTimer-derived class if you want to process timer
    // messages in it, use non default ctor or SetOwner() otherwise
    virtual void Notify();

    // getting info
    // ------------

    // return true if the timer is running
    virtual bool IsRunning() const = 0;

    // return the timer ID
    int GetId() const { return m_idTimer; }

    // get the (last) timer interval in milliseconds
    int GetInterval() const { return m_milli; }

    // return true if the timer is one shot
    bool IsOneShot() const { return m_oneShot; }

protected:
    // common part of all ctors
    void Init()
        { m_owner = NULL; m_idTimer = wxID_ANY; m_milli = 0; m_oneShot = false; }

    wxEvtHandler *m_owner;
    int     m_idTimer;
    int     m_milli;        // the timer interval
    bool    m_oneShot;      // true if one shot

    DECLARE_NO_COPY_CLASS(wxTimerBase)
};

// ----------------------------------------------------------------------------
// wxTimer itself
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #include "wx/msw/timer.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/timer.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/timer.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/timer.h"
#elif defined(__WXX11__) || defined(__WXMGL__) || defined(__WXDFB__)
    #include "wx/generic/timer.h"
#elif defined (__WXCOCOA__)
    #include "wx/cocoa/timer.h"
#elif defined(__WXMAC__)
    #include "wx/mac/timer.h"
#elif defined(__WXPM__)
    #include "wx/os2/timer.h"
#endif

// ----------------------------------------------------------------------------
// wxTimerRunner: starts the timer in its ctor, stops in the dtor
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTimerRunner
{
public:
    wxTimerRunner(wxTimer& timer) : m_timer(timer) { }
    wxTimerRunner(wxTimer& timer, int milli, bool oneShot = false)
        : m_timer(timer)
    {
        m_timer.Start(milli, oneShot);
    }

    void Start(int milli, bool oneShot = false)
    {
        m_timer.Start(milli, oneShot);
    }

    ~wxTimerRunner()
    {
        if ( m_timer.IsRunning() )
        {
            m_timer.Stop();
        }
    }

private:
    wxTimer& m_timer;

    DECLARE_NO_COPY_CLASS(wxTimerRunner)
};

// ----------------------------------------------------------------------------
// wxTimerEvent
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTimerEvent : public wxEvent
{
public:
    wxTimerEvent(int timerid = 0, int interval = 0) : wxEvent(timerid)
    {
        m_eventType = wxEVT_TIMER;

        m_interval = interval;
    }

    // accessors
    int GetInterval() const { return m_interval; }

    // implement the base class pure virtual
    virtual wxEvent *Clone() const { return new wxTimerEvent(*this); }

private:
    int m_interval;

    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxTimerEvent)
};

typedef void (wxEvtHandler::*wxTimerEventFunction)(wxTimerEvent&);

#define wxTimerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxTimerEventFunction, &func)

#define EVT_TIMER(timerid, func) \
    wx__DECLARE_EVT1(wxEVT_TIMER, timerid, wxTimerEventHandler(func))

#endif // wxUSE_GUI && wxUSE_TIMER

#endif
    // _WX_TIMER_H_BASE_
