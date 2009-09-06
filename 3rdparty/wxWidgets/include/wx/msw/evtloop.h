///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/evtloop.h
// Purpose:     wxEventLoop class for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-31
// RCS-ID:      $Id: evtloop.h 36881 2006-01-15 10:13:40Z ABX $
// Copyright:   (c) 2003-2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_EVTLOOP_H_
#define _WX_MSW_EVTLOOP_H_

#include "wx/window.h"

// ----------------------------------------------------------------------------
// wxEventLoop
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxEventLoop : public wxEventLoopManual
{
public:
    wxEventLoop();

    // implement base class pure virtuals
    virtual bool Pending() const;
    virtual bool Dispatch();

    // MSW-specific methods
    // --------------------

    // preprocess a message, return true if processed (i.e. no further
    // dispatching required)
    virtual bool PreProcessMessage(WXMSG *msg);

    // process a single message
    virtual void ProcessMessage(WXMSG *msg);

    // set the critical window: this is the window such that all the events
    // except those to this window (and its children) stop to be processed
    // (typical examples: assert or crash report dialog)
    //
    // calling this function with NULL argument restores the normal event
    // handling
    static void SetCriticalWindow(wxWindowMSW *win) { ms_winCritical = win; }

    // return true if there is no critical window or if this window is [a child
    // of] the critical one
    static bool AllowProcessing(wxWindowMSW *win)
    {
        return !ms_winCritical || IsChildOfCriticalWindow(win);
    }

protected:
    // override/implement base class virtuals
    virtual void WakeUp();
    virtual void OnNextIteration();

    // check if the given window is a child of ms_winCritical (which must be
    // non NULL)
    static bool IsChildOfCriticalWindow(wxWindowMSW *win);


    // critical window or NULL
    static wxWindowMSW *ms_winCritical;
};

#endif // _WX_MSW_EVTLOOP_H_
