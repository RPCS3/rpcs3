///////////////////////////////////////////////////////////////////////////////
// Name:        wx/power.h
// Purpose:     functions and classes for system power management
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-05-27
// RCS-ID:      $Id: power.h 48811 2007-09-19 23:11:28Z RD $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_POWER_H_
#define _WX_POWER_H_

#include "wx/event.h"

// ----------------------------------------------------------------------------
// power management constants
// ----------------------------------------------------------------------------

enum wxPowerType
{
    wxPOWER_SOCKET,
    wxPOWER_BATTERY,
    wxPOWER_UNKNOWN
};

enum wxBatteryState
{
    wxBATTERY_NORMAL_STATE,    // system is fully usable
    wxBATTERY_LOW_STATE,       // start to worry
    wxBATTERY_CRITICAL_STATE,  // save quickly
    wxBATTERY_SHUTDOWN_STATE,  // too late
    wxBATTERY_UNKNOWN_STATE
};

// ----------------------------------------------------------------------------
// wxPowerEvent is generated when the system online status changes
// ----------------------------------------------------------------------------

// currently the power events are only available under Windows, to avoid
// compiling in the code for handling them which is never going to be invoked
// under the other platforms, we define wxHAS_POWER_EVENTS symbol if this event
// is available, it should be used to guard all code using wxPowerEvent
#ifdef __WXMSW__

#define wxHAS_POWER_EVENTS

class WXDLLIMPEXP_BASE wxPowerEvent : public wxEvent
{
public:
    wxPowerEvent(wxEventType evtType) : wxEvent(wxID_NONE, evtType)
    {
        m_veto = false;
    }

    // Veto the operation (only makes sense with EVT_POWER_SUSPENDING)
    void Veto() { m_veto = true; }

    bool IsVetoed() const { return m_veto; }


    // default copy ctor, assignment operator and dtor are ok

    virtual wxEvent *Clone() const { return new wxPowerEvent(*this); }

private:
    bool m_veto;

#if wxABI_VERSION >= 20806
    DECLARE_ABSTRACT_CLASS(wxPowerEvent)
#endif
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_BASE, wxEVT_POWER_SUSPENDING, 406)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_BASE, wxEVT_POWER_SUSPENDED, 407)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_BASE, wxEVT_POWER_SUSPEND_CANCEL, 408)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_BASE, wxEVT_POWER_RESUME, 444)
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxPowerEventFunction)(wxPowerEvent&);

#define wxPowerEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction) \
        wxStaticCastEvent(wxPowerEventFunction, &func)

#define EVT_POWER_SUSPENDING(func) \
    wx__DECLARE_EVT0(wxEVT_POWER_SUSPENDING, wxPowerEventHandler(func))
#define EVT_POWER_SUSPENDED(func) \
    wx__DECLARE_EVT0(wxEVT_POWER_SUSPENDED, wxPowerEventHandler(func))
#define EVT_POWER_SUSPEND_CANCEL(func) \
    wx__DECLARE_EVT0(wxEVT_POWER_SUSPEND_CANCEL, wxPowerEventHandler(func))
#define EVT_POWER_RESUME(func) \
    wx__DECLARE_EVT0(wxEVT_POWER_RESUME, wxPowerEventHandler(func))

#else // no support for power events
    #undef wxHAS_POWER_EVENTS
#endif // support for power events/no support

// ----------------------------------------------------------------------------
// power management functions
// ----------------------------------------------------------------------------

// return the current system power state: online or offline
WXDLLIMPEXP_BASE wxPowerType wxGetPowerType();

// return approximate battery state
WXDLLIMPEXP_BASE wxBatteryState wxGetBatteryState();

#endif // _WX_POWER_H_
