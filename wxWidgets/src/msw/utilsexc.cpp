/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/utilsexc.cpp
// Purpose:     wxExecute implementation for MSW
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: utilsexc.cpp 54695 2008-07-18 22:22:16Z VZ $
// Copyright:   (c) 1998-2002 wxWidgets dev team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #if wxUSE_STREAMS
        #include "wx/stream.h"
    #endif
    #include "wx/module.h"
#endif

#include "wx/process.h"

#include "wx/apptrait.h"


#include "wx/msw/private.h"

#include <ctype.h>

#if !defined(__GNUWIN32__) && !defined(__SALFORDC__) && !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    #include <direct.h>
#ifndef __MWERKS__
    #include <dos.h>
#endif
#endif

#if defined(__GNUWIN32__)
    #include <sys/unistd.h>
    #include <sys/stat.h>
#endif

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    #ifndef __UNIX__
        #include <io.h>
    #endif

    #ifndef __GNUWIN32__
        #include <shellapi.h>
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __WATCOMC__
    #if !(defined(_MSC_VER) && (_MSC_VER > 800))
        #include <errno.h>
    #endif
#endif
#include <stdarg.h>

#if wxUSE_IPC
    #include "wx/dde.h"         // for WX_DDE hack in wxExecute
#endif // wxUSE_IPC

// implemented in utils.cpp
extern "C" WXDLLIMPEXP_BASE HWND
wxCreateHiddenWindow(LPCTSTR *pclassname, LPCTSTR classname, WNDPROC wndproc);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// this message is sent when the process we're waiting for terminates
#define wxWM_PROC_TERMINATED (WM_USER + 10000)

// ----------------------------------------------------------------------------
// this module globals
// ----------------------------------------------------------------------------

// we need to create a hidden window to receive the process termination
// notifications and for this we need a (Win) class name for it which we will
// register the first time it's needed
static const wxChar *wxMSWEXEC_WNDCLASSNAME = wxT("_wxExecute_Internal_Class");
static const wxChar *gs_classForHiddenWindow = NULL;

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

// structure describing the process we're being waiting for
struct wxExecuteData
{
public:
    ~wxExecuteData()
    {
        if ( !::CloseHandle(hProcess) )
        {
            wxLogLastError(wxT("CloseHandle(hProcess)"));
        }
    }

    HWND       hWnd;          // window to send wxWM_PROC_TERMINATED to
    HANDLE     hProcess;      // handle of the process
    DWORD      dwProcessId;   // pid of the process
    wxProcess *handler;
    DWORD      dwExitCode;    // the exit code of the process
    bool       state;         // set to false when the process finishes
};

class wxExecuteModule : public wxModule
{
public:
    virtual bool OnInit() { return true; }
    virtual void OnExit()
    {
        if ( *gs_classForHiddenWindow )
        {
            if ( !::UnregisterClass(wxMSWEXEC_WNDCLASSNAME, wxGetInstance()) )
            {
                wxLogLastError(_T("UnregisterClass(wxExecClass)"));
            }

            gs_classForHiddenWindow = NULL;
        }
    }

private:
    DECLARE_DYNAMIC_CLASS(wxExecuteModule)
};

#if wxUSE_STREAMS && !defined(__WXWINCE__)

// ----------------------------------------------------------------------------
// wxPipeStreams
// ----------------------------------------------------------------------------

class wxPipeInputStream: public wxInputStream
{
public:
    wxPipeInputStream(HANDLE hInput);
    virtual ~wxPipeInputStream();

    // returns true if the pipe is still opened
    bool IsOpened() const { return m_hInput != INVALID_HANDLE_VALUE; }

    // returns true if there is any data to be read from the pipe
    virtual bool CanRead() const;

protected:
    size_t OnSysRead(void *buffer, size_t len);

protected:
    HANDLE m_hInput;

    DECLARE_NO_COPY_CLASS(wxPipeInputStream)
};

class wxPipeOutputStream: public wxOutputStream
{
public:
    wxPipeOutputStream(HANDLE hOutput);
    virtual ~wxPipeOutputStream() { Close(); }
    bool Close();

protected:
    size_t OnSysWrite(const void *buffer, size_t len);

protected:
    HANDLE m_hOutput;

    DECLARE_NO_COPY_CLASS(wxPipeOutputStream)
};

// define this to let wxexec.cpp know that we know what we're doing
#define _WX_USED_BY_WXEXECUTE_
#include "../common/execcmn.cpp"

// ----------------------------------------------------------------------------
// wxPipe represents a Win32 anonymous pipe
// ----------------------------------------------------------------------------

class wxPipe
{
public:
    // the symbolic names for the pipe ends
    enum Direction
    {
        Read,
        Write
    };

    // default ctor doesn't do anything
    wxPipe() { m_handles[Read] = m_handles[Write] = INVALID_HANDLE_VALUE; }

    // create the pipe, return true if ok, false on error
    bool Create()
    {
        // default secutiry attributes
        SECURITY_ATTRIBUTES security;

        security.nLength              = sizeof(security);
        security.lpSecurityDescriptor = NULL;
        security.bInheritHandle       = TRUE; // to pass it to the child

        if ( !::CreatePipe(&m_handles[0], &m_handles[1], &security, 0) )
        {
            wxLogSysError(_("Failed to create an anonymous pipe"));

            return false;
        }

        return true;
    }

    // return true if we were created successfully
    bool IsOk() const { return m_handles[Read] != INVALID_HANDLE_VALUE; }

    // return the descriptor for one of the pipe ends
    HANDLE operator[](Direction which) const { return m_handles[which]; }

    // detach a descriptor, meaning that the pipe dtor won't close it, and
    // return it
    HANDLE Detach(Direction which)
    {
        HANDLE handle = m_handles[which];
        m_handles[which] = INVALID_HANDLE_VALUE;

        return handle;
    }

    // close the pipe descriptors
    void Close()
    {
        for ( size_t n = 0; n < WXSIZEOF(m_handles); n++ )
        {
            if ( m_handles[n] != INVALID_HANDLE_VALUE )
            {
                ::CloseHandle(m_handles[n]);
                m_handles[n] = INVALID_HANDLE_VALUE;
            }
        }
    }

    // dtor closes the pipe descriptors
    ~wxPipe() { Close(); }

private:
    HANDLE m_handles[2];
};

#endif // wxUSE_STREAMS

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// process termination detecting support
// ----------------------------------------------------------------------------

// thread function for the thread monitoring the process termination
static DWORD __stdcall wxExecuteThread(void *arg)
{
    wxExecuteData * const data = (wxExecuteData *)arg;

    if ( ::WaitForSingleObject(data->hProcess, INFINITE) != WAIT_OBJECT_0 )
    {
        wxLogDebug(_T("Waiting for the process termination failed!"));
    }

    // get the exit code
    if ( !::GetExitCodeProcess(data->hProcess, &data->dwExitCode) )
    {
        wxLogLastError(wxT("GetExitCodeProcess"));
    }

    wxASSERT_MSG( data->dwExitCode != STILL_ACTIVE,
                  wxT("process should have terminated") );

    // send a message indicating process termination to the window
    ::SendMessage(data->hWnd, wxWM_PROC_TERMINATED, 0, (LPARAM)data);

    return 0;
}

// window procedure of a hidden window which is created just to receive
// the notification message when a process exits
LRESULT APIENTRY _EXPORT wxExecuteWindowCbk(HWND hWnd, UINT message,
                                            WPARAM wParam, LPARAM lParam)
{
    if ( message == wxWM_PROC_TERMINATED )
    {
        DestroyWindow(hWnd);    // we don't need it any more

        wxExecuteData * const data = (wxExecuteData *)lParam;
        if ( data->handler )
        {
            data->handler->OnTerminate((int)data->dwProcessId,
                                       (int)data->dwExitCode);
        }

        if ( data->state )
        {
            // we're executing synchronously, tell the waiting thread
            // that the process finished
            data->state = 0;
        }
        else
        {
            // asynchronous execution - we should do the clean up
            delete data;
        }

        return 0;
    }
    else
    {
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// ============================================================================
// implementation of IO redirection support classes
// ============================================================================

#if wxUSE_STREAMS && !defined(__WXWINCE__)

// ----------------------------------------------------------------------------
// wxPipeInputStreams
// ----------------------------------------------------------------------------

wxPipeInputStream::wxPipeInputStream(HANDLE hInput)
{
    m_hInput = hInput;
}

wxPipeInputStream::~wxPipeInputStream()
{
    if ( m_hInput != INVALID_HANDLE_VALUE )
        ::CloseHandle(m_hInput);
}

bool wxPipeInputStream::CanRead() const
{
    // we can read if there's something in the put back buffer
    // even pipe is closed
    if ( m_wbacksize > m_wbackcur )
        return true;

    wxPipeInputStream * const self = wxConstCast(this, wxPipeInputStream);

    if ( !IsOpened() )
    {
        // set back to mark Eof as it may have been unset by Ungetch()
        self->m_lasterror = wxSTREAM_EOF;
        return false;
    }

    DWORD nAvailable;

    // function name is misleading, it works with anon pipes as well
    DWORD rc = ::PeekNamedPipe
                    (
                      m_hInput,     // handle
                      NULL, 0,      // ptr to buffer and its size
                      NULL,         // [out] bytes read
                      &nAvailable,  // [out] bytes available
                      NULL          // [out] bytes left
                    );

    if ( !rc )
    {
        if ( ::GetLastError() != ERROR_BROKEN_PIPE )
        {
            // unexpected error
            wxLogLastError(_T("PeekNamedPipe"));
        }

        // don't try to continue reading from a pipe if an error occurred or if
        // it had been closed
        ::CloseHandle(m_hInput);

        self->m_hInput = INVALID_HANDLE_VALUE;
        self->m_lasterror = wxSTREAM_EOF;

        nAvailable = 0;
    }

    return nAvailable != 0;
}

size_t wxPipeInputStream::OnSysRead(void *buffer, size_t len)
{
    if ( !IsOpened() )
    {
        m_lasterror = wxSTREAM_EOF;

        return 0;
    }

    DWORD bytesRead;
    if ( !::ReadFile(m_hInput, buffer, len, &bytesRead, NULL) )
    {
        m_lasterror = ::GetLastError() == ERROR_BROKEN_PIPE
                        ? wxSTREAM_EOF
                        : wxSTREAM_READ_ERROR;
    }

    // bytesRead is set to 0, as desired, if an error occurred
    return bytesRead;
}

// ----------------------------------------------------------------------------
// wxPipeOutputStream
// ----------------------------------------------------------------------------

wxPipeOutputStream::wxPipeOutputStream(HANDLE hOutput)
{
    m_hOutput = hOutput;

    // unblock the pipe to prevent deadlocks when we're writing to the pipe
    // from which the child process can't read because it is writing in its own
    // end of it
    DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    if ( !::SetNamedPipeHandleState
            (
                m_hOutput,
                &mode,
                NULL,       // collection count (we don't set it)
                NULL        // timeout (we don't set it neither)
            ) )
    {
        wxLogLastError(_T("SetNamedPipeHandleState(PIPE_NOWAIT)"));
    }
}

bool wxPipeOutputStream::Close()
{
   return ::CloseHandle(m_hOutput) != 0;
}


size_t wxPipeOutputStream::OnSysWrite(const void *buffer, size_t len)
{
    m_lasterror = wxSTREAM_NO_ERROR;

    DWORD totalWritten = 0;
    while ( len > 0 )
    {
        DWORD chunkWritten;
        if ( !::WriteFile(m_hOutput, buffer, len, &chunkWritten, NULL) )
        {
            m_lasterror = ::GetLastError() == ERROR_BROKEN_PIPE
                                ? wxSTREAM_EOF
                                : wxSTREAM_WRITE_ERROR;
            break;
        }

        if ( !chunkWritten )
            break;

        buffer = (char *)buffer + chunkWritten;
        totalWritten += chunkWritten;
        len -= chunkWritten;
    }

    return totalWritten;
}

#endif // wxUSE_STREAMS

// ============================================================================
// wxExecute functions family
// ============================================================================

#if wxUSE_IPC

// connect to the given server via DDE and ask it to execute the command
bool
wxExecuteDDE(const wxString& ddeServer,
             const wxString& ddeTopic,
             const wxString& ddeCommand)
{
    bool ok wxDUMMY_INITIALIZE(false);

    wxDDEClient client;
    wxConnectionBase *
        conn = client.MakeConnection(wxEmptyString, ddeServer, ddeTopic);
    if ( !conn )
    {
        ok = false;
    }
    else // connected to DDE server
    {
        // the added complication here is that although most programs use
        // XTYP_EXECUTE for their DDE API, some important ones -- like Word
        // and other MS stuff - use XTYP_REQUEST!
        //
        // moreover, anotheri mportant program (IE) understands both but
        // returns an error from Execute() so we must try Request() first
        // to avoid doing it twice
        {
            // we're prepared for this one to fail, so don't show errors
            wxLogNull noErrors;

            ok = conn->Request(ddeCommand) != NULL;
        }

        if ( !ok )
        {
            // now try execute -- but show the errors
            ok = conn->Execute(ddeCommand);
        }
    }

    return ok;
}

#endif // wxUSE_IPC

long wxExecute(const wxString& cmd, int flags, wxProcess *handler)
{
    wxCHECK_MSG( !cmd.empty(), 0, wxT("empty command in wxExecute") );

#if wxUSE_THREADS
    // for many reasons, the code below breaks down if it's called from another
    // thread -- this could be fixed, but as Unix versions don't support this
    // neither I don't want to waste time on this now
    wxASSERT_MSG( wxThread::IsMain(),
                    _T("wxExecute() can be called only from the main thread") );
#endif // wxUSE_THREADS

    wxString command;

#if wxUSE_IPC
    // DDE hack: this is really not pretty, but we need to allow this for
    // transparent handling of DDE servers in wxMimeTypesManager. Usually it
    // returns the command which should be run to view/open/... a file of the
    // given type. Sometimes, however, this command just launches the server
    // and an additional DDE request must be made to really open the file. To
    // keep all this well hidden from the application, we allow a special form
    // of command: WX_DDE#<command>#DDE_SERVER#DDE_TOPIC#DDE_COMMAND in which
    // case we execute just <command> and process the rest below
    wxString ddeServer, ddeTopic, ddeCommand;
    static const size_t lenDdePrefix = 7;   // strlen("WX_DDE:")
    if ( cmd.Left(lenDdePrefix) == _T("WX_DDE#") )
    {
        // speed up the concatenations below
        ddeServer.reserve(256);
        ddeTopic.reserve(256);
        ddeCommand.reserve(256);

        const wxChar *p = cmd.c_str() + 7;
        while ( *p && *p != _T('#') )
        {
            command += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(_T("invalid WX_DDE command in wxExecute"));
        }

        while ( *p && *p != _T('#') )
        {
            ddeServer += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(_T("invalid WX_DDE command in wxExecute"));
        }

        while ( *p && *p != _T('#') )
        {
            ddeTopic += *p++;
        }

        if ( *p )
        {
            // skip '#'
            p++;
        }
        else
        {
            wxFAIL_MSG(_T("invalid WX_DDE command in wxExecute"));
        }

        while ( *p )
        {
            ddeCommand += *p++;
        }

        // if we want to just launch the program and not wait for its
        // termination, try to execute DDE command right now, it can succeed if
        // the process is already running - but as it fails if it's not
        // running, suppress any errors it might generate
        if ( !(flags & wxEXEC_SYNC) )
        {
            wxLogNull noErrors;
            if ( wxExecuteDDE(ddeServer, ddeTopic, ddeCommand) )
            {
                // a dummy PID - this is a hack, of course, but it's well worth
                // it as we don't open a new server each time we're called
                // which would be quite bad
                return -1;
            }
        }
    }
    else
#endif // wxUSE_IPC
    {
        // no DDE
        command = cmd;
    }

    // the IO redirection is only supported with wxUSE_STREAMS
    BOOL redirect = FALSE;

#if wxUSE_STREAMS && !defined(__WXWINCE__)
    wxPipe pipeIn, pipeOut, pipeErr;

    // we'll save here the copy of pipeIn[Write]
    HANDLE hpipeStdinWrite = INVALID_HANDLE_VALUE;

    // open the pipes to which child process IO will be redirected if needed
    if ( handler && handler->IsRedirected() )
    {
        // create pipes for redirecting stdin, stdout and stderr
        if ( !pipeIn.Create() || !pipeOut.Create() || !pipeErr.Create() )
        {
            wxLogSysError(_("Failed to redirect the child process IO"));

            // indicate failure: we need to return different error code
            // depending on the sync flag
            return flags & wxEXEC_SYNC ? -1 : 0;
        }

        redirect = TRUE;
    }
#endif // wxUSE_STREAMS

    // create the process
    STARTUPINFO si;
    wxZeroMemory(si);
    si.cb = sizeof(si);

#if wxUSE_STREAMS && !defined(__WXWINCE__)
    if ( redirect )
    {
        si.dwFlags = STARTF_USESTDHANDLES;

        si.hStdInput = pipeIn[wxPipe::Read];
        si.hStdOutput = pipeOut[wxPipe::Write];
        si.hStdError = pipeErr[wxPipe::Write];

        // when the std IO is redirected, we don't show the (console) process
        // window by default, but this can be overridden by the caller by
        // specifying wxEXEC_NOHIDE flag
        if ( !(flags & wxEXEC_NOHIDE) )
        {
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
        }

        // we must duplicate the handle to the write side of stdin pipe to make
        // it non inheritable: indeed, we must close the writing end of pipeIn
        // before launching the child process as otherwise this handle will be
        // inherited by the child which will never close it and so the pipe
        // will never be closed and the child will be left stuck in ReadFile()
        HANDLE pipeInWrite = pipeIn.Detach(wxPipe::Write);
        if ( !::DuplicateHandle
                (
                    ::GetCurrentProcess(),
                    pipeInWrite,
                    ::GetCurrentProcess(),
                    &hpipeStdinWrite,
                    0,                      // desired access: unused here
                    FALSE,                  // not inherited
                    DUPLICATE_SAME_ACCESS   // same access as for src handle
                ) )
        {
            wxLogLastError(_T("DuplicateHandle"));
        }

        ::CloseHandle(pipeInWrite);
    }
#endif // wxUSE_STREAMS

    PROCESS_INFORMATION pi;
    DWORD dwFlags = CREATE_SUSPENDED;

#ifndef __WXWINCE__
    dwFlags |= CREATE_DEFAULT_ERROR_MODE ;
#else
    // we are assuming commands without spaces for now
    wxString moduleName = command.BeforeFirst(wxT(' '));
    wxString arguments = command.AfterFirst(wxT(' '));
#endif

    bool ok = ::CreateProcess
                (
                    // WinCE requires appname to be non null
                    // Win32 allows for null
#ifdef __WXWINCE__
                 (wxChar *)
                 moduleName.c_str(), // application name
                 (wxChar *)
                 arguments.c_str(),  // arguments
#else
                 NULL,               // application name (use only cmd line)
                 (wxChar *)
                 command.c_str(),    // full command line
#endif
                 NULL,               // security attributes: defaults for both
                 NULL,               //   the process and its main thread
                 redirect,           // inherit handles if we use pipes
                 dwFlags,            // process creation flags
                 NULL,               // environment (use the same)
                 NULL,               // current directory (use the same)
                 &si,                // startup info (unused here)
                 &pi                 // process info
                ) != 0;

#if wxUSE_STREAMS && !defined(__WXWINCE__)
    // we can close the pipe ends used by child anyhow
    if ( redirect )
    {
        ::CloseHandle(pipeIn.Detach(wxPipe::Read));
        ::CloseHandle(pipeOut.Detach(wxPipe::Write));
        ::CloseHandle(pipeErr.Detach(wxPipe::Write));
    }
#endif // wxUSE_STREAMS

    if ( !ok )
    {
#if wxUSE_STREAMS && !defined(__WXWINCE__)
        // close the other handles too
        if ( redirect )
        {
            ::CloseHandle(pipeOut.Detach(wxPipe::Read));
            ::CloseHandle(pipeErr.Detach(wxPipe::Read));
        }
#endif // wxUSE_STREAMS

        wxLogSysError(_("Execution of command '%s' failed"), command.c_str());

        return flags & wxEXEC_SYNC ? -1 : 0;
    }

#if wxUSE_STREAMS && !defined(__WXWINCE__)
    // the input buffer bufOut is connected to stdout, this is why it is
    // called bufOut and not bufIn
    wxStreamTempInputBuffer bufOut,
                            bufErr;

    if ( redirect )
    {
        // We can now initialize the wxStreams
        wxPipeInputStream *
            outStream = new wxPipeInputStream(pipeOut.Detach(wxPipe::Read));
        wxPipeInputStream *
            errStream = new wxPipeInputStream(pipeErr.Detach(wxPipe::Read));
        wxPipeOutputStream *
            inStream = new wxPipeOutputStream(hpipeStdinWrite);

        handler->SetPipeStreams(outStream, inStream, errStream);

        bufOut.Init(outStream);
        bufErr.Init(errStream);
    }
#endif // wxUSE_STREAMS

    // create a hidden window to receive notification about process
    // termination
    HWND hwnd = wxCreateHiddenWindow
                (
                    &gs_classForHiddenWindow,
                    wxMSWEXEC_WNDCLASSNAME,
                    (WNDPROC)wxExecuteWindowCbk
                );

    wxASSERT_MSG( hwnd, wxT("can't create a hidden window for wxExecute") );

    // Alloc data
    wxExecuteData *data = new wxExecuteData;
    data->hProcess    = pi.hProcess;
    data->dwProcessId = pi.dwProcessId;
    data->hWnd        = hwnd;
    data->state       = (flags & wxEXEC_SYNC) != 0;
    if ( flags & wxEXEC_SYNC )
    {
        // handler may be !NULL for capturing program output, but we don't use
        // it wxExecuteData struct in this case
        data->handler = NULL;
    }
    else
    {
        // may be NULL or not
        data->handler = handler;
    }

    DWORD tid;
    HANDLE hThread = ::CreateThread(NULL,
                                    0,
                                    wxExecuteThread,
                                    (void *)data,
                                    0,
                                    &tid);

    // resume process we created now - whether the thread creation succeeded or
    // not
    if ( ::ResumeThread(pi.hThread) == (DWORD)-1 )
    {
        // ignore it - what can we do?
        wxLogLastError(wxT("ResumeThread in wxExecute"));
    }

    // close unneeded handle
    if ( !::CloseHandle(pi.hThread) )
        wxLogLastError(wxT("CloseHandle(hThread)"));

    if ( !hThread )
    {
        wxLogLastError(wxT("CreateThread in wxExecute"));

        DestroyWindow(hwnd);
        delete data;

        // the process still started up successfully...
        return pi.dwProcessId;
    }

    ::CloseHandle(hThread);

#if wxUSE_IPC && !defined(__WXWINCE__)
    // second part of DDE hack: now establish the DDE conversation with the
    // just launched process
    if ( !ddeServer.empty() )
    {
        bool ok;

        // give the process the time to init itself
        //
        // we use a very big timeout hoping that WaitForInputIdle() will return
        // much sooner, but not INFINITE just in case the process hangs
        // completely - like this we will regain control sooner or later
        switch ( ::WaitForInputIdle(pi.hProcess, 10000 /* 10 seconds */) )
        {
            default:
                wxFAIL_MSG( _T("unexpected WaitForInputIdle() return code") );
                // fall through

            case -1:
                wxLogLastError(_T("WaitForInputIdle() in wxExecute"));

            case WAIT_TIMEOUT:
                wxLogDebug(_T("Timeout too small in WaitForInputIdle"));

                ok = false;
                break;

            case 0:
                // ok, process ready to accept DDE requests
                ok = wxExecuteDDE(ddeServer, ddeTopic, ddeCommand);
        }

        if ( !ok )
        {
            wxLogDebug(_T("Failed to send DDE request to the process \"%s\"."),
                       cmd.c_str());
        }
    }
#endif // wxUSE_IPC

    if ( !(flags & wxEXEC_SYNC) )
    {
        // clean up will be done when the process terminates

        // return the pid
        return pi.dwProcessId;
    }

    wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
    wxCHECK_MSG( traits, -1, _T("no wxAppTraits in wxExecute()?") );

    void *cookie = NULL;
    if ( !(flags & wxEXEC_NODISABLE) )
    {
        // disable all app windows while waiting for the child process to finish
        cookie = traits->BeforeChildWaitLoop();
    }

    // wait until the child process terminates
    while ( data->state )
    {
#if wxUSE_STREAMS && !defined(__WXWINCE__)
        bufOut.Update();
        bufErr.Update();
#endif // wxUSE_STREAMS

        // don't eat 100% of the CPU -- ugly but anything else requires
        // real async IO which we don't have for the moment
        ::Sleep(50);

        // we must process messages or we'd never get wxWM_PROC_TERMINATED
        traits->AlwaysYield();
    }

    if ( !(flags & wxEXEC_NODISABLE) )
    {
        // reenable disabled windows back
        traits->AfterChildWaitLoop(cookie);
    }

    DWORD dwExitCode = data->dwExitCode;
    delete data;

    // return the exit code
    return dwExitCode;
}

long wxExecute(wxChar **argv, int flags, wxProcess *handler)
{
    wxString command;

    wxString arg;
    for ( ;; )
    {
        arg = *argv++;

        // we didn't quote the arguments properly in the previous wx versions
        // and while this is the right thing to do, there is a good chance that
        // people worked around our bug in their code by quoting the arguments
        // manually before, so, for compatibility sake, keep the argument
        // unchanged if it's already quoted

        bool quote;
        if ( arg.empty() )
        {
            // we need to quote empty arguments, otherwise they'd just
            // disappear
            quote = true;
        }
        else // non-empty
        {
            if ( *arg.begin() != _T('"') || *arg.rbegin() != _T('"') )
            {
                // escape any quotes present in the string to avoid interfering
                // with the command line parsing in the child process
                arg.Replace(_T("\""), _T("\\\""), true /* replace all */);

                // and quote any arguments containing the spaces to prevent
                // them from being broken down
                quote = arg.find_first_of(_T(" \t")) != wxString::npos;
            }
            else // already quoted
            {
                quote = false;
            }
        }

        if ( quote )
            command += _T('\"') + arg + _T('\"');
        else
            command += arg;

        if ( !*argv )
            break;

        command += _T(' ');
    }

    return wxExecute(command, flags, handler);
}
