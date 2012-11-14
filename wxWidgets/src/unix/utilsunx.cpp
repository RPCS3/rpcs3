/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/utilsunx.cpp
// Purpose:     generic Unix implementation of many wx functions
// Author:      Vadim Zeitlin
// Id:          $Id: utilsunx.cpp 49239 2007-10-19 03:10:31Z DE $
// Copyright:   (c) 1998 Robert Roebling, Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "wx/utils.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
#endif

#include "wx/apptrait.h"

#include "wx/process.h"
#include "wx/thread.h"

#include "wx/wfstream.h"

#include "wx/unix/execute.h"
#include "wx/unix/private.h"

#include <pwd.h>

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#define HAS_PIPE_INPUT_STREAM (wxUSE_STREAMS && wxUSE_FILE)

#if HAS_PIPE_INPUT_STREAM

// define this to let wxexec.cpp know that we know what we're doing
#define _WX_USED_BY_WXEXECUTE_
#include "../common/execcmn.cpp"

#endif // HAS_PIPE_INPUT_STREAM

#if wxUSE_BASE

#if defined(__MWERKS__) && defined(__MACH__)
    #ifndef WXWIN_OS_DESCRIPTION
        #define WXWIN_OS_DESCRIPTION "MacOS X"
    #endif
    #ifndef HAVE_NANOSLEEP
        #define HAVE_NANOSLEEP
    #endif
    #ifndef HAVE_UNAME
        #define HAVE_UNAME
    #endif

    // our configure test believes we can use sigaction() if the function is
    // available but Metrowekrs with MSL run-time does have the function but
    // doesn't have sigaction struct so finally we can't use it...
    #ifdef __MSL__
        #undef wxUSE_ON_FATAL_EXCEPTION
        #define wxUSE_ON_FATAL_EXCEPTION 0
    #endif
#endif

// not only the statfs syscall is called differently depending on platform, but
// one of its incarnations, statvfs(), takes different arguments under
// different platforms and even different versions of the same system (Solaris
// 7 and 8): if you want to test for this, don't forget that the problems only
// appear if the large files support is enabled
#ifdef HAVE_STATFS
    #ifdef __BSD__
        #include <sys/param.h>
        #include <sys/mount.h>
    #else // !__BSD__
        #include <sys/vfs.h>
    #endif // __BSD__/!__BSD__

    #define wxStatfs statfs

    #ifndef HAVE_STATFS_DECL
        // some systems lack statfs() prototype in the system headers (AIX 4)
        extern "C" int statfs(const char *path, struct statfs *buf);
    #endif
#endif // HAVE_STATFS

#ifdef HAVE_STATVFS
    #include <sys/statvfs.h>

    #define wxStatfs statvfs
#endif // HAVE_STATVFS

#if defined(HAVE_STATFS) || defined(HAVE_STATVFS)
    // WX_STATFS_T is detected by configure
    #define wxStatfs_t WX_STATFS_T
#endif

// SGI signal.h defines signal handler arguments differently depending on
// whether _LANGUAGE_C_PLUS_PLUS is set or not - do set it
#if defined(__SGI__) && !defined(_LANGUAGE_C_PLUS_PLUS)
    #define _LANGUAGE_C_PLUS_PLUS 1
#endif // SGI hack

#include <stdarg.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>          // for O_WRONLY and friends
#include <time.h>           // nanosleep() and/or usleep()
#include <ctype.h>          // isspace()
#include <sys/time.h>       // needed for FD_SETSIZE

#ifdef HAVE_UNAME
    #include <sys/utsname.h> // for uname()
#endif // HAVE_UNAME

// Used by wxGetFreeMemory().
#ifdef __SGI__
    #include <sys/sysmp.h>
    #include <sys/sysinfo.h>   // for SAGET and MINFO structures
#endif

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

// many versions of Unices have this function, but it is not defined in system
// headers - please add your system here if it is the case for your OS.
// SunOS < 5.6 (i.e. Solaris < 2.6) and DG-UX are like this.
#if !defined(HAVE_USLEEP) && \
    ((defined(__SUN__) && !defined(__SunOs_5_6) && \
                         !defined(__SunOs_5_7) && !defined(__SUNPRO_CC)) || \
     defined(__osf__) || defined(__EMX__))
    extern "C"
    {
        #ifdef __EMX__
            /* I copied this from the XFree86 diffs. AV. */
            #define INCL_DOSPROCESS
            #include <os2.h>
            inline void usleep(unsigned long delay)
            {
                DosSleep(delay ? (delay/1000l) : 1l);
            }
        #else // Unix
            int usleep(unsigned int usec);
        #endif // __EMX__/Unix
    };

    #define HAVE_USLEEP 1
#endif // Unices without usleep()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// sleeping
// ----------------------------------------------------------------------------

void wxSleep(int nSecs)
{
    sleep(nSecs);
}

void wxMicroSleep(unsigned long microseconds)
{
#if defined(HAVE_NANOSLEEP)
    timespec tmReq;
    tmReq.tv_sec = (time_t)(microseconds / 1000000);
    tmReq.tv_nsec = (microseconds % 1000000) * 1000;

    // we're not interested in remaining time nor in return value
    (void)nanosleep(&tmReq, (timespec *)NULL);
#elif defined(HAVE_USLEEP)
    // uncomment this if you feel brave or if you are sure that your version
    // of Solaris has a safe usleep() function but please notice that usleep()
    // is known to lead to crashes in MT programs in Solaris 2.[67] and is not
    // documented as MT-Safe
    #if defined(__SUN__) && wxUSE_THREADS
        #error "usleep() cannot be used in MT programs under Solaris."
    #endif // Sun

    usleep(microseconds);
#elif defined(HAVE_SLEEP)
    // under BeOS sleep() takes seconds (what about other platforms, if any?)
    sleep(microseconds * 1000000);
#else // !sleep function
    #error "usleep() or nanosleep() function required for wxMicroSleep"
#endif // sleep function
}

void wxMilliSleep(unsigned long milliseconds)
{
    wxMicroSleep(milliseconds*1000);
}

// ----------------------------------------------------------------------------
// process management
// ----------------------------------------------------------------------------

int wxKill(long pid, wxSignal sig, wxKillError *rc, int flags)
{
    int err = kill((pid_t) (flags & wxKILL_CHILDREN) ? -pid : pid, (int)sig);
    if ( rc )
    {
        switch ( err ? errno : 0 )
        {
            case 0:
                *rc = wxKILL_OK;
                break;

            case EINVAL:
                *rc = wxKILL_BAD_SIGNAL;
                break;

            case EPERM:
                *rc = wxKILL_ACCESS_DENIED;
                break;

            case ESRCH:
                *rc = wxKILL_NO_PROCESS;
                break;

            default:
                // this goes against Unix98 docs so log it
                wxLogDebug(_T("unexpected kill(2) return value %d"), err);

                // something else...
                *rc = wxKILL_ERROR;
        }
    }

    return err;
}

#define WXEXECUTE_NARGS   127

#if defined(__DARWIN__)
long wxMacExecute(wxChar **argv,
               int flags,
               wxProcess *process);
#endif

long wxExecute( const wxString& command, int flags, wxProcess *process )
{
    wxCHECK_MSG( !command.empty(), 0, wxT("can't exec empty command") );

    wxLogTrace(wxT("exec"), wxT("Executing \"%s\""), command.c_str());

#if wxUSE_THREADS
    // fork() doesn't mix well with POSIX threads: on many systems the program
    // deadlocks or crashes for some reason. Probably our code is buggy and
    // doesn't do something which must be done to allow this to work, but I
    // don't know what yet, so for now just warn the user (this is the least we
    // can do) about it
    wxASSERT_MSG( wxThread::IsMain(),
                    _T("wxExecute() can be called only from the main thread") );
#endif // wxUSE_THREADS

    int argc = 0;
    wxChar *argv[WXEXECUTE_NARGS];
    wxString argument;
    const wxChar *cptr = command.c_str();
    wxChar quotechar = wxT('\0'); // is arg quoted?
    bool escaped = false;

    // split the command line in arguments
    do
    {
        argument = wxEmptyString;
        quotechar = wxT('\0');

        // eat leading whitespace:
        while ( wxIsspace(*cptr) )
            cptr++;

        if ( *cptr == wxT('\'') || *cptr == wxT('"') )
            quotechar = *cptr++;

        do
        {
            if ( *cptr == wxT('\\') && ! escaped )
            {
                escaped = true;
                cptr++;
                continue;
            }

            // all other characters:
            argument += *cptr++;
            escaped = false;

            // have we reached the end of the argument?
            if ( (*cptr == quotechar && ! escaped)
                 || (quotechar == wxT('\0') && wxIsspace(*cptr))
                 || *cptr == wxT('\0') )
            {
                wxASSERT_MSG( argc < WXEXECUTE_NARGS,
                              wxT("too many arguments in wxExecute") );

                argv[argc] = new wxChar[argument.length() + 1];
                wxStrcpy(argv[argc], argument.c_str());
                argc++;

                // if not at end of buffer, swallow last character:
                if(*cptr)
                    cptr++;

                break; // done with this one, start over
            }
        } while(*cptr);
    } while(*cptr);
    argv[argc] = NULL;

    long lRc;
#if defined(__DARWIN__)
    // wxMacExecute only executes app bundles.
    // It returns an error code if the target is not an app bundle, thus falling
    // through to the regular wxExecute for non app bundles.
    lRc = wxMacExecute(argv, flags, process);
    if( lRc != ((flags & wxEXEC_SYNC) ? -1 : 0))
        return lRc;
#endif

    // do execute the command
    lRc = wxExecute(argv, flags, process);

    // clean up
    argc = 0;
    while( argv[argc] )
        delete [] argv[argc++];

    return lRc;
}

// ----------------------------------------------------------------------------
// wxShell
// ----------------------------------------------------------------------------

static wxString wxMakeShellCommand(const wxString& command)
{
    wxString cmd;
    if ( !command )
    {
        // just an interactive shell
        cmd = _T("xterm");
    }
    else
    {
        // execute command in a shell
        cmd << _T("/bin/sh -c '") << command << _T('\'');
    }

    return cmd;
}

bool wxShell(const wxString& command)
{
    return wxExecute(wxMakeShellCommand(command), wxEXEC_SYNC) == 0;
}

bool wxShell(const wxString& command, wxArrayString& output)
{
    wxCHECK_MSG( !command.empty(), false, _T("can't exec shell non interactively") );

    return wxExecute(wxMakeShellCommand(command), output);
}

// Shutdown or reboot the PC
bool wxShutdown(wxShutdownFlags wFlags)
{
    wxChar level;
    switch ( wFlags )
    {
        case wxSHUTDOWN_POWEROFF:
            level = _T('0');
            break;

        case wxSHUTDOWN_REBOOT:
            level = _T('6');
            break;

        default:
            wxFAIL_MSG( _T("unknown wxShutdown() flag") );
            return false;
    }

    return system(wxString::Format(_T("init %c"), level).mb_str()) == 0;
}

// ----------------------------------------------------------------------------
// wxStream classes to support IO redirection in wxExecute
// ----------------------------------------------------------------------------

#if HAS_PIPE_INPUT_STREAM

bool wxPipeInputStream::CanRead() const
{
    if ( m_lasterror == wxSTREAM_EOF )
        return false;

    // check if there is any input available
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    const int fd = m_file->fd();

    fd_set readfds;

    wxFD_ZERO(&readfds);
    wxFD_SET(fd, &readfds);

    switch ( select(fd + 1, &readfds, NULL, NULL, &tv) )
    {
        case -1:
            wxLogSysError(_("Impossible to get child process input"));
            // fall through

        case 0:
            return false;

        default:
            wxFAIL_MSG(_T("unexpected select() return value"));
            // still fall through

        case 1:
            // input available -- or maybe not, as select() returns 1 when a
            // read() will complete without delay, but it could still not read
            // anything
            return !Eof();
    }
}

#endif // HAS_PIPE_INPUT_STREAM

// ----------------------------------------------------------------------------
// wxExecute: the real worker function
// ----------------------------------------------------------------------------

long wxExecute(wxChar **argv, int flags, wxProcess *process)
{
    // for the sync execution, we return -1 to indicate failure, but for async
    // case we return 0 which is never a valid PID
    //
    // we define this as a macro, not a variable, to avoid compiler warnings
    // about "ERROR_RETURN_CODE value may be clobbered by fork()"
    #define ERROR_RETURN_CODE ((flags & wxEXEC_SYNC) ? -1 : 0)

    wxCHECK_MSG( *argv, ERROR_RETURN_CODE, wxT("can't exec empty command") );

#if wxUSE_UNICODE
    int mb_argc = 0;
    char *mb_argv[WXEXECUTE_NARGS];

    while (argv[mb_argc])
    {
        wxWX2MBbuf mb_arg = wxSafeConvertWX2MB(argv[mb_argc]);
        mb_argv[mb_argc] = strdup(mb_arg);
        mb_argc++;
    }
    mb_argv[mb_argc] = (char *) NULL;

    // this macro will free memory we used above
    #define ARGS_CLEANUP                                 \
        for ( mb_argc = 0; mb_argv[mb_argc]; mb_argc++ ) \
            free(mb_argv[mb_argc])
#else // ANSI
    // no need for cleanup
    #define ARGS_CLEANUP

    wxChar **mb_argv = argv;
#endif // Unicode/ANSI

    // we want this function to work even if there is no wxApp so ensure that
    // we have a valid traits pointer
    wxConsoleAppTraits traitsConsole;
    wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
    if ( !traits )
        traits = &traitsConsole;

    // this struct contains all information which we pass to and from
    // wxAppTraits methods
    wxExecuteData execData;
    execData.flags = flags;
    execData.process = process;

    // create pipes
    if ( !traits->CreateEndProcessPipe(execData) )
    {
        wxLogError( _("Failed to execute '%s'\n"), *argv );

        ARGS_CLEANUP;

        return ERROR_RETURN_CODE;
    }

    // pipes for inter process communication
    wxPipe pipeIn,      // stdin
           pipeOut,     // stdout
           pipeErr;     // stderr

    if ( process && process->IsRedirected() )
    {
        if ( !pipeIn.Create() || !pipeOut.Create() || !pipeErr.Create() )
        {
            wxLogError( _("Failed to execute '%s'\n"), *argv );

            ARGS_CLEANUP;

            return ERROR_RETURN_CODE;
        }
    }

    // fork the process
    //
    // NB: do *not* use vfork() here, it completely breaks this code for some
    //     reason under Solaris (and maybe others, although not under Linux)
    //     But on OpenVMS we do not have fork so we have to use vfork and
    //     cross our fingers that it works.
#ifdef __VMS
   pid_t pid = vfork();
#else
   pid_t pid = fork();
#endif
   if ( pid == -1 )     // error?
    {
        wxLogSysError( _("Fork failed") );

        ARGS_CLEANUP;

        return ERROR_RETURN_CODE;
    }
    else if ( pid == 0 )  // we're in child
    {
        // These lines close the open file descriptors to to avoid any
        // input/output which might block the process or irritate the user. If
        // one wants proper IO for the subprocess, the right thing to do is to
        // start an xterm executing it.
        if ( !(flags & wxEXEC_SYNC) )
        {
            // FD_SETSIZE is unsigned under BSD, signed under other platforms
            // so we need a cast to avoid warnings on all platforms
            for ( int fd = 0; fd < (int)FD_SETSIZE; fd++ )
            {
                if ( fd == pipeIn[wxPipe::Read]
                        || fd == pipeOut[wxPipe::Write]
                        || fd == pipeErr[wxPipe::Write]
                        || traits->IsWriteFDOfEndProcessPipe(execData, fd) )
                {
                    // don't close this one, we still need it
                    continue;
                }

                // leave stderr opened too, it won't do any harm
                if ( fd != STDERR_FILENO )
                    close(fd);
            }
        }

#if !defined(__VMS) && !defined(__EMX__)
        if ( flags & wxEXEC_MAKE_GROUP_LEADER )
        {
            // Set process group to child process' pid.  Then killing -pid
            // of the parent will kill the process and all of its children.
            setsid();
        }
#endif // !__VMS

        // reading side can be safely closed but we should keep the write one
        // opened
        traits->DetachWriteFDOfEndProcessPipe(execData);

        // redirect stdin, stdout and stderr
        if ( pipeIn.IsOk() )
        {
            if ( dup2(pipeIn[wxPipe::Read], STDIN_FILENO) == -1 ||
                 dup2(pipeOut[wxPipe::Write], STDOUT_FILENO) == -1 ||
                 dup2(pipeErr[wxPipe::Write], STDERR_FILENO) == -1 )
            {
                wxLogSysError(_("Failed to redirect child process input/output"));
            }

            pipeIn.Close();
            pipeOut.Close();
            pipeErr.Close();
        }

        execvp (*mb_argv, mb_argv);

        fprintf(stderr, "execvp(");
        // CS changed ppc to ppc_ as ppc is not available under mac os CW Mach-O
        for ( char **ppc_ = mb_argv; *ppc_; ppc_++ )
            fprintf(stderr, "%s%s", ppc_ == mb_argv ? "" : ", ", *ppc_);
        fprintf(stderr, ") failed with error %d!\n", errno);

        // there is no return after successful exec()
        _exit(-1);

        // some compilers complain about missing return - of course, they
        // should know that exit() doesn't return but what else can we do if
        // they don't?
        //
        // and, sure enough, other compilers complain about unreachable code
        // after exit() call, so we can just always have return here...
#if defined(__VMS) || defined(__INTEL_COMPILER)
        return 0;
#endif
    }
    else // we're in parent
    {
        ARGS_CLEANUP;

        // save it for WaitForChild() use
        execData.pid = pid;

        // prepare for IO redirection

#if HAS_PIPE_INPUT_STREAM
        // the input buffer bufOut is connected to stdout, this is why it is
        // called bufOut and not bufIn
        wxStreamTempInputBuffer bufOut,
                                bufErr;
#endif // HAS_PIPE_INPUT_STREAM

        if ( process && process->IsRedirected() )
        {
#if HAS_PIPE_INPUT_STREAM
            wxOutputStream *inStream =
                new wxFileOutputStream(pipeIn.Detach(wxPipe::Write));

            wxPipeInputStream *outStream =
                new wxPipeInputStream(pipeOut.Detach(wxPipe::Read));

            wxPipeInputStream *errStream =
                new wxPipeInputStream(pipeErr.Detach(wxPipe::Read));

            process->SetPipeStreams(outStream, inStream, errStream);

            bufOut.Init(outStream);
            bufErr.Init(errStream);

            execData.bufOut = &bufOut;
            execData.bufErr = &bufErr;
#endif // HAS_PIPE_INPUT_STREAM
        }

        if ( pipeIn.IsOk() )
        {
            pipeIn.Close();
            pipeOut.Close();
            pipeErr.Close();
        }

        return traits->WaitForChild(execData);
    }

#if !defined(__VMS) && !defined(__INTEL_COMPILER)
    return ERROR_RETURN_CODE;
#endif
}

#undef ERROR_RETURN_CODE
#undef ARGS_CLEANUP

// ----------------------------------------------------------------------------
// file and directory functions
// ----------------------------------------------------------------------------

const wxChar* wxGetHomeDir( wxString *home  )
{
    *home = wxGetUserHome( wxEmptyString );
    wxString tmp;
    if ( home->empty() )
        *home = wxT("/");
#ifdef __VMS
    tmp = *home;
    if ( tmp.Last() != wxT(']'))
        if ( tmp.Last() != wxT('/')) *home << wxT('/');
#endif
    return home->c_str();
}

#if wxUSE_UNICODE
const wxMB2WXbuf wxGetUserHome( const wxString &user )
#else // just for binary compatibility -- there is no 'const' here
char *wxGetUserHome( const wxString &user )
#endif
{
    struct passwd *who = (struct passwd *) NULL;

    if ( !user )
    {
        wxChar *ptr;

        if ((ptr = wxGetenv(wxT("HOME"))) != NULL)
        {
#if wxUSE_UNICODE
            wxWCharBuffer buffer( ptr );
            return buffer;
#else
            return ptr;
#endif
        }
        if ((ptr = wxGetenv(wxT("USER"))) != NULL || (ptr = wxGetenv(wxT("LOGNAME"))) != NULL)
        {
            who = getpwnam(wxSafeConvertWX2MB(ptr));
        }

        // We now make sure the the user exists!
        if (who == NULL)
        {
            who = getpwuid(getuid());
        }
    }
    else
    {
      who = getpwnam (user.mb_str());
    }

    return wxSafeConvertMB2WX(who ? who->pw_dir : 0);
}

// ----------------------------------------------------------------------------
// network and user id routines
// ----------------------------------------------------------------------------

// private utility function which returns output of the given command, removing
// the trailing newline
static wxString wxGetCommandOutput(const wxString &cmd)
{
    FILE *f = popen(cmd.ToAscii(), "r");
    if ( !f )
    {
        wxLogSysError(_T("Executing \"%s\" failed"), cmd.c_str());
        return wxEmptyString;
    }

    wxString s;
    char buf[256];
    while ( !feof(f) )
    {
        if ( !fgets(buf, sizeof(buf), f) )
            break;

        s += wxString::FromAscii(buf);
    }

    pclose(f);

    if ( !s.empty() && s.Last() == _T('\n') )
        s.RemoveLast();

    return s;
}

// retrieve either the hostname or FQDN depending on platform (caller must
// check whether it's one or the other, this is why this function is for
// private use only)
static bool wxGetHostNameInternal(wxChar *buf, int sz)
{
    wxCHECK_MSG( buf, false, wxT("NULL pointer in wxGetHostNameInternal") );

    *buf = wxT('\0');

    // we're using uname() which is POSIX instead of less standard sysinfo()
#if defined(HAVE_UNAME)
    struct utsname uts;
    bool ok = uname(&uts) != -1;
    if ( ok )
    {
        wxStrncpy(buf, wxSafeConvertMB2WX(uts.nodename), sz - 1);
        buf[sz] = wxT('\0');
    }
#elif defined(HAVE_GETHOSTNAME)
    char cbuf[sz];
    bool ok = gethostname(cbuf, sz) != -1;
    if ( ok )
    {
        wxStrncpy(buf, wxSafeConvertMB2WX(cbuf), sz - 1);
        buf[sz] = wxT('\0');
    }
#else // no uname, no gethostname
    wxFAIL_MSG(wxT("don't know host name for this machine"));

    bool ok = false;
#endif // uname/gethostname

    if ( !ok )
    {
        wxLogSysError(_("Cannot get the hostname"));
    }

    return ok;
}

bool wxGetHostName(wxChar *buf, int sz)
{
    bool ok = wxGetHostNameInternal(buf, sz);

    if ( ok )
    {
        // BSD systems return the FQDN, we only want the hostname, so extract
        // it (we consider that dots are domain separators)
        wxChar *dot = wxStrchr(buf, wxT('.'));
        if ( dot )
        {
            // nuke it
            *dot = wxT('\0');
        }
    }

    return ok;
}

bool wxGetFullHostName(wxChar *buf, int sz)
{
    bool ok = wxGetHostNameInternal(buf, sz);

    if ( ok )
    {
        if ( !wxStrchr(buf, wxT('.')) )
        {
            struct hostent *host = gethostbyname(wxSafeConvertWX2MB(buf));
            if ( !host )
            {
                wxLogSysError(_("Cannot get the official hostname"));

                ok = false;
            }
            else
            {
                // the canonical name
                wxStrncpy(buf, wxSafeConvertMB2WX(host->h_name), sz);
            }
        }
        //else: it's already a FQDN (BSD behaves this way)
    }

    return ok;
}

bool wxGetUserId(wxChar *buf, int sz)
{
    struct passwd *who;

    *buf = wxT('\0');
    if ((who = getpwuid(getuid ())) != NULL)
    {
        wxStrncpy (buf, wxSafeConvertMB2WX(who->pw_name), sz - 1);
        return true;
    }

    return false;
}

bool wxGetUserName(wxChar *buf, int sz)
{
#ifdef HAVE_PW_GECOS
    struct passwd *who;

    *buf = wxT('\0');
    if ((who = getpwuid (getuid ())) != NULL)
    {
       char *comma = strchr(who->pw_gecos, ',');
       if (comma)
           *comma = '\0'; // cut off non-name comment fields
       wxStrncpy (buf, wxSafeConvertMB2WX(who->pw_gecos), sz - 1);
       return true;
    }

    return false;
#else // !HAVE_PW_GECOS
    return wxGetUserId(buf, sz);
#endif // HAVE_PW_GECOS/!HAVE_PW_GECOS
}

bool wxIsPlatform64Bit()
{
    const wxString machine = wxGetCommandOutput(wxT("uname -m"));

    // the test for "64" is obviously not 100% reliable but seems to work fine
    // in practice
    return machine.Contains(wxT("64")) ||
                machine.Contains(wxT("alpha"));
}

// these functions are in mac/utils.cpp for wxMac
#ifndef __WXMAC__

wxOperatingSystemId wxGetOsVersion(int *verMaj, int *verMin)
{
    // get OS version
    int major, minor;
    wxString release = wxGetCommandOutput(wxT("uname -r"));
    if ( !release.empty() && wxSscanf(release, wxT("%d.%d"), &major, &minor) != 2 )
    {
        // unrecognized uname string format
        major =
        minor = -1;
    }

    if ( verMaj )
        *verMaj = major;
    if ( verMin )
        *verMin = minor;

    // try to understand which OS are we running
    wxString kernel = wxGetCommandOutput(wxT("uname -s"));
    if ( kernel.empty() )
        kernel = wxGetCommandOutput(wxT("uname -o"));

    if ( kernel.empty() )
        return wxOS_UNKNOWN;

    return wxPlatformInfo::GetOperatingSystemId(kernel);
}

wxString wxGetOsDescription()
{
    return wxGetCommandOutput(wxT("uname -s -r -m"));
}

#endif // !__WXMAC__

unsigned long wxGetProcessId()
{
    return (unsigned long)getpid();
}

wxMemorySize wxGetFreeMemory()
{
#if defined(__LINUX__)
    // get it from /proc/meminfo
    FILE *fp = fopen("/proc/meminfo", "r");
    if ( fp )
    {
        long memFree = -1;

        char buf[1024];
        if ( fgets(buf, WXSIZEOF(buf), fp) && fgets(buf, WXSIZEOF(buf), fp) )
        {
            // /proc/meminfo changed its format in kernel 2.6
            if ( wxPlatformInfo().CheckOSVersion(2, 6) )
            {
                unsigned long cached, buffers;
                sscanf(buf, "MemFree: %ld", &memFree);

                fgets(buf, WXSIZEOF(buf), fp);
                sscanf(buf, "Buffers: %lu", &buffers);

                fgets(buf, WXSIZEOF(buf), fp);
                sscanf(buf, "Cached: %lu", &cached);

                // add to "MemFree" also the "Buffers" and "Cached" values as
                // free(1) does as otherwise the value never makes sense: for
                // kernel 2.6 it's always almost 0
                memFree += buffers + cached;

                // values here are always expressed in kB and we want bytes
                memFree *= 1024;
            }
            else // Linux 2.4 (or < 2.6, anyhow)
            {
                long memTotal, memUsed;
                sscanf(buf, "Mem: %ld %ld %ld", &memTotal, &memUsed, &memFree);
            }
        }

        fclose(fp);

        return (wxMemorySize)memFree;
    }
#elif defined(__SUN__) && defined(_SC_AVPHYS_PAGES)
    return (wxMemorySize)(sysconf(_SC_AVPHYS_PAGES)*sysconf(_SC_PAGESIZE));
#elif defined(__SGI__)
    struct rminfo realmem;
    if ( sysmp(MP_SAGET, MPSA_RMINFO, &realmem, sizeof realmem) == 0 )
        return ((wxMemorySize)realmem.physmem * sysconf(_SC_PAGESIZE));
//#elif defined(__FREEBSD__) -- might use sysctl() to find it out, probably
#endif

    // can't find it out
    return -1;
}

bool wxGetDiskSpace(const wxString& path, wxDiskspaceSize_t *pTotal, wxDiskspaceSize_t *pFree)
{
#if defined(HAVE_STATFS) || defined(HAVE_STATVFS)
    // the case to "char *" is needed for AIX 4.3
    wxStatfs_t fs;
    if ( wxStatfs((char *)(const char*)path.fn_str(), &fs) != 0 )
    {
        wxLogSysError( wxT("Failed to get file system statistics") );

        return false;
    }

    // under Solaris we also have to use f_frsize field instead of f_bsize
    // which is in general a multiple of f_frsize
#ifdef HAVE_STATVFS
    wxDiskspaceSize_t blockSize = fs.f_frsize;
#else // HAVE_STATFS
    wxDiskspaceSize_t blockSize = fs.f_bsize;
#endif // HAVE_STATVFS/HAVE_STATFS

    if ( pTotal )
    {
        *pTotal = wxDiskspaceSize_t(fs.f_blocks) * blockSize;
    }

    if ( pFree )
    {
        *pFree = wxDiskspaceSize_t(fs.f_bavail) * blockSize;
    }

    return true;
#else // !HAVE_STATFS && !HAVE_STATVFS
    return false;
#endif // HAVE_STATFS
}

// ----------------------------------------------------------------------------
// env vars
// ----------------------------------------------------------------------------

bool wxGetEnv(const wxString& var, wxString *value)
{
    // wxGetenv is defined as getenv()
    wxChar *p = wxGetenv(var);
    if ( !p )
        return false;

    if ( value )
    {
        *value = p;
    }

    return true;
}

bool wxSetEnv(const wxString& variable, const wxChar *value)
{
#if defined(HAVE_SETENV)
    if ( !value )
    {
#ifdef HAVE_UNSETENV
        // don't test unsetenv() return value: it's void on some systems (at
        // least Darwin)
        unsetenv(variable.mb_str());
        return true;
#else
        value = _T(""); // we can't pass NULL to setenv()
#endif
    }

    return setenv(variable.mb_str(),
                  wxString(value).mb_str(),
                  1 /* overwrite */) == 0;
#elif defined(HAVE_PUTENV)
    wxString s = variable;
    if ( value )
        s << _T('=') << value;

    // transform to ANSI
    const wxWX2MBbuf p = s.mb_str();

    // the string will be free()d by libc
    char *buf = (char *)malloc(strlen(p) + 1);
    strcpy(buf, p);

    return putenv(buf) == 0;
#else // no way to set an env var
    return false;
#endif
}

// ----------------------------------------------------------------------------
// signal handling
// ----------------------------------------------------------------------------

#if wxUSE_ON_FATAL_EXCEPTION

#include <signal.h>

extern "C" void wxFatalSignalHandler(wxTYPE_SA_HANDLER)
{
    if ( wxTheApp )
    {
        // give the user a chance to do something special about this
        wxTheApp->OnFatalException();
    }

    abort();
}

bool wxHandleFatalExceptions(bool doit)
{
    // old sig handlers
    static bool s_savedHandlers = false;
    static struct sigaction s_handlerFPE,
                            s_handlerILL,
                            s_handlerBUS,
                            s_handlerSEGV;

    bool ok = true;
    if ( doit && !s_savedHandlers )
    {
        // install the signal handler
        struct sigaction act;

        // some systems extend it with non std fields, so zero everything
        memset(&act, 0, sizeof(act));

        act.sa_handler = wxFatalSignalHandler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

        ok &= sigaction(SIGFPE, &act, &s_handlerFPE) == 0;
        ok &= sigaction(SIGILL, &act, &s_handlerILL) == 0;
        ok &= sigaction(SIGBUS, &act, &s_handlerBUS) == 0;
        ok &= sigaction(SIGSEGV, &act, &s_handlerSEGV) == 0;
        if ( !ok )
        {
            wxLogDebug(_T("Failed to install our signal handler."));
        }

        s_savedHandlers = true;
    }
    else if ( s_savedHandlers )
    {
        // uninstall the signal handler
        ok &= sigaction(SIGFPE, &s_handlerFPE, NULL) == 0;
        ok &= sigaction(SIGILL, &s_handlerILL, NULL) == 0;
        ok &= sigaction(SIGBUS, &s_handlerBUS, NULL) == 0;
        ok &= sigaction(SIGSEGV, &s_handlerSEGV, NULL) == 0;
        if ( !ok )
        {
            wxLogDebug(_T("Failed to uninstall our signal handler."));
        }

        s_savedHandlers = false;
    }
    //else: nothing to do

    return ok;
}

#endif // wxUSE_ON_FATAL_EXCEPTION

#endif // wxUSE_BASE

#if wxUSE_GUI

#ifdef __DARWIN__
    #include <sys/errno.h>
#endif
// ----------------------------------------------------------------------------
// wxExecute support
// ----------------------------------------------------------------------------

/*
    NOTE: The original code shipped in 2.8 used __DARWIN__ && __WXMAC__ to wrap
    the wxGUIAppTraits differences but __DARWIN__ && (__WXMAC__ || __WXCOCOA__)
    to decide whether to call wxAddProcessCallbackForPid instead of
    wxAddProcessCallback.  This define normalizes things so the two match.

    Since wxCocoa was already creating the pipes in its wxGUIAppTraits I
    decided to leave that as is and implement wxAddProcessCallback in the
    utilsexec_cf.cpp file.  I didn't see a reason to wrap that in a __WXCOCOA__
    check since it's valid for both wxMac and wxCocoa.

    Since the existing code is working for wxMac I've left it as is although
    do note that the old task_for_pid method still used on PPC machines is
    expected to fail in Leopard PPC and theoretically already fails if you run
    your PPC app under Rosetta.

    You thus have two choices if you find end process detect broken:
     1) Change the define below such that the new code is used for wxMac.
        This is theoretically ABI compatible since the old code still remains
        in utilsexec_cf.cpp it's just no longer used by this code.
     2) Change the USE_POLLING define in utilsexc_cf.cpp to 1 unconditionally
        This is theoretically not compatible since it removes the
        wxMAC_MachPortEndProcessDetect helper function.  Though in practice
        this shouldn't be a problem since it wasn't prototyped anywhere.
 */
#define USE_OLD_DARWIN_END_PROCESS_DETECT (defined(__DARWIN__) && defined(__WXMAC__))
// #define USE_OLD_DARWIN_END_PROCESS_DETECT 0

// wxMac doesn't use the same process end detection mechanisms so we don't
// need wxExecute-related helpers for it.
#if !USE_OLD_DARWIN_END_PROCESS_DETECT

bool wxGUIAppTraits::CreateEndProcessPipe(wxExecuteData& execData)
{
    return execData.pipeEndProcDetect.Create();
}

bool wxGUIAppTraits::IsWriteFDOfEndProcessPipe(wxExecuteData& execData, int fd)
{
    return fd == (execData.pipeEndProcDetect)[wxPipe::Write];
}

void wxGUIAppTraits::DetachWriteFDOfEndProcessPipe(wxExecuteData& execData)
{
    execData.pipeEndProcDetect.Detach(wxPipe::Write);
    execData.pipeEndProcDetect.Close();
}

#else // !Darwin

bool wxGUIAppTraits::CreateEndProcessPipe(wxExecuteData& WXUNUSED(execData))
{
    return true;
}

bool
wxGUIAppTraits::IsWriteFDOfEndProcessPipe(wxExecuteData& WXUNUSED(execData),
                                          int WXUNUSED(fd))
{
    return false;
}

void
wxGUIAppTraits::DetachWriteFDOfEndProcessPipe(wxExecuteData& WXUNUSED(execData))
{
    // nothing to do here, we don't use the pipe
}

#endif // !Darwin/Darwin

int wxGUIAppTraits::WaitForChild(wxExecuteData& execData)
{
    wxEndProcessData *endProcData = new wxEndProcessData;

    const int flags = execData.flags;

    // wxAddProcessCallback is now (with DARWIN) allowed to call the
    // callback function directly if the process terminates before
    // the callback can be added to the run loop. Set up the endProcData.
    if ( flags & wxEXEC_SYNC )
    {
        // we may have process for capturing the program output, but it's
        // not used in wxEndProcessData in the case of sync execution
        endProcData->process = NULL;

        // sync execution: indicate it by negating the pid
        endProcData->pid = -execData.pid;
    }
    else
    {
        // async execution, nothing special to do -- caller will be
        // notified about the process termination if process != NULL, endProcData
        // will be deleted in GTK_EndProcessDetector
        endProcData->process  = execData.process;
        endProcData->pid      = execData.pid;
    }


#if USE_OLD_DARWIN_END_PROCESS_DETECT
    endProcData->tag = wxAddProcessCallbackForPid(endProcData, execData.pid);
#else
    endProcData->tag = wxAddProcessCallback
                (
                    endProcData,
                    execData.pipeEndProcDetect.Detach(wxPipe::Read)
                );

    execData.pipeEndProcDetect.Close();
#endif // USE_OLD_DARWIN_END_PROCESS_DETECT

    if ( flags & wxEXEC_SYNC )
    {
        wxBusyCursor bc;
        wxWindowDisabler *wd = flags & wxEXEC_NODISABLE ? NULL
                                                        : new wxWindowDisabler;

        // endProcData->pid will be set to 0 from GTK_EndProcessDetector when the
        // process terminates
        while ( endProcData->pid != 0 )
        {
            bool idle = true;

#if HAS_PIPE_INPUT_STREAM
            if ( execData.bufOut )
            {
                execData.bufOut->Update();
                idle = false;
            }

            if ( execData.bufErr )
            {
                execData.bufErr->Update();
                idle = false;
            }
#endif // HAS_PIPE_INPUT_STREAM

            // don't consume 100% of the CPU while we're sitting in this
            // loop
            if ( idle )
                wxMilliSleep(1);

            // give GTK+ a chance to call GTK_EndProcessDetector here and
            // also repaint the GUI
            wxYield();
        }

        int exitcode = endProcData->exitcode;

        delete wd;
        delete endProcData;

        return exitcode;
    }
    else // async execution
    {
        return execData.pid;
    }
}

#endif // wxUSE_GUI
#if wxUSE_BASE

void wxHandleProcessTermination(wxEndProcessData *proc_data)
{
    // notify user about termination if required
    if ( proc_data->process )
    {
        proc_data->process->OnTerminate(proc_data->pid, proc_data->exitcode);
    }

    // clean up
    if ( proc_data->pid > 0 )
    {
       delete proc_data;
    }
    else
    {
       // let wxExecute() know that the process has terminated
       proc_data->pid = 0;
    }
}

#endif // wxUSE_BASE
