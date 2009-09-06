///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/apptbase.h
// Purpose:     declaration of wxAppTraits for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.06.2003
// RCS-ID:      $Id: apptbase.h 40599 2006-08-13 21:00:32Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_APPTBASE_H_
#define _WX_MSW_APPTBASE_H_

// ----------------------------------------------------------------------------
// wxAppTraits: the MSW version adds extra hooks needed by MSW-only code
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxAppTraits : public wxAppTraitsBase
{
public:
    // wxExecute() support methods
    // ---------------------------

    // called before starting to wait for the child termination, may return
    // some opaque data which will be passed later to AfterChildWaitLoop()
    virtual void *BeforeChildWaitLoop() = 0;

    // process pending Windows messages, even in console app
    virtual void AlwaysYield() = 0;

    // called after starting to wait for the child termination, the parameter
    // is the return value of BeforeChildWaitLoop()
    virtual void AfterChildWaitLoop(void *data) = 0;


    // wxThread helpers
    // ----------------

    // process a message while waiting for a(nother) thread, should return
    // false if and only if we have to exit the application
    virtual bool DoMessageFromThreadWait() = 0;
};

#endif // _WX_MSW_APPTBASE_H_

