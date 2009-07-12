/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/thread.cpp
// Purpose:     wxThread Implementation
// Author:      Original from Wolfram Gloger/Guilhem Lavaux
// Modified by: Vadim Zeitlin to make it work :-)
// Created:     04/22/98
// RCS-ID:      $Id: thread.cpp 57100 2008-12-04 00:22:04Z VZ $
// Copyright:   (c) Wolfram Gloger (1996, 1997), Guilhem Lavaux (1998);
//                  Vadim Zeitlin (1999-2002)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_THREADS

#include "wx/thread.h"

#ifndef WX_PRECOMP
    #include "wx/msw/missing.h"
    #include "wx/intl.h"
    #include "wx/app.h"
    #include "wx/module.h"
#endif

#include "wx/apptrait.h"
#include "wx/scopeguard.h"

#include "wx/msw/private.h"
#include "wx/msw/seh.h"

#include "wx/except.h"

// must have this symbol defined to get _beginthread/_endthread declarations
#ifndef _MT
    #define _MT
#endif

#if defined(__BORLANDC__)
    #if !defined(__MT__)
        // I can't set -tWM in the IDE (anyone?) so have to do this
        #define __MT__
    #endif

    #if !defined(__MFC_COMPAT__)
        // Needed to know about _beginthreadex etc..
        #define __MFC_COMPAT__
    #endif
#endif // BC++

// define wxUSE_BEGIN_THREAD if the compiler has _beginthreadex() function
// which should be used instead of Win32 ::CreateThread() if possible
#if defined(__VISUALC__) || \
    (defined(__BORLANDC__) && (__BORLANDC__ >= 0x500)) || \
    (defined(__GNUG__) && defined(__MSVCRT__)) || \
    defined(__WATCOMC__) || defined(__MWERKS__)

#ifndef __WXWINCE__
    #undef wxUSE_BEGIN_THREAD
    #define wxUSE_BEGIN_THREAD
#endif

#endif

#ifdef wxUSE_BEGIN_THREAD
    // this is where _beginthreadex() is declared
    #include <process.h>

    // the return type of the thread function entry point
    typedef unsigned THREAD_RETVAL;

    // the calling convention of the thread function entry point
    #define THREAD_CALLCONV __stdcall
#else
    // the settings for CreateThread()
    typedef DWORD THREAD_RETVAL;
    #define THREAD_CALLCONV WINAPI
#endif

// this is a hack to allow the code here to know whether wxEventLoop is
// currently running: as wxBase doesn't have event loop at all, we can't simple
// use "wxEventLoop::GetActive() != NULL" test, so instead wxEventLoop uses
// this variable to indicate whether it is active
//
// the proper solution is to use wxAppTraits to abstract the base/GUI-dependent
// operation of waiting for the thread to terminate and is already implemented
// in cvs HEAD, this is just a backwards compatible hack for 2.8
WXDLLIMPEXP_DATA_BASE(int) wxRunningEventLoopCount = 0;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the possible states of the thread ("=>" shows all possible transitions from
// this state)
enum wxThreadState
{
    STATE_NEW,          // didn't start execution yet (=> RUNNING)
    STATE_RUNNING,      // thread is running (=> PAUSED, CANCELED)
    STATE_PAUSED,       // thread is temporarily suspended (=> RUNNING)
    STATE_CANCELED,     // thread should terminate a.s.a.p. (=> EXITED)
    STATE_EXITED        // thread is terminating
};

// ----------------------------------------------------------------------------
// this module globals
// ----------------------------------------------------------------------------

// TLS index of the slot where we store the pointer to the current thread
static DWORD gs_tlsThisThread = 0xFFFFFFFF;

// id of the main thread - the one which can call GUI functions without first
// calling wxMutexGuiEnter()
static DWORD gs_idMainThread = 0;

// if it's false, some secondary thread is holding the GUI lock
static bool gs_bGuiOwnedByMainThread = true;

// critical section which controls access to all GUI functions: any secondary
// thread (i.e. except the main one) must enter this crit section before doing
// any GUI calls
static wxCriticalSection *gs_critsectGui = NULL;

// critical section which protects gs_nWaitingForGui variable
static wxCriticalSection *gs_critsectWaitingForGui = NULL;

// critical section which serializes WinThreadStart() and WaitForTerminate()
// (this is a potential bottleneck, we use a single crit sect for all threads
// in the system, but normally time spent inside it should be quite short)
static wxCriticalSection *gs_critsectThreadDelete = NULL;

// number of threads waiting for GUI in wxMutexGuiEnter()
static size_t gs_nWaitingForGui = 0;

// are we waiting for a thread termination?
static bool gs_waitingForThread = false;

// ============================================================================
// Windows implementation of thread and related classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxCriticalSection
// ----------------------------------------------------------------------------

wxCriticalSection::wxCriticalSection()
{
    wxCOMPILE_TIME_ASSERT( sizeof(CRITICAL_SECTION) <= sizeof(wxCritSectBuffer),
                           wxCriticalSectionBufferTooSmall );

    ::InitializeCriticalSection((CRITICAL_SECTION *)m_buffer);
}

wxCriticalSection::~wxCriticalSection()
{
    ::DeleteCriticalSection((CRITICAL_SECTION *)m_buffer);
}

void wxCriticalSection::Enter()
{
    ::EnterCriticalSection((CRITICAL_SECTION *)m_buffer);
}

void wxCriticalSection::Leave()
{
    ::LeaveCriticalSection((CRITICAL_SECTION *)m_buffer);
}

// ----------------------------------------------------------------------------
// wxMutex
// ----------------------------------------------------------------------------

class wxMutexInternal
{
public:
    wxMutexInternal(wxMutexType mutexType);
    ~wxMutexInternal();

    bool IsOk() const { return m_mutex != NULL; }

    wxMutexError Lock() { return LockTimeout(INFINITE); }
    wxMutexError TryLock() { return LockTimeout(0); }
    wxMutexError Unlock();

private:
    wxMutexError LockTimeout(DWORD milliseconds);

    HANDLE m_mutex;

    DECLARE_NO_COPY_CLASS(wxMutexInternal)
};

// all mutexes are recursive under Win32 so we don't use mutexType
wxMutexInternal::wxMutexInternal(wxMutexType WXUNUSED(mutexType))
{
    // create a nameless (hence intra process and always private) mutex
    m_mutex = ::CreateMutex
                (
                    NULL,       // default secutiry attributes
                    false,      // not initially locked
                    NULL        // no name
                );

    if ( !m_mutex )
    {
        wxLogLastError(_T("CreateMutex()"));
    }
}

wxMutexInternal::~wxMutexInternal()
{
    if ( m_mutex )
    {
        if ( !::CloseHandle(m_mutex) )
        {
            wxLogLastError(_T("CloseHandle(mutex)"));
        }
    }
}

wxMutexError wxMutexInternal::LockTimeout(DWORD milliseconds)
{
    DWORD rc = ::WaitForSingleObject(m_mutex, milliseconds);
    switch ( rc )
    {
        case WAIT_ABANDONED:
            // the previous caller died without releasing the mutex, so even
            // though we did get it, log a message about this
            wxLogDebug(_T("WaitForSingleObject() returned WAIT_ABANDONED"));
            // fall through

        case WAIT_OBJECT_0:
            // ok
            break;

        case WAIT_TIMEOUT:
            return wxMUTEX_BUSY;

        default:
            wxFAIL_MSG(wxT("impossible return value in wxMutex::Lock"));
            // fall through

        case WAIT_FAILED:
            wxLogLastError(_T("WaitForSingleObject(mutex)"));
            return wxMUTEX_MISC_ERROR;
    }

    return wxMUTEX_NO_ERROR;
}

wxMutexError wxMutexInternal::Unlock()
{
    if ( !::ReleaseMutex(m_mutex) )
    {
        wxLogLastError(_T("ReleaseMutex()"));

        return wxMUTEX_MISC_ERROR;
    }

    return wxMUTEX_NO_ERROR;
}

// --------------------------------------------------------------------------
// wxSemaphore
// --------------------------------------------------------------------------

// a trivial wrapper around Win32 semaphore
class wxSemaphoreInternal
{
public:
    wxSemaphoreInternal(int initialcount, int maxcount);
    ~wxSemaphoreInternal();

    bool IsOk() const { return m_semaphore != NULL; }

    wxSemaError Wait() { return WaitTimeout(INFINITE); }

    wxSemaError TryWait()
    {
        wxSemaError rc = WaitTimeout(0);
        if ( rc == wxSEMA_TIMEOUT )
            rc = wxSEMA_BUSY;

        return rc;
    }

    wxSemaError WaitTimeout(unsigned long milliseconds);

    wxSemaError Post();

private:
    HANDLE m_semaphore;

    DECLARE_NO_COPY_CLASS(wxSemaphoreInternal)
};

wxSemaphoreInternal::wxSemaphoreInternal(int initialcount, int maxcount)
{
#if !defined(_WIN32_WCE) || (_WIN32_WCE >= 300)
    if ( maxcount == 0 )
    {
        // make it practically infinite
        maxcount = INT_MAX;
    }

    m_semaphore = ::CreateSemaphore
                    (
                        NULL,           // default security attributes
                        initialcount,
                        maxcount,
                        NULL            // no name
                    );
#endif
    if ( !m_semaphore )
    {
        wxLogLastError(_T("CreateSemaphore()"));
    }
}

wxSemaphoreInternal::~wxSemaphoreInternal()
{
    if ( m_semaphore )
    {
        if ( !::CloseHandle(m_semaphore) )
        {
            wxLogLastError(_T("CloseHandle(semaphore)"));
        }
    }
}

wxSemaError wxSemaphoreInternal::WaitTimeout(unsigned long milliseconds)
{
    DWORD rc = ::WaitForSingleObject( m_semaphore, milliseconds );

    switch ( rc )
    {
        case WAIT_OBJECT_0:
           return wxSEMA_NO_ERROR;

        case WAIT_TIMEOUT:
           return wxSEMA_TIMEOUT;

        default:
            wxLogLastError(_T("WaitForSingleObject(semaphore)"));
    }

    return wxSEMA_MISC_ERROR;
}

wxSemaError wxSemaphoreInternal::Post()
{
#if !defined(_WIN32_WCE) || (_WIN32_WCE >= 300)
    if ( !::ReleaseSemaphore(m_semaphore, 1, NULL /* ptr to previous count */) )
    {
        if ( GetLastError() == ERROR_TOO_MANY_POSTS )
        {
            return wxSEMA_OVERFLOW;
        }
        else
        {
            wxLogLastError(_T("ReleaseSemaphore"));
            return wxSEMA_MISC_ERROR;
        }
    }

    return wxSEMA_NO_ERROR;
#else
    return wxSEMA_MISC_ERROR;
#endif
}

// ----------------------------------------------------------------------------
// wxThread implementation
// ----------------------------------------------------------------------------

// wxThreadInternal class
// ----------------------

class wxThreadInternal
{
public:
    wxThreadInternal(wxThread *thread)
    {
        m_thread = thread;
        m_hThread = 0;
        m_state = STATE_NEW;
        m_priority = WXTHREAD_DEFAULT_PRIORITY;
        m_nRef = 1;
    }

    ~wxThreadInternal()
    {
        Free();
    }

    void Free()
    {
        if ( m_hThread )
        {
            if ( !::CloseHandle(m_hThread) )
            {
                wxLogLastError(wxT("CloseHandle(thread)"));
            }

            m_hThread = 0;
        }
    }

    // create a new (suspended) thread (for the given thread object)
    bool Create(wxThread *thread, unsigned int stackSize);

    // wait for the thread to terminate, either by itself, or by asking it
    // (politely, this is not Kill()!) to do it
    wxThreadError WaitForTerminate(wxCriticalSection& cs,
                                   wxThread::ExitCode *pRc,
                                   wxThread *threadToDelete = NULL);

    // kill the thread unconditionally
    wxThreadError Kill();

    // suspend/resume/terminate
    bool Suspend();
    bool Resume();
    void Cancel() { m_state = STATE_CANCELED; }

    // thread state
    void SetState(wxThreadState state) { m_state = state; }
    wxThreadState GetState() const { return m_state; }

    // thread priority
    void SetPriority(unsigned int priority);
    unsigned int GetPriority() const { return m_priority; }

    // thread handle and id
    HANDLE GetHandle() const { return m_hThread; }
    DWORD  GetId() const { return m_tid; }

    // the thread function forwarding to DoThreadStart
    static THREAD_RETVAL THREAD_CALLCONV WinThreadStart(void *thread);

    // really start the thread (if it's not already dead)
    static THREAD_RETVAL DoThreadStart(wxThread *thread);

    // call OnExit() on the thread
    static void DoThreadOnExit(wxThread *thread);


    void KeepAlive()
    {
        if ( m_thread->IsDetached() )
            ::InterlockedIncrement(&m_nRef);
    }

    void LetDie()
    {
        if ( m_thread->IsDetached() && !::InterlockedDecrement(&m_nRef) )
            delete m_thread;
    }

private:
    // the thread we're associated with
    wxThread *m_thread;

    HANDLE        m_hThread;    // handle of the thread
    wxThreadState m_state;      // state, see wxThreadState enum
    unsigned int  m_priority;   // thread priority in "wx" units
    DWORD         m_tid;        // thread id

    // number of threads which need this thread to remain alive, when the count
    // reaches 0 we kill the owning wxThread -- and die ourselves with it
    LONG m_nRef;

    DECLARE_NO_COPY_CLASS(wxThreadInternal)
};

// small class which keeps a thread alive during its lifetime
class wxThreadKeepAlive
{
public:
    wxThreadKeepAlive(wxThreadInternal& thrImpl) : m_thrImpl(thrImpl)
        { m_thrImpl.KeepAlive(); }
    ~wxThreadKeepAlive()
        { m_thrImpl.LetDie(); }

private:
    wxThreadInternal& m_thrImpl;
};

/* static */
void wxThreadInternal::DoThreadOnExit(wxThread *thread)
{
    wxTRY
    {
        thread->OnExit();
    }
    wxCATCH_ALL( wxTheApp->OnUnhandledException(); )
}

/* static */
THREAD_RETVAL wxThreadInternal::DoThreadStart(wxThread *thread)
{
    wxON_BLOCK_EXIT1(DoThreadOnExit, thread);

    THREAD_RETVAL rc = (THREAD_RETVAL)-1;

    wxTRY
    {
        // store the thread object in the TLS
        if ( !::TlsSetValue(gs_tlsThisThread, thread) )
        {
            wxLogSysError(_("Can not start thread: error writing TLS."));

            return (THREAD_RETVAL)-1;
        }

        rc = (THREAD_RETVAL)thread->Entry();
    }
    wxCATCH_ALL( wxTheApp->OnUnhandledException(); )

    return rc;
}

/* static */
THREAD_RETVAL THREAD_CALLCONV wxThreadInternal::WinThreadStart(void *param)
{
    THREAD_RETVAL rc = (THREAD_RETVAL)-1;

    wxThread * const thread = (wxThread *)param;

    // each thread has its own SEH translator so install our own a.s.a.p.
    DisableAutomaticSETranslator();

    // first of all, check whether we hadn't been cancelled already and don't
    // start the user code at all then
    const bool hasExited = thread->m_internal->GetState() == STATE_EXITED;

    // run the thread function itself inside a SEH try/except block
    wxSEH_TRY
    {
        if ( hasExited )
            DoThreadOnExit(thread);
        else
            rc = DoThreadStart(thread);
    }
    wxSEH_HANDLE((THREAD_RETVAL)-1)


    // save IsDetached because thread object can be deleted by joinable
    // threads after state is changed to STATE_EXITED.
    const bool isDetached = thread->IsDetached();
    if ( !hasExited )
    {
        // enter m_critsect before changing the thread state
        //
        // NB: can't use wxCriticalSectionLocker here as we use SEH and it's
        //     incompatible with C++ object dtors
        thread->m_critsect.Enter();
        thread->m_internal->SetState(STATE_EXITED);
        thread->m_critsect.Leave();
    }

    // the thread may delete itself now if it wants, we don't need it any more
    if ( isDetached )
        thread->m_internal->LetDie();

    return rc;
}

void wxThreadInternal::SetPriority(unsigned int priority)
{
    m_priority = priority;

    // translate wxWidgets priority to the Windows one
    int win_priority;
    if (m_priority <= 20)
        win_priority = THREAD_PRIORITY_LOWEST;
    else if (m_priority <= 40)
        win_priority = THREAD_PRIORITY_BELOW_NORMAL;
    else if (m_priority <= 60)
        win_priority = THREAD_PRIORITY_NORMAL;
    else if (m_priority <= 80)
        win_priority = THREAD_PRIORITY_ABOVE_NORMAL;
    else if (m_priority <= 100)
        win_priority = THREAD_PRIORITY_HIGHEST;
    else
    {
        wxFAIL_MSG(wxT("invalid value of thread priority parameter"));
        win_priority = THREAD_PRIORITY_NORMAL;
    }

    if ( !::SetThreadPriority(m_hThread, win_priority) )
    {
        wxLogSysError(_("Can't set thread priority"));
    }
}

bool wxThreadInternal::Create(wxThread *thread, unsigned int stackSize)
{
    wxASSERT_MSG( m_state == STATE_NEW && !m_hThread,
                    _T("Create()ing thread twice?") );

    // for compilers which have it, we should use C RTL function for thread
    // creation instead of Win32 API one because otherwise we will have memory
    // leaks if the thread uses C RTL (and most threads do)
#ifdef wxUSE_BEGIN_THREAD

    // Watcom is reported to not like 0 stack size (which means "use default"
    // for the other compilers and is also the default value for stackSize)
#ifdef __WATCOMC__
    if ( !stackSize )
        stackSize = 10240;
#endif // __WATCOMC__

    m_hThread = (HANDLE)_beginthreadex
                        (
                          NULL,                             // default security
                          stackSize,
                          wxThreadInternal::WinThreadStart, // entry point
                          thread,
                          CREATE_SUSPENDED,
                          (unsigned int *)&m_tid
                        );
#else // compiler doesn't have _beginthreadex
    m_hThread = ::CreateThread
                  (
                    NULL,                               // default security
                    stackSize,                          // stack size
                    wxThreadInternal::WinThreadStart,   // thread entry point
                    (LPVOID)thread,                     // parameter
                    CREATE_SUSPENDED,                   // flags
                    &m_tid                              // [out] thread id
                  );
#endif // _beginthreadex/CreateThread

    if ( m_hThread == NULL )
    {
        wxLogSysError(_("Can't create thread"));

        return false;
    }

    if ( m_priority != WXTHREAD_DEFAULT_PRIORITY )
    {
        SetPriority(m_priority);
    }

    return true;
}

wxThreadError wxThreadInternal::Kill()
{
    if ( !::TerminateThread(m_hThread, (DWORD)-1) )
    {
        wxLogSysError(_("Couldn't terminate thread"));

        return wxTHREAD_MISC_ERROR;
    }

    Free();

    return wxTHREAD_NO_ERROR;
}

wxThreadError
wxThreadInternal::WaitForTerminate(wxCriticalSection& cs,
                                   wxThread::ExitCode *pRc,
                                   wxThread *threadToDelete)
{
    // prevent the thread C++ object from disappearing as long as we are using
    // it here
    wxThreadKeepAlive keepAlive(*this);


    // we may either wait passively for the thread to terminate (when called
    // from Wait()) or ask it to terminate (when called from Delete())
    bool shouldDelete = threadToDelete != NULL;

    wxThread::ExitCode rc = 0;

    // we might need to resume the thread if it's currently stopped
    bool shouldResume = false;

    // as Delete() (which calls us) is always safe to call we need to consider
    // all possible states
    {
        wxCriticalSectionLocker lock(cs);

        if ( m_state == STATE_NEW )
        {
            if ( shouldDelete )
            {
                // WinThreadStart() will see it and terminate immediately, no
                // need to cancel the thread -- but we still need to resume it
                // to let it run
                m_state = STATE_EXITED;

                // we must call Resume() as the thread hasn't been initially
                // resumed yet (and as Resume() it knows about STATE_EXITED
                // special case, it won't touch it and WinThreadStart() will
                // just exit immediately)
                shouldResume = true;
                shouldDelete = false;
            }
            //else: shouldResume is correctly set to false here, wait until
            //      someone else runs the thread and it finishes
        }
        else // running, paused, cancelled or even exited
        {
            shouldResume = m_state == STATE_PAUSED;
        }
    }

    // resume the thread if it is paused
    if ( shouldResume )
        Resume();

    // ask the thread to terminate
    if ( shouldDelete )
    {
        wxCriticalSectionLocker lock(cs);

        Cancel();
    }


    // now wait for thread to finish
    if ( wxThread::IsMain() )
    {
        // set flag for wxIsWaitingForThread()
        gs_waitingForThread = true;
    }

    // we can't just wait for the thread to terminate because it might be
    // calling some GUI functions and so it will never terminate before we
    // process the Windows messages that result from these functions
    // (note that even in console applications we might have to process
    // messages if we use wxExecute() or timers or ...)
    DWORD result wxDUMMY_INITIALIZE(0);
    do
    {
        if ( wxThread::IsMain() )
        {
            // give the thread we're waiting for chance to do the GUI call
            // it might be in
            if ( (gs_nWaitingForGui > 0) && wxGuiOwnedByMainThread() )
            {
                wxMutexGuiLeave();
            }
        }

#if !defined(QS_ALLPOSTMESSAGE)
#define QS_ALLPOSTMESSAGE 0
#endif
        if ( !wxRunningEventLoopCount )
        {
            // don't ask for Windows messages if we don't have a running event
            // loop to process them as otherwise we'd enter an infinite loop
            // with MsgWaitForMultipleObjects() always returning WAIT_OBJECT_0
            // + 1 because the message would remain forever in the queue
            result = ::WaitForSingleObject(m_hThread, INFINITE);
        }
        else // wait for thread termination without blocking the GUI
        {
            result = ::MsgWaitForMultipleObjects
                     (
                       1,              // number of objects to wait for
                       &m_hThread,     // the objects
                       false,          // don't wait for all objects
                       INFINITE,       // no timeout
                       QS_ALLINPUT |   // return as soon as there are any events
                       QS_ALLPOSTMESSAGE
                     );
        }

        switch ( result )
        {
            case 0xFFFFFFFF:
                // error
                wxLogSysError(_("Can not wait for thread termination"));
                Kill();
                return wxTHREAD_KILLED;

            case WAIT_OBJECT_0:
                // thread we're waiting for terminated
                break;

            case WAIT_OBJECT_0 + 1:
                // new message arrived, process it -- but only if we're the
                // main thread as we don't support processing messages in
                // the other ones
                //
                // NB: we still must include QS_ALLINPUT even when waiting
                //     in a secondary thread because if it had created some
                //     window somehow (possible not even using wxWidgets)
                //     the system might dead lock then
                if ( wxThread::IsMain() )
                {
                    wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits()
                                                   : NULL;

                    if ( traits && !traits->DoMessageFromThreadWait() )
                    {
                        // WM_QUIT received: kill the thread
                        Kill();

                        return wxTHREAD_KILLED;
                    }
                }
                break;

            default:
                wxFAIL_MSG(wxT("unexpected result of MsgWaitForMultipleObject"));
        }
    } while ( result != WAIT_OBJECT_0 );

    if ( wxThread::IsMain() )
    {
        gs_waitingForThread = false;
    }


    // although the thread might be already in the EXITED state it might not
    // have terminated yet and so we are not sure that it has actually
    // terminated if the "if" above hadn't been taken
    for ( ;; )
    {
        if ( !::GetExitCodeThread(m_hThread, (LPDWORD)&rc) )
        {
            wxLogLastError(wxT("GetExitCodeThread"));

            rc = (wxThread::ExitCode)-1;

            break;
        }

        if ( (DWORD)rc != STILL_ACTIVE )
            break;

        // give the other thread some time to terminate, otherwise we may be
        // starving it
        ::Sleep(1);
    }

    if ( pRc )
        *pRc = rc;

    // we don't need the thread handle any more in any case
    Free();


    return rc == (wxThread::ExitCode)-1 ? wxTHREAD_MISC_ERROR
                                        : wxTHREAD_NO_ERROR;
}

bool wxThreadInternal::Suspend()
{
    DWORD nSuspendCount = ::SuspendThread(m_hThread);
    if ( nSuspendCount == (DWORD)-1 )
    {
        wxLogSysError(_("Can not suspend thread %x"), m_hThread);

        return false;
    }

    m_state = STATE_PAUSED;

    return true;
}

bool wxThreadInternal::Resume()
{
    DWORD nSuspendCount = ::ResumeThread(m_hThread);
    if ( nSuspendCount == (DWORD)-1 )
    {
        wxLogSysError(_("Can not resume thread %x"), m_hThread);

        return false;
    }

    // don't change the state from STATE_EXITED because it's special and means
    // we are going to terminate without running any user code - if we did it,
    // the code in WaitForTerminate() wouldn't work
    if ( m_state != STATE_EXITED )
    {
        m_state = STATE_RUNNING;
    }

    return true;
}

// static functions
// ----------------

wxThread *wxThread::This()
{
    wxThread *thread = (wxThread *)::TlsGetValue(gs_tlsThisThread);

    // be careful, 0 may be a valid return value as well
    if ( !thread && (::GetLastError() != NO_ERROR) )
    {
        wxLogSysError(_("Couldn't get the current thread pointer"));

        // return NULL...
    }

    return thread;
}

bool wxThread::IsMain()
{
    return ::GetCurrentThreadId() == gs_idMainThread || gs_idMainThread == 0;
}

void wxThread::Yield()
{
    // 0 argument to Sleep() is special and means to just give away the rest of
    // our timeslice
    ::Sleep(0);
}

void wxThread::Sleep(unsigned long milliseconds)
{
    ::Sleep(milliseconds);
}

int wxThread::GetCPUCount()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    return si.dwNumberOfProcessors;
}

unsigned long wxThread::GetCurrentId()
{
    return (unsigned long)::GetCurrentThreadId();
}

bool wxThread::SetConcurrency(size_t WXUNUSED_IN_WINCE(level))
{
#ifdef __WXWINCE__
    return false;
#else
    wxASSERT_MSG( IsMain(), _T("should only be called from the main thread") );

    // ok only for the default one
    if ( level == 0 )
        return 0;

    // get system affinity mask first
    HANDLE hProcess = ::GetCurrentProcess();
    DWORD_PTR dwProcMask, dwSysMask;
    if ( ::GetProcessAffinityMask(hProcess, &dwProcMask, &dwSysMask) == 0 )
    {
        wxLogLastError(_T("GetProcessAffinityMask"));

        return false;
    }

    // how many CPUs have we got?
    if ( dwSysMask == 1 )
    {
        // don't bother with all this complicated stuff - on a single
        // processor system it doesn't make much sense anyhow
        return level == 1;
    }

    // calculate the process mask: it's a bit vector with one bit per
    // processor; we want to schedule the process to run on first level
    // CPUs
    DWORD bit = 1;
    while ( bit )
    {
        if ( dwSysMask & bit )
        {
            // ok, we can set this bit
            dwProcMask |= bit;

            // another process added
            if ( --level == 0 )
            {
                // and that's enough
                break;
            }
        }

        // next bit
        bit <<= 1;
    }

    // could we set all bits?
    if ( level != 0 )
    {
        wxLogDebug(_T("bad level %u in wxThread::SetConcurrency()"), level);

        return false;
    }

    // set it: we can't link to SetProcessAffinityMask() because it doesn't
    // exist in Win9x, use RT binding instead

    typedef BOOL (WINAPI *SETPROCESSAFFINITYMASK)(HANDLE, DWORD_PTR);

    // can use static var because we're always in the main thread here
    static SETPROCESSAFFINITYMASK pfnSetProcessAffinityMask = NULL;

    if ( !pfnSetProcessAffinityMask )
    {
        HMODULE hModKernel = ::LoadLibrary(_T("kernel32"));
        if ( hModKernel )
        {
            pfnSetProcessAffinityMask = (SETPROCESSAFFINITYMASK)
                ::GetProcAddress(hModKernel, "SetProcessAffinityMask");
        }

        // we've discovered a MT version of Win9x!
        wxASSERT_MSG( pfnSetProcessAffinityMask,
                      _T("this system has several CPUs but no SetProcessAffinityMask function?") );
    }

    if ( !pfnSetProcessAffinityMask )
    {
        // msg given above - do it only once
        return false;
    }

    if ( pfnSetProcessAffinityMask(hProcess, dwProcMask) == 0 )
    {
        wxLogLastError(_T("SetProcessAffinityMask"));

        return false;
    }

    return true;
#endif // __WXWINCE__/!__WXWINCE__
}

// ctor and dtor
// -------------

wxThread::wxThread(wxThreadKind kind)
{
    m_internal = new wxThreadInternal(this);

    m_isDetached = kind == wxTHREAD_DETACHED;
}

wxThread::~wxThread()
{
    delete m_internal;
}

// create/start thread
// -------------------

wxThreadError wxThread::Create(unsigned int stackSize)
{
    wxCriticalSectionLocker lock(m_critsect);

    if ( !m_internal->Create(this, stackSize) )
        return wxTHREAD_NO_RESOURCE;

    return wxTHREAD_NO_ERROR;
}

wxThreadError wxThread::Run()
{
    wxCriticalSectionLocker lock(m_critsect);

    if ( m_internal->GetState() != STATE_NEW )
    {
        // actually, it may be almost any state at all, not only STATE_RUNNING
        return wxTHREAD_RUNNING;
    }

    // the thread has just been created and is still suspended - let it run
    return Resume();
}

// suspend/resume thread
// ---------------------

wxThreadError wxThread::Pause()
{
    wxCriticalSectionLocker lock(m_critsect);

    return m_internal->Suspend() ? wxTHREAD_NO_ERROR : wxTHREAD_MISC_ERROR;
}

wxThreadError wxThread::Resume()
{
    wxCriticalSectionLocker lock(m_critsect);

    return m_internal->Resume() ? wxTHREAD_NO_ERROR : wxTHREAD_MISC_ERROR;
}

// stopping thread
// ---------------

wxThread::ExitCode wxThread::Wait()
{
    // although under Windows we can wait for any thread, it's an error to
    // wait for a detached one in wxWin API
    wxCHECK_MSG( !IsDetached(), (ExitCode)-1,
                 _T("wxThread::Wait(): can't wait for detached thread") );

    ExitCode rc = (ExitCode)-1;

    (void)m_internal->WaitForTerminate(m_critsect, &rc);

    return rc;
}

wxThreadError wxThread::Delete(ExitCode *pRc)
{
    return m_internal->WaitForTerminate(m_critsect, pRc, this);
}

wxThreadError wxThread::Kill()
{
    if ( !IsRunning() )
        return wxTHREAD_NOT_RUNNING;

    wxThreadError rc = m_internal->Kill();

    if ( IsDetached() )
    {
        delete this;
    }
    else // joinable
    {
        // update the status of the joinable thread
        wxCriticalSectionLocker lock(m_critsect);
        m_internal->SetState(STATE_EXITED);
    }

    return rc;
}

void wxThread::Exit(ExitCode status)
{
    m_internal->Free();

    if ( IsDetached() )
    {
        delete this;
    }
    else // joinable
    {
        // update the status of the joinable thread
        wxCriticalSectionLocker lock(m_critsect);
        m_internal->SetState(STATE_EXITED);
    }

#ifdef wxUSE_BEGIN_THREAD
    _endthreadex((unsigned)status);
#else // !VC++
    ::ExitThread((DWORD)status);
#endif // VC++/!VC++

    wxFAIL_MSG(wxT("Couldn't return from ExitThread()!"));
}

// priority setting
// ----------------

void wxThread::SetPriority(unsigned int prio)
{
    wxCriticalSectionLocker lock(m_critsect);

    m_internal->SetPriority(prio);
}

unsigned int wxThread::GetPriority() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return m_internal->GetPriority();
}

unsigned long wxThread::GetId() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return (unsigned long)m_internal->GetId();
}

bool wxThread::IsRunning() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return m_internal->GetState() == STATE_RUNNING;
}

bool wxThread::IsAlive() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return (m_internal->GetState() == STATE_RUNNING) ||
           (m_internal->GetState() == STATE_PAUSED);
}

bool wxThread::IsPaused() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return m_internal->GetState() == STATE_PAUSED;
}

bool wxThread::TestDestroy()
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect); // const_cast

    return m_internal->GetState() == STATE_CANCELED;
}

// ----------------------------------------------------------------------------
// Automatic initialization for thread module
// ----------------------------------------------------------------------------

class wxThreadModule : public wxModule
{
public:
    virtual bool OnInit();
    virtual void OnExit();

private:
    DECLARE_DYNAMIC_CLASS(wxThreadModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxThreadModule, wxModule)

bool wxThreadModule::OnInit()
{
    // allocate TLS index for storing the pointer to the current thread
    gs_tlsThisThread = ::TlsAlloc();
    if ( gs_tlsThisThread == 0xFFFFFFFF )
    {
        // in normal circumstances it will only happen if all other
        // TLS_MINIMUM_AVAILABLE (>= 64) indices are already taken - in other
        // words, this should never happen
        wxLogSysError(_("Thread module initialization failed: impossible to allocate index in thread local storage"));

        return false;
    }

    // main thread doesn't have associated wxThread object, so store 0 in the
    // TLS instead
    if ( !::TlsSetValue(gs_tlsThisThread, (LPVOID)0) )
    {
        ::TlsFree(gs_tlsThisThread);
        gs_tlsThisThread = 0xFFFFFFFF;

        wxLogSysError(_("Thread module initialization failed: can not store value in thread local storage"));

        return false;
    }

    gs_critsectWaitingForGui = new wxCriticalSection();

    gs_critsectGui = new wxCriticalSection();
    gs_critsectGui->Enter();

    gs_critsectThreadDelete = new wxCriticalSection;

    // no error return for GetCurrentThreadId()
    gs_idMainThread = ::GetCurrentThreadId();

    return true;
}

void wxThreadModule::OnExit()
{
    if ( !::TlsFree(gs_tlsThisThread) )
    {
        wxLogLastError(wxT("TlsFree failed."));
    }

    delete gs_critsectThreadDelete;
    gs_critsectThreadDelete = NULL;

    if ( gs_critsectGui )
    {
        gs_critsectGui->Leave();
        delete gs_critsectGui;
        gs_critsectGui = NULL;
    }

    delete gs_critsectWaitingForGui;
    gs_critsectWaitingForGui = NULL;
}

// ----------------------------------------------------------------------------
// under Windows, these functions are implemented using a critical section and
// not a mutex, so the names are a bit confusing
// ----------------------------------------------------------------------------

void WXDLLIMPEXP_BASE wxMutexGuiEnter()
{
    // this would dead lock everything...
    wxASSERT_MSG( !wxThread::IsMain(),
                  wxT("main thread doesn't want to block in wxMutexGuiEnter()!") );

    // the order in which we enter the critical sections here is crucial!!

    // set the flag telling to the main thread that we want to do some GUI
    {
        wxCriticalSectionLocker enter(*gs_critsectWaitingForGui);

        gs_nWaitingForGui++;
    }

    wxWakeUpMainThread();

    // now we may block here because the main thread will soon let us in
    // (during the next iteration of OnIdle())
    gs_critsectGui->Enter();
}

void WXDLLIMPEXP_BASE wxMutexGuiLeave()
{
    wxCriticalSectionLocker enter(*gs_critsectWaitingForGui);

    if ( wxThread::IsMain() )
    {
        gs_bGuiOwnedByMainThread = false;
    }
    else
    {
        // decrement the number of threads waiting for GUI access now
        wxASSERT_MSG( gs_nWaitingForGui > 0,
                      wxT("calling wxMutexGuiLeave() without entering it first?") );

        gs_nWaitingForGui--;

        wxWakeUpMainThread();
    }

    gs_critsectGui->Leave();
}

void WXDLLIMPEXP_BASE wxMutexGuiLeaveOrEnter()
{
    wxASSERT_MSG( wxThread::IsMain(),
                  wxT("only main thread may call wxMutexGuiLeaveOrEnter()!") );

    wxCriticalSectionLocker enter(*gs_critsectWaitingForGui);

    if ( gs_nWaitingForGui == 0 )
    {
        // no threads are waiting for GUI - so we may acquire the lock without
        // any danger (but only if we don't already have it)
        if ( !wxGuiOwnedByMainThread() )
        {
            gs_critsectGui->Enter();

            gs_bGuiOwnedByMainThread = true;
        }
        //else: already have it, nothing to do
    }
    else
    {
        // some threads are waiting, release the GUI lock if we have it
        if ( wxGuiOwnedByMainThread() )
        {
            wxMutexGuiLeave();
        }
        //else: some other worker thread is doing GUI
    }
}

bool WXDLLIMPEXP_BASE wxGuiOwnedByMainThread()
{
    return gs_bGuiOwnedByMainThread;
}

// wake up the main thread if it's in ::GetMessage()
void WXDLLIMPEXP_BASE wxWakeUpMainThread()
{
    // sending any message would do - hopefully WM_NULL is harmless enough
    if ( !::PostThreadMessage(gs_idMainThread, WM_NULL, 0, 0) )
    {
        // should never happen
        wxLogLastError(wxT("PostThreadMessage(WM_NULL)"));
    }
}

bool WXDLLIMPEXP_BASE wxIsWaitingForThread()
{
    return gs_waitingForThread;
}

// ----------------------------------------------------------------------------
// include common implementation code
// ----------------------------------------------------------------------------

#include "wx/thrimpl.cpp"

#endif // wxUSE_THREADS
