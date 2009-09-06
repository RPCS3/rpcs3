/////////////////////////////////////////////////////////////////////////////
// Name:        unix/execute.h
// Purpose:     private details of wxExecute() implementation
// Author:      Vadim Zeitlin
// Id:          $Id: execute.h 35055 2005-08-02 22:58:06Z MW $
// Copyright:   (c) 1998 Robert Roebling, Julian Smart, Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_EXECUTE_H
#define _WX_UNIX_EXECUTE_H

#include "wx/unix/pipe.h"

class WXDLLIMPEXP_BASE wxProcess;
class wxStreamTempInputBuffer;

// if pid > 0, the execution is async and the data is freed in the callback
// executed when the process terminates, if pid < 0, the execution is
// synchronous and the caller (wxExecute) frees the data
struct wxEndProcessData
{
    int pid,                // pid of the process
        tag;                // port dependent value
    wxProcess *process;     // if !NULL: notified on process termination
    int  exitcode;          // the exit code
};

// struct in which information is passed from wxExecute() to wxAppTraits
// methods
struct wxExecuteData
{
    wxExecuteData()
    {
        flags =
        pid = 0;

        process = NULL;

#if wxUSE_STREAMS
        bufOut =
        bufErr = NULL;
#endif // wxUSE_STREAMS
    }

    // wxExecute() flags
    int flags;

    // the pid of the child process
    int pid;

    // the associated process object or NULL
    wxProcess *process;

    // pipe used for end process detection
    wxPipe pipeEndProcDetect;

#if wxUSE_STREAMS
    // the input buffer bufOut is connected to stdout, this is why it is
    // called bufOut and not bufIn
    wxStreamTempInputBuffer *bufOut,
                            *bufErr;
#endif // wxUSE_STREAMS
};

// this function is called when the process terminates from port specific
// callback function and is common to all ports (src/unix/utilsunx.cpp)
extern WXDLLIMPEXP_BASE void wxHandleProcessTermination(wxEndProcessData *proc_data);

// this function is called to associate the port-specific callback with the
// child process. The return valus is port-specific.
extern WXDLLIMPEXP_CORE int wxAddProcessCallback(wxEndProcessData *proc_data, int fd);

#if defined(__DARWIN__) && (defined(__WXMAC__) || defined(__WXCOCOA__))
// For ports (e.g. DARWIN) which can add callbacks based on the pid
extern int wxAddProcessCallbackForPid(wxEndProcessData *proc_data, int pid);
#endif

#endif // _WX_UNIX_EXECUTE_H
