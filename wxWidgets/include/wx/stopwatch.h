/////////////////////////////////////////////////////////////////////////////
// Name:        wx/stopwatch.h
// Purpose:     wxStopWatch and global time-related functions
// Author:      Julian Smart (wxTimer), Sylvain Bougnoux (wxStopWatch)
// Created:     26.06.03 (extracted from wx/timer.h)
// RCS-ID:      $Id: stopwatch.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998-2003 Julian Smart, Sylvain Bougnoux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STOPWATCH_H_
#define _WX_STOPWATCH_H_

#include "wx/defs.h"
#include "wx/longlong.h"

// ----------------------------------------------------------------------------
// wxStopWatch: measure time intervals with up to 1ms resolution
// ----------------------------------------------------------------------------

#if wxUSE_STOPWATCH

class WXDLLIMPEXP_BASE wxStopWatch
{
public:
    // ctor starts the stop watch
    wxStopWatch() { m_pauseCount = 0; Start(); }

    // start the stop watch at the moment t0
    void Start(long t0 = 0);

    // pause the stop watch
    void Pause()
    {
        if ( m_pauseCount++ == 0 )
            m_pause = GetElapsedTime();
    }

    // resume it
    void Resume()
    {
        wxASSERT_MSG( m_pauseCount > 0,
                      wxT("Resuming stop watch which is not paused") );

        if ( --m_pauseCount == 0 )
            Start(m_pause);
    }

    // get elapsed time since the last Start() in milliseconds
    long Time() const;

protected:
    // returns the elapsed time since t0
    long GetElapsedTime() const;

private:
    // the time of the last Start()
    wxLongLong m_t0;

    // the time of the last Pause() (only valid if m_pauseCount > 0)
    long m_pause;

    // if > 0, the stop watch is paused, otherwise it is running
    int m_pauseCount;
};

#endif // wxUSE_STOPWATCH

#if wxUSE_LONGLONG && WXWIN_COMPATIBILITY_2_6

    // Starts a global timer
    // -- DEPRECATED: use wxStopWatch instead
    wxDEPRECATED( void WXDLLIMPEXP_BASE wxStartTimer() );

    // Gets elapsed milliseconds since last wxStartTimer or wxGetElapsedTime
    // -- DEPRECATED: use wxStopWatch instead
    wxDEPRECATED( long WXDLLIMPEXP_BASE wxGetElapsedTime(bool resetTimer = true) );

#endif // wxUSE_LONGLONG && WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// global time functions
// ----------------------------------------------------------------------------

// Get number of seconds since local time 00:00:00 Jan 1st 1970.
extern long WXDLLIMPEXP_BASE wxGetLocalTime();

// Get number of seconds since GMT 00:00:00, Jan 1st 1970.
extern long WXDLLIMPEXP_BASE wxGetUTCTime();

#if wxUSE_LONGLONG
    typedef wxLongLong wxMilliClock_t;
#else
    typedef double wxMilliClock_t;
#endif // wxUSE_LONGLONG

// Get number of milliseconds since local time 00:00:00 Jan 1st 1970
extern wxMilliClock_t WXDLLIMPEXP_BASE wxGetLocalTimeMillis();

#define wxGetCurrentTime() wxGetLocalTime()

#endif // _WX_STOPWATCH_H_
