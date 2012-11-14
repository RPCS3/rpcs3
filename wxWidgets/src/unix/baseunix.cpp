///////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/baseunix.cpp
// Purpose:     misc stuff only used in console applications under Unix
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.06.2003
// RCS-ID:      $Id: baseunix.cpp 40599 2006-08-13 21:00:32Z VZ $
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
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/utils.h"
#endif //WX_PRECOMP

#include "wx/apptrait.h"
#include "wx/unix/execute.h"

// for waitpid()
#include <sys/types.h>
#include <sys/wait.h>

// ============================================================================
// wxConsoleAppTraits implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxExecute support
// ----------------------------------------------------------------------------

bool wxConsoleAppTraits::CreateEndProcessPipe(wxExecuteData& WXUNUSED(data))
{
    // nothing to do, so always ok
    return true;
}

bool
wxConsoleAppTraits::IsWriteFDOfEndProcessPipe(wxExecuteData& WXUNUSED(data),
                                              int WXUNUSED(fd))
{
    // we don't have any pipe
    return false;
}

void
wxConsoleAppTraits::DetachWriteFDOfEndProcessPipe(wxExecuteData& WXUNUSED(data))
{
    // nothing to do
}


int
wxConsoleAppTraits::WaitForChild(wxExecuteData& execData)
{
    wxASSERT_MSG( execData.flags & wxEXEC_SYNC,
                  wxT("async execution not supported yet") );

    int exitcode = 0;
    if ( waitpid(execData.pid, &exitcode, 0) == -1 || !WIFEXITED(exitcode) )
    {
        wxLogSysError(_("Waiting for subprocess termination failed"));
    }

    return exitcode;
}

