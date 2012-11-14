///////////////////////////////////////////////////////////////////////////////
// Name:        msw/basemsw.cpp
// Purpose:     misc stuff only used in console applications under MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.06.2003
// RCS-ID:      $Id: basemsw.cpp 39831 2006-06-25 23:53:53Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
// License:     wxWindows licence
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

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#include "wx/apptrait.h"
// MBN: this is a workaround for MSVC 5: if it is not #included in
// some wxBase file, wxRecursionGuard methods won't be exported from
// wxBase.dll, and MSVC 5 will give linker errors
#include "wx/recguard.h"

#include "wx/msw/private.h"

// ============================================================================
// wxConsoleAppTraits implementation
// ============================================================================

void wxConsoleAppTraits::AlwaysYield()
{
    // we need to use special logic to deal with WM_PAINT: as this pseudo
    // message is generated automatically as long as there are invalidated
    // windows belonging to this thread, we'd never return if we waited here
    // until we have no more of them left. OTOH, this message is always the
    // last one in the queue, so we can safely return as soon as we detect it
    MSG msg;
    while ( ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        if ( msg.message == WM_PAINT )
            break;
    }
}

void *wxConsoleAppTraits::BeforeChildWaitLoop()
{
    // nothing to do here
    return NULL;
}

void wxConsoleAppTraits::AfterChildWaitLoop(void * WXUNUSED(data))
{
    // nothing to do here
}

bool wxConsoleAppTraits::DoMessageFromThreadWait()
{
    // nothing to process here
    return true;
}

