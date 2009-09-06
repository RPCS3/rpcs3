/////////////////////////////////////////////////////////////////////////////
// Name:        common/timercmn.cpp
// Purpose:     wxTimerBase implementation
// Author:      Julian Smart, Guillermo Rodriguez, Vadim Zeitlin
// Modified by: VZ: extracted all non-wxTimer stuff in stopwatch.cpp (20.06.03)
// Created:     04/01/98
// RCS-ID:      $Id: timercmn.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Julian Smart
//              (c) 1999 Guillermo Rodriguez <guille@iies.es>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// wxWin headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TIMER

#ifndef WX_PRECOMP
    #include "wx/timer.h"
#endif

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTimerEvent, wxEvent)

// ============================================================================
// wxTimerBase implementation
// ============================================================================

wxTimerBase::~wxTimerBase()
{
    // this destructor is required for Darwin
}

void wxTimerBase::Notify()
{
    // the base class version generates an event if it has owner - which it
    // should because otherwise nobody can process timer events
    wxCHECK_RET( m_owner, _T("wxTimer::Notify() should be overridden.") );

    wxTimerEvent event(m_idTimer, m_milli);
    event.SetEventObject(this);
    (void)m_owner->ProcessEvent(event);
}

bool wxTimerBase::Start(int milliseconds, bool oneShot)
{
    // under MSW timers only work when they're started from the main thread so
    // let the caller know about it
#if wxUSE_THREADS
    wxASSERT_MSG( wxThread::IsMain(),
                  _T("timer can only be started from the main thread") );
#endif // wxUSE_THREADS

    if ( IsRunning() )
    {
        // not stopping the already running timer might work for some
        // platforms (no problems under MSW) but leads to mysterious crashes
        // on the others (GTK), so to be on the safe side do it here
        Stop();
    }

    if ( milliseconds != -1 )
    {
        m_milli = milliseconds;
    }

    m_oneShot = oneShot;

    return true;
}

#endif // wxUSE_TIMER

