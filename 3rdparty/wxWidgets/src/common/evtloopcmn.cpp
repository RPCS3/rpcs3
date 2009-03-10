///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/evtloopcmn.cpp
// Purpose:     common wxEventLoop-related stuff
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-01-12
// RCS-ID:      $Id: evtloopcmn.cpp 45938 2007-05-10 02:07:41Z VZ $
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

#include "wx/evtloop.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif //WX_PRECOMP

// see the comment near the declaration of wxRunningEventLoopCount in
// src/msw/thread.cpp for the explanation of this hack
#if defined(__WXMSW__) && wxUSE_THREADS

extern WXDLLIMPEXP_DATA_BASE(int) wxRunningEventLoopCount;
struct wxRunningEventLoopCounter
{
    wxRunningEventLoopCounter() { wxRunningEventLoopCount++; }
    ~wxRunningEventLoopCounter() { wxRunningEventLoopCount--; }
};

#endif // __WXMSW__

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

wxEventLoop *wxEventLoopBase::ms_activeLoop = NULL;

// wxEventLoopManual is unused in the other ports
#if defined(__WXMSW__) || defined(__WXMAC__) || defined(__WXDFB__)

// ============================================================================
// wxEventLoopManual implementation
// ============================================================================

wxEventLoopManual::wxEventLoopManual()
{
    m_exitcode = 0;
    m_shouldExit = false;
}

int wxEventLoopManual::Run()
{
    // event loops are not recursive, you need to create another loop!
    wxCHECK_MSG( !IsRunning(), -1, _T("can't reenter a message loop") );

    // ProcessIdle() and Dispatch() below may throw so the code here should
    // be exception-safe, hence we must use local objects for all actions we
    // should undo
    wxEventLoopActivator activate(wx_static_cast(wxEventLoop *, this));

#if defined(__WXMSW__) && wxUSE_THREADS
    wxRunningEventLoopCounter evtLoopCounter;
#endif // __WXMSW__

    // we must ensure that OnExit() is called even if an exception is thrown
    // from inside Dispatch() but we must call it from Exit() in normal
    // situations because it is supposed to be called synchronously,
    // wxModalEventLoop depends on this (so we can't just use ON_BLOCK_EXIT or
    // something similar here)
#if wxUSE_EXCEPTIONS
    for ( ;; )
    {
        try
        {
#endif // wxUSE_EXCEPTIONS

            // this is the event loop itself
            for ( ;; )
            {
                // give them the possibility to do whatever they want
                OnNextIteration();

                // generate and process idle events for as long as we don't
                // have anything else to do
                while ( !Pending() && (wxTheApp && wxTheApp->ProcessIdle()) )
                    ;

                // if the "should exit" flag is set, the loop should terminate
                // but not before processing any remaining messages so while
                // Pending() returns true, do process them
                if ( m_shouldExit )
                {
                    while ( Pending() )
                        Dispatch();

                    break;
                }

                // a message came or no more idle processing to do, sit in
                // Dispatch() waiting for the next message
                if ( !Dispatch() )
                {
                    // we got WM_QUIT
                    break;
                }
            }

#if wxUSE_EXCEPTIONS
            // exit the outer loop as well
            break;
        }
        catch ( ... )
        {
            try
            {
                if ( !wxTheApp || !wxTheApp->OnExceptionInMainLoop() )
                {
                    OnExit();
                    break;
                }
                //else: continue running the event loop
            }
            catch ( ... )
            {
                // OnException() throwed, possibly rethrowing the same
                // exception again: very good, but we still need OnExit() to
                // be called
                OnExit();
                throw;
            }
        }
    }
#endif // wxUSE_EXCEPTIONS

    return m_exitcode;
}

void wxEventLoopManual::Exit(int rc)
{
    wxCHECK_RET( IsRunning(), _T("can't call Exit() if not running") );

    m_exitcode = rc;
    m_shouldExit = true;

    OnExit();

    // all we have to do to exit from the loop is to (maybe) wake it up so that
    // it can notice that Exit() had been called
    //
    // in particular, do *not* use here calls such as PostQuitMessage() (under
    // MSW) which terminate the current event loop here because we're not sure
    // that it is going to be processed by the correct event loop: it would be
    // possible that another one is started and terminated by mistake if we do
    // this
    WakeUp();
}

#endif // __WXMSW__ || __WXMAC__ || __WXDFB__
