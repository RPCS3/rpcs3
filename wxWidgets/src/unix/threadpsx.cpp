/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/threadpsx.cpp
// Purpose:     wxThread (Posix) Implementation
// Author:      Original from Wolfram Gloger/Guilhem Lavaux
// Modified by: K. S. Sreeram (2002): POSIXified wxCondition, added wxSemaphore
// Created:     04/22/98
// RCS-ID:      $Id: threadpsx.cpp 48537 2007-09-03 22:52:38Z VZ $
// Copyright:   (c) Wolfram Gloger (1996, 1997)
//                  Guilhem Lavaux (1998)
//                  Vadim Zeitlin (1999-2002)
//                  Robert Roebling (1999)
//                  K. S. Sreeram (2002)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declaration
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_THREADS

#include "wx/thread.h"

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
    #include "wx/timer.h"
    #include "wx/stopwatch.h"
    #include "wx/module.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_SCHED_H
    #include <sched.h>
#endif

#ifdef HAVE_THR_SETCONCURRENCY
    #include <thread.h>
#endif

// we use wxFFile under Linux in GetCPUCount()
#ifdef __LINUX__
    #include "wx/ffile.h"
    // For setpriority.
    #include <sys/time.h>
    #include <sys/resource.h>
#endif

#ifdef __VMS
    #define THR_ID(thr) ((long long)(thr)->GetId())
#else
    #define THR_ID(thr) ((long)(thr)->GetId())
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the possible states of the thread and transitions from them
enum wxThreadState
{
    STATE_NEW,          // didn't start execution yet (=> RUNNING)
    STATE_RUNNING,      // running (=> PAUSED or EXITED)
    STATE_PAUSED,       // suspended (=> RUNNING or EXITED)
    STATE_EXITED        // thread doesn't exist any more
};

// the exit value of a thread which has been cancelled
static const wxThread::ExitCode EXITCODE_CANCELLED = (wxThread::ExitCode)-1;

// trace mask for wxThread operations
#define TRACE_THREADS   _T("thread")

// you can get additional debugging messages for the semaphore operations
#define TRACE_SEMA      _T("semaphore")

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static void ScheduleThreadForDeletion();
static void DeleteThread(wxThread *This);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// an (non owning) array of pointers to threads
WX_DEFINE_ARRAY_PTR(wxThread *, wxArrayThread);

// an entry for a thread we can wait for

// -----------------------------------------------------------------------------
// global data
// -----------------------------------------------------------------------------

// we keep the list of all threads created by the application to be able to
// terminate them on exit if there are some left - otherwise the process would
// be left in memory
static wxArrayThread gs_allThreads;

// a mutex to protect gs_allThreads
static wxMutex *gs_mutexAllThreads = NULL;

// the id of the main thread
static pthread_t gs_tidMain = (pthread_t)-1;

// the key for the pointer to the associated wxThread object
static pthread_key_t gs_keySelf;

// the number of threads which are being deleted - the program won't exit
// until there are any left
static size_t gs_nThreadsBeingDeleted = 0;

// a mutex to protect gs_nThreadsBeingDeleted
static wxMutex *gs_mutexDeleteThread = (wxMutex *)NULL;

// and a condition variable which will be signaled when all
// gs_nThreadsBeingDeleted will have been deleted
static wxCondition *gs_condAllDeleted = (wxCondition *)NULL;

// this mutex must be acquired before any call to a GUI function
// (it's not inside #if wxUSE_GUI because this file is compiled as part
// of wxBase)
static wxMutex *gs_mutexGui = NULL;

// when we wait for a thread to exit, we're blocking on a condition which the
// thread signals in its SignalExit() method -- but this condition can't be a
// member of the thread itself as a detached thread may delete itself at any
// moment and accessing the condition member of the thread after this would
// result in a disaster
//
// so instead we maintain a global list of the structs below for the threads
// we're interested in waiting on

// ============================================================================
// wxMutex implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMutexInternal
// ----------------------------------------------------------------------------

// this is a simple wrapper around pthread_mutex_t which provides error
// checking
class wxMutexInternal
{
public:
    wxMutexInternal(wxMutexType mutexType);
    ~wxMutexInternal();

    wxMutexError Lock();
    wxMutexError TryLock();
    wxMutexError Unlock();

    bool IsOk() const { return m_isOk; }

private:
    pthread_mutex_t m_mutex;
    bool m_isOk;

    // wxConditionInternal uses our m_mutex
    friend class wxConditionInternal;
};

#if defined(HAVE_PTHREAD_MUTEXATTR_T) && \
        wxUSE_UNIX && !defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE_DECL)
// on some systems pthread_mutexattr_settype() is not in the headers (but it is
// in the library, otherwise we wouldn't compile this code at all)
extern "C" int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
#endif

wxMutexInternal::wxMutexInternal(wxMutexType mutexType)
{
    int err;
    switch ( mutexType )
    {
        case wxMUTEX_RECURSIVE:
            // support recursive locks like Win32, i.e. a thread can lock a
            // mutex which it had itself already locked
            //
            // unfortunately initialization of recursive mutexes is non
            // portable, so try several methods
#ifdef HAVE_PTHREAD_MUTEXATTR_T
            {
                pthread_mutexattr_t attr;
                pthread_mutexattr_init(&attr);
                pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

                err = pthread_mutex_init(&m_mutex, &attr);
            }
#elif defined(HAVE_PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
            // we can use this only as initializer so we have to assign it
            // first to a temp var - assigning directly to m_mutex wouldn't
            // even compile
            {
                pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
                m_mutex = mutex;
            }
#else // no recursive mutexes
            err = EINVAL;
#endif // HAVE_PTHREAD_MUTEXATTR_T/...
            break;

        default:
            wxFAIL_MSG( _T("unknown mutex type") );
            // fall through

        case wxMUTEX_DEFAULT:
            err = pthread_mutex_init(&m_mutex, NULL);
            break;
    }

    m_isOk = err == 0;
    if ( !m_isOk )
    {
        wxLogApiError( wxT("pthread_mutex_init()"), err);
    }
}

wxMutexInternal::~wxMutexInternal()
{
    if ( m_isOk )
    {
        int err = pthread_mutex_destroy(&m_mutex);
        if ( err != 0 )
        {
            wxLogApiError( wxT("pthread_mutex_destroy()"), err);
        }
    }
}

wxMutexError wxMutexInternal::Lock()
{
    int err = pthread_mutex_lock(&m_mutex);
    switch ( err )
    {
        case EDEADLK:
            // only error checking mutexes return this value and so it's an
            // unexpected situation -- hence use assert, not wxLogDebug
            wxFAIL_MSG( _T("mutex deadlock prevented") );
            return wxMUTEX_DEAD_LOCK;

        case EINVAL:
            wxLogDebug(_T("pthread_mutex_lock(): mutex not initialized."));
            break;

        case 0:
            return wxMUTEX_NO_ERROR;

        default:
            wxLogApiError(_T("pthread_mutex_lock()"), err);
    }

    return wxMUTEX_MISC_ERROR;
}

wxMutexError wxMutexInternal::TryLock()
{
    int err = pthread_mutex_trylock(&m_mutex);
    switch ( err )
    {
        case EBUSY:
            // not an error: mutex is already locked, but we're prepared for
            // this
            return wxMUTEX_BUSY;

        case EINVAL:
            wxLogDebug(_T("pthread_mutex_trylock(): mutex not initialized."));
            break;

        case 0:
            return wxMUTEX_NO_ERROR;

        default:
            wxLogApiError(_T("pthread_mutex_trylock()"), err);
    }

    return wxMUTEX_MISC_ERROR;
}

wxMutexError wxMutexInternal::Unlock()
{
    int err = pthread_mutex_unlock(&m_mutex);
    switch ( err )
    {
        case EPERM:
            // we don't own the mutex
            return wxMUTEX_UNLOCKED;

        case EINVAL:
            wxLogDebug(_T("pthread_mutex_unlock(): mutex not initialized."));
            break;

        case 0:
            return wxMUTEX_NO_ERROR;

        default:
            wxLogApiError(_T("pthread_mutex_unlock()"), err);
    }

    return wxMUTEX_MISC_ERROR;
}

// ===========================================================================
// wxCondition implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxConditionInternal
// ---------------------------------------------------------------------------

// this is a wrapper around pthread_cond_t associated with a wxMutex (and hence
// with a pthread_mutex_t)
class wxConditionInternal
{
public:
    wxConditionInternal(wxMutex& mutex);
    ~wxConditionInternal();

    bool IsOk() const { return m_isOk && m_mutex.IsOk(); }

    wxCondError Wait();
    wxCondError WaitTimeout(unsigned long milliseconds);

    wxCondError Signal();
    wxCondError Broadcast();

private:
    // get the POSIX mutex associated with us
    pthread_mutex_t *GetPMutex() const { return &m_mutex.m_internal->m_mutex; }

    wxMutex& m_mutex;
    pthread_cond_t m_cond;

    bool m_isOk;
};

wxConditionInternal::wxConditionInternal(wxMutex& mutex)
                   : m_mutex(mutex)
{
    int err = pthread_cond_init(&m_cond, NULL /* default attributes */);

    m_isOk = err == 0;

    if ( !m_isOk )
    {
        wxLogApiError(_T("pthread_cond_init()"), err);
    }
}

wxConditionInternal::~wxConditionInternal()
{
    if ( m_isOk )
    {
        int err = pthread_cond_destroy(&m_cond);
        if ( err != 0 )
        {
            wxLogApiError(_T("pthread_cond_destroy()"), err);
        }
    }
}

wxCondError wxConditionInternal::Wait()
{
    int err = pthread_cond_wait(&m_cond, GetPMutex());
    if ( err != 0 )
    {
        wxLogApiError(_T("pthread_cond_wait()"), err);

        return wxCOND_MISC_ERROR;
    }

    return wxCOND_NO_ERROR;
}

wxCondError wxConditionInternal::WaitTimeout(unsigned long milliseconds)
{
    wxLongLong curtime = wxGetLocalTimeMillis();
    curtime += milliseconds;
    wxLongLong temp = curtime / 1000;
    int sec = temp.GetLo();
    temp *= 1000;
    temp = curtime - temp;
    int millis = temp.GetLo();

    timespec tspec;

    tspec.tv_sec = sec;
    tspec.tv_nsec = millis * 1000L * 1000L;

    int err = pthread_cond_timedwait( &m_cond, GetPMutex(), &tspec );
    switch ( err )
    {
        case ETIMEDOUT:
            return wxCOND_TIMEOUT;

        case 0:
            return wxCOND_NO_ERROR;

        default:
            wxLogApiError(_T("pthread_cond_timedwait()"), err);
    }

    return wxCOND_MISC_ERROR;
}

wxCondError wxConditionInternal::Signal()
{
    int err = pthread_cond_signal(&m_cond);
    if ( err != 0 )
    {
        wxLogApiError(_T("pthread_cond_signal()"), err);

        return wxCOND_MISC_ERROR;
    }

    return wxCOND_NO_ERROR;
}

wxCondError wxConditionInternal::Broadcast()
{
    int err = pthread_cond_broadcast(&m_cond);
    if ( err != 0 )
    {
        wxLogApiError(_T("pthread_cond_broadcast()"), err);

        return wxCOND_MISC_ERROR;
    }

    return wxCOND_NO_ERROR;
}

// ===========================================================================
// wxSemaphore implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxSemaphoreInternal
// ---------------------------------------------------------------------------

// we implement the semaphores using mutexes and conditions instead of using
// the sem_xxx() POSIX functions because they're not widely available and also
// because it's impossible to implement WaitTimeout() using them
class wxSemaphoreInternal
{
public:
    wxSemaphoreInternal(int initialcount, int maxcount);

    bool IsOk() const { return m_isOk; }

    wxSemaError Wait();
    wxSemaError TryWait();
    wxSemaError WaitTimeout(unsigned long milliseconds);

    wxSemaError Post();

private:
    wxMutex m_mutex;
    wxCondition m_cond;

    size_t m_count,
           m_maxcount;

    bool m_isOk;
};

wxSemaphoreInternal::wxSemaphoreInternal(int initialcount, int maxcount)
                   : m_cond(m_mutex)
{

    if ( (initialcount < 0 || maxcount < 0) ||
            ((maxcount > 0) && (initialcount > maxcount)) )
    {
        wxFAIL_MSG( _T("wxSemaphore: invalid initial or maximal count") );

        m_isOk = false;
    }
    else
    {
        m_maxcount = (size_t)maxcount;
        m_count = (size_t)initialcount;
    }

    m_isOk = m_mutex.IsOk() && m_cond.IsOk();
}

wxSemaError wxSemaphoreInternal::Wait()
{
    wxMutexLocker locker(m_mutex);

    while ( m_count == 0 )
    {
        wxLogTrace(TRACE_SEMA,
                   _T("Thread %ld waiting for semaphore to become signalled"),
                   wxThread::GetCurrentId());

        if ( m_cond.Wait() != wxCOND_NO_ERROR )
            return wxSEMA_MISC_ERROR;

        wxLogTrace(TRACE_SEMA,
                   _T("Thread %ld finished waiting for semaphore, count = %lu"),
                   wxThread::GetCurrentId(), (unsigned long)m_count);
    }

    m_count--;

    return wxSEMA_NO_ERROR;
}

wxSemaError wxSemaphoreInternal::TryWait()
{
    wxMutexLocker locker(m_mutex);

    if ( m_count == 0 )
        return wxSEMA_BUSY;

    m_count--;

    return wxSEMA_NO_ERROR;
}

wxSemaError wxSemaphoreInternal::WaitTimeout(unsigned long milliseconds)
{
    wxMutexLocker locker(m_mutex);

    wxLongLong startTime = wxGetLocalTimeMillis();

    while ( m_count == 0 )
    {
        wxLongLong elapsed = wxGetLocalTimeMillis() - startTime;
        long remainingTime = (long)milliseconds - (long)elapsed.GetLo();
        if ( remainingTime <= 0 )
        {
            // timeout
            return wxSEMA_TIMEOUT;
        }

        switch ( m_cond.WaitTimeout(remainingTime) )
        {
            case wxCOND_TIMEOUT:
                return wxSEMA_TIMEOUT;

            default:
                return wxSEMA_MISC_ERROR;

            case wxCOND_NO_ERROR:
                ;
        }
    }

    m_count--;

    return wxSEMA_NO_ERROR;
}

wxSemaError wxSemaphoreInternal::Post()
{
    wxMutexLocker locker(m_mutex);

    if ( m_maxcount > 0 && m_count == m_maxcount )
    {
        return wxSEMA_OVERFLOW;
    }

    m_count++;

    wxLogTrace(TRACE_SEMA,
               _T("Thread %ld about to signal semaphore, count = %lu"),
               wxThread::GetCurrentId(), (unsigned long)m_count);

    return m_cond.Signal() == wxCOND_NO_ERROR ? wxSEMA_NO_ERROR
                                              : wxSEMA_MISC_ERROR;
}

// ===========================================================================
// wxThread implementation
// ===========================================================================

// the thread callback functions must have the C linkage
extern "C"
{

#ifdef wxHAVE_PTHREAD_CLEANUP
    // thread exit function
    void wxPthreadCleanup(void *ptr);
#endif // wxHAVE_PTHREAD_CLEANUP

void *wxPthreadStart(void *ptr);

} // extern "C"

// ----------------------------------------------------------------------------
// wxThreadInternal
// ----------------------------------------------------------------------------

class wxThreadInternal
{
public:
    wxThreadInternal();
    ~wxThreadInternal();

    // thread entry function
    static void *PthreadStart(wxThread *thread);

    // thread actions
        // start the thread
    wxThreadError Run();
        // unblock the thread allowing it to run
    void SignalRun() { m_semRun.Post(); }
        // ask the thread to terminate
    void Wait();
        // go to sleep until Resume() is called
    void Pause();
        // resume the thread
    void Resume();

    // accessors
        // priority
    int GetPriority() const { return m_prio; }
    void SetPriority(int prio) { m_prio = prio; }
        // state
    wxThreadState GetState() const { return m_state; }
    void SetState(wxThreadState state)
    {
#ifdef __WXDEBUG__
        static const wxChar *stateNames[] =
        {
            _T("NEW"),
            _T("RUNNING"),
            _T("PAUSED"),
            _T("EXITED"),
        };

        wxLogTrace(TRACE_THREADS, _T("Thread %ld: %s => %s."),
                   (long)GetId(), stateNames[m_state], stateNames[state]);
#endif // __WXDEBUG__

        m_state = state;
    }
        // id
    pthread_t GetId() const { return m_threadId; }
    pthread_t *GetIdPtr() { return &m_threadId; }
        // "cancelled" flag
    void SetCancelFlag() { m_cancelled = true; }
    bool WasCancelled() const { return m_cancelled; }
        // exit code
    void SetExitCode(wxThread::ExitCode exitcode) { m_exitcode = exitcode; }
    wxThread::ExitCode GetExitCode() const { return m_exitcode; }

        // the pause flag
    void SetReallyPaused(bool paused) { m_isPaused = paused; }
    bool IsReallyPaused() const { return m_isPaused; }

        // tell the thread that it is a detached one
    void Detach()
    {
        wxCriticalSectionLocker lock(m_csJoinFlag);

        m_shouldBeJoined = false;
        m_isDetached = true;
    }

#ifdef wxHAVE_PTHREAD_CLEANUP
    // this is used by wxPthreadCleanup() only
    static void Cleanup(wxThread *thread);
#endif // wxHAVE_PTHREAD_CLEANUP

private:
    pthread_t     m_threadId;   // id of the thread
    wxThreadState m_state;      // see wxThreadState enum
    int           m_prio;       // in wxWidgets units: from 0 to 100

    // this flag is set when the thread should terminate
    bool m_cancelled;

    // this flag is set when the thread is blocking on m_semSuspend
    bool m_isPaused;

    // the thread exit code - only used for joinable (!detached) threads and
    // is only valid after the thread termination
    wxThread::ExitCode m_exitcode;

    // many threads may call Wait(), but only one of them should call
    // pthread_join(), so we have to keep track of this
    wxCriticalSection m_csJoinFlag;
    bool m_shouldBeJoined;
    bool m_isDetached;

    // this semaphore is posted by Run() and the threads Entry() is not
    // called before it is done
    wxSemaphore m_semRun;

    // this one is signaled when the thread should resume after having been
    // Pause()d
    wxSemaphore m_semSuspend;
};

// ----------------------------------------------------------------------------
// thread startup and exit functions
// ----------------------------------------------------------------------------

void *wxPthreadStart(void *ptr)
{
    return wxThreadInternal::PthreadStart((wxThread *)ptr);
}

void *wxThreadInternal::PthreadStart(wxThread *thread)
{
    wxThreadInternal *pthread = thread->m_internal;

    wxLogTrace(TRACE_THREADS, _T("Thread %ld started."), THR_ID(pthread));

    // associate the thread pointer with the newly created thread so that
    // wxThread::This() will work
    int rc = pthread_setspecific(gs_keySelf, thread);
    if ( rc != 0 )
    {
        wxLogSysError(rc, _("Cannot start thread: error writing TLS"));

        return (void *)-1;
    }

    // have to declare this before pthread_cleanup_push() which defines a
    // block!
    bool dontRunAtAll;

#ifdef wxHAVE_PTHREAD_CLEANUP
    // install the cleanup handler which will be called if the thread is
    // cancelled
    pthread_cleanup_push(wxPthreadCleanup, thread);
#endif // wxHAVE_PTHREAD_CLEANUP

    // wait for the semaphore to be posted from Run()
    pthread->m_semRun.Wait();

    // test whether we should run the run at all - may be it was deleted
    // before it started to Run()?
    {
        wxCriticalSectionLocker lock(thread->m_critsect);

        dontRunAtAll = pthread->GetState() == STATE_NEW &&
                       pthread->WasCancelled();
    }

    if ( !dontRunAtAll )
    {
        // call the main entry
        wxLogTrace(TRACE_THREADS,
                   _T("Thread %ld about to enter its Entry()."),
                   THR_ID(pthread));

        pthread->m_exitcode = thread->Entry();

        wxLogTrace(TRACE_THREADS,
                   _T("Thread %ld Entry() returned %lu."),
                   THR_ID(pthread), wxPtrToUInt(pthread->m_exitcode));

        {
            wxCriticalSectionLocker lock(thread->m_critsect);

            // change the state of the thread to "exited" so that
            // wxPthreadCleanup handler won't do anything from now (if it's
            // called before we do pthread_cleanup_pop below)
            pthread->SetState(STATE_EXITED);
        }
    }

    // NB: pthread_cleanup_push/pop() are macros and pop contains the matching
    //     '}' for the '{' in push, so they must be used in the same block!
#ifdef wxHAVE_PTHREAD_CLEANUP
    #ifdef __DECCXX
        // under Tru64 we get a warning from macro expansion
        #pragma message save
        #pragma message disable(declbutnotref)
    #endif

    // remove the cleanup handler without executing it
    pthread_cleanup_pop(FALSE);

    #ifdef __DECCXX
        #pragma message restore
    #endif
#endif // wxHAVE_PTHREAD_CLEANUP

    if ( dontRunAtAll )
    {
        // FIXME: deleting a possibly joinable thread here???
        delete thread;

        return EXITCODE_CANCELLED;
    }
    else
    {
        // terminate the thread
        thread->Exit(pthread->m_exitcode);

        wxFAIL_MSG(wxT("wxThread::Exit() can't return."));

        return NULL;
    }
}

#ifdef wxHAVE_PTHREAD_CLEANUP

// this handler is called when the thread is cancelled
extern "C" void wxPthreadCleanup(void *ptr)
{
    wxThreadInternal::Cleanup((wxThread *)ptr);
}

void wxThreadInternal::Cleanup(wxThread *thread)
{
    if (pthread_getspecific(gs_keySelf) == 0) return;
    {
        wxCriticalSectionLocker lock(thread->m_critsect);
        if ( thread->m_internal->GetState() == STATE_EXITED )
        {
            // thread is already considered as finished.
            return;
        }
    }

    // exit the thread gracefully
    thread->Exit(EXITCODE_CANCELLED);
}

#endif // wxHAVE_PTHREAD_CLEANUP

// ----------------------------------------------------------------------------
// wxThreadInternal
// ----------------------------------------------------------------------------

wxThreadInternal::wxThreadInternal()
{
    m_state = STATE_NEW;
    m_cancelled = false;
    m_prio = WXTHREAD_DEFAULT_PRIORITY;
    m_threadId = 0;
    m_exitcode = 0;

    // set to true only when the thread starts waiting on m_semSuspend
    m_isPaused = false;

    // defaults for joinable threads
    m_shouldBeJoined = true;
    m_isDetached = false;
}

wxThreadInternal::~wxThreadInternal()
{
}

wxThreadError wxThreadInternal::Run()
{
    wxCHECK_MSG( GetState() == STATE_NEW, wxTHREAD_RUNNING,
                 wxT("thread may only be started once after Create()") );

    SetState(STATE_RUNNING);

    // wake up threads waiting for our start
    SignalRun();

    return wxTHREAD_NO_ERROR;
}

void wxThreadInternal::Wait()
{
    wxCHECK_RET( !m_isDetached, _T("can't wait for a detached thread") );

    // if the thread we're waiting for is waiting for the GUI mutex, we will
    // deadlock so make sure we release it temporarily
    if ( wxThread::IsMain() )
        wxMutexGuiLeave();

    wxLogTrace(TRACE_THREADS,
               _T("Starting to wait for thread %ld to exit."),
               THR_ID(this));

    // to avoid memory leaks we should call pthread_join(), but it must only be
    // done once so use a critical section to serialize the code below
    {
        wxCriticalSectionLocker lock(m_csJoinFlag);

        if ( m_shouldBeJoined )
        {
            // FIXME shouldn't we set cancellation type to DISABLED here? If
            //       we're cancelled inside pthread_join(), things will almost
            //       certainly break - but if we disable the cancellation, we
            //       might deadlock
            if ( pthread_join(GetId(), &m_exitcode) != 0 )
            {
                // this is a serious problem, so use wxLogError and not
                // wxLogDebug: it is possible to bring the system to its knees
                // by creating too many threads and not joining them quite
                // easily
                wxLogError(_("Failed to join a thread, potential memory leak detected - please restart the program"));
            }

            m_shouldBeJoined = false;
        }
    }

    // reacquire GUI mutex
    if ( wxThread::IsMain() )
        wxMutexGuiEnter();
}

void wxThreadInternal::Pause()
{
    // the state is set from the thread which pauses us first, this function
    // is called later so the state should have been already set
    wxCHECK_RET( m_state == STATE_PAUSED,
                 wxT("thread must first be paused with wxThread::Pause().") );

   wxLogTrace(TRACE_THREADS,
              _T("Thread %ld goes to sleep."), THR_ID(this));

    // wait until the semaphore is Post()ed from Resume()
    m_semSuspend.Wait();
}

void wxThreadInternal::Resume()
{
    wxCHECK_RET( m_state == STATE_PAUSED,
                 wxT("can't resume thread which is not suspended.") );

    // the thread might be not actually paused yet - if there were no call to
    // TestDestroy() since the last call to Pause() for example
    if ( IsReallyPaused() )
    {
       wxLogTrace(TRACE_THREADS,
                  _T("Waking up thread %ld"), THR_ID(this));

        // wake up Pause()
        m_semSuspend.Post();

        // reset the flag
        SetReallyPaused(false);
    }
    else
    {
        wxLogTrace(TRACE_THREADS,
                   _T("Thread %ld is not yet really paused"), THR_ID(this));
    }

    SetState(STATE_RUNNING);
}

// -----------------------------------------------------------------------------
// wxThread static functions
// -----------------------------------------------------------------------------

wxThread *wxThread::This()
{
    return (wxThread *)pthread_getspecific(gs_keySelf);
}

bool wxThread::IsMain()
{
    return (bool)pthread_equal(pthread_self(), gs_tidMain) || gs_tidMain == (pthread_t)-1;
}

void wxThread::Yield()
{
#ifdef HAVE_SCHED_YIELD
    sched_yield();
#endif
}

void wxThread::Sleep(unsigned long milliseconds)
{
    wxMilliSleep(milliseconds);
}

int wxThread::GetCPUCount()
{
#if defined(_SC_NPROCESSORS_ONLN)
    // this works for Solaris and Linux 2.6
    int rc = sysconf(_SC_NPROCESSORS_ONLN);
    if ( rc != -1 )
    {
        return rc;
    }
#elif defined(__LINUX__) && wxUSE_FFILE
    // read from proc (can't use wxTextFile here because it's a special file:
    // it has 0 size but still can be read from)
    wxLogNull nolog;

    wxFFile file(_T("/proc/cpuinfo"));
    if ( file.IsOpened() )
    {
        // slurp the whole file
        wxString s;
        if ( file.ReadAll(&s) )
        {
            // (ab)use Replace() to find the number of "processor: num" strings
            size_t count = s.Replace(_T("processor\t:"), _T(""));
            if ( count > 0 )
            {
                return count;
            }

            wxLogDebug(_T("failed to parse /proc/cpuinfo"));
        }
        else
        {
            wxLogDebug(_T("failed to read /proc/cpuinfo"));
        }
    }
#endif // different ways to get number of CPUs

    // unknown
    return -1;
}

// VMS is a 64 bit system and threads have 64 bit pointers.
// FIXME: also needed for other systems????
#ifdef __VMS
unsigned long long wxThread::GetCurrentId()
{
    return (unsigned long long)pthread_self();
}

#else // !__VMS

unsigned long wxThread::GetCurrentId()
{
    return (unsigned long)pthread_self();
}

#endif // __VMS/!__VMS


bool wxThread::SetConcurrency(size_t level)
{
#ifdef HAVE_THR_SETCONCURRENCY
    int rc = thr_setconcurrency(level);
    if ( rc != 0 )
    {
        wxLogSysError(rc, _T("thr_setconcurrency() failed"));
    }

    return rc == 0;
#else // !HAVE_THR_SETCONCURRENCY
    // ok only for the default value
    return level == 0;
#endif // HAVE_THR_SETCONCURRENCY/!HAVE_THR_SETCONCURRENCY
}

// -----------------------------------------------------------------------------
// creating thread
// -----------------------------------------------------------------------------

wxThread::wxThread(wxThreadKind kind)
{
    // add this thread to the global list of all threads
    {
        wxMutexLocker lock(*gs_mutexAllThreads);

        gs_allThreads.Add(this);
    }

    m_internal = new wxThreadInternal();

    m_isDetached = kind == wxTHREAD_DETACHED;
}

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    #define WXUNUSED_STACKSIZE(identifier)  identifier
#else
    #define WXUNUSED_STACKSIZE(identifier)  WXUNUSED(identifier)
#endif

wxThreadError wxThread::Create(unsigned int WXUNUSED_STACKSIZE(stackSize))
{
    if ( m_internal->GetState() != STATE_NEW )
    {
        // don't recreate thread
        return wxTHREAD_RUNNING;
    }

    // set up the thread attribute: right now, we only set thread priority
    pthread_attr_t attr;
    pthread_attr_init(&attr);

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    if (stackSize)
      pthread_attr_setstacksize(&attr, stackSize);
#endif

#ifdef HAVE_THREAD_PRIORITY_FUNCTIONS
    int policy;
    if ( pthread_attr_getschedpolicy(&attr, &policy) != 0 )
    {
        wxLogError(_("Cannot retrieve thread scheduling policy."));
    }

#ifdef __VMS__
   /* the pthread.h contains too many spaces. This is a work-around */
# undef sched_get_priority_max
#undef sched_get_priority_min
#define sched_get_priority_max(_pol_) \
     (_pol_ == SCHED_OTHER ? PRI_FG_MAX_NP : PRI_FIFO_MAX)
#define sched_get_priority_min(_pol_) \
     (_pol_ == SCHED_OTHER ? PRI_FG_MIN_NP : PRI_FIFO_MIN)
#endif

    int max_prio = sched_get_priority_max(policy),
        min_prio = sched_get_priority_min(policy),
        prio = m_internal->GetPriority();

    if ( min_prio == -1 || max_prio == -1 )
    {
        wxLogError(_("Cannot get priority range for scheduling policy %d."),
                   policy);
    }
    else if ( max_prio == min_prio )
    {
        if ( prio != WXTHREAD_DEFAULT_PRIORITY )
        {
            // notify the programmer that this doesn't work here
            wxLogWarning(_("Thread priority setting is ignored."));
        }
        //else: we have default priority, so don't complain

        // anyhow, don't do anything because priority is just ignored
    }
    else
    {
        struct sched_param sp;
        if ( pthread_attr_getschedparam(&attr, &sp) != 0 )
        {
            wxFAIL_MSG(_T("pthread_attr_getschedparam() failed"));
        }

        sp.sched_priority = min_prio + (prio*(max_prio - min_prio))/100;

        if ( pthread_attr_setschedparam(&attr, &sp) != 0 )
        {
            wxFAIL_MSG(_T("pthread_attr_setschedparam(priority) failed"));
        }
    }
#endif // HAVE_THREAD_PRIORITY_FUNCTIONS

#ifdef HAVE_PTHREAD_ATTR_SETSCOPE
    // this will make the threads created by this process really concurrent
    if ( pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) != 0 )
    {
        wxFAIL_MSG(_T("pthread_attr_setscope(PTHREAD_SCOPE_SYSTEM) failed"));
    }
#endif // HAVE_PTHREAD_ATTR_SETSCOPE

    // VZ: assume that this one is always available (it's rather fundamental),
    //     if this function is ever missing we should try to use
    //     pthread_detach() instead (after thread creation)
    if ( m_isDetached )
    {
        if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 )
        {
            wxFAIL_MSG(_T("pthread_attr_setdetachstate(DETACHED) failed"));
        }

        // never try to join detached threads
        m_internal->Detach();
    }
    //else: threads are created joinable by default, it's ok

    // create the new OS thread object
    int rc = pthread_create
             (
                m_internal->GetIdPtr(),
                &attr,
                wxPthreadStart,
                (void *)this
             );

    if ( pthread_attr_destroy(&attr) != 0 )
    {
        wxFAIL_MSG(_T("pthread_attr_destroy() failed"));
    }

    if ( rc != 0 )
    {
        m_internal->SetState(STATE_EXITED);

        return wxTHREAD_NO_RESOURCE;
    }

    return wxTHREAD_NO_ERROR;
}

wxThreadError wxThread::Run()
{
    wxCriticalSectionLocker lock(m_critsect);

    wxCHECK_MSG( m_internal->GetId(), wxTHREAD_MISC_ERROR,
                 wxT("must call wxThread::Create() first") );

    return m_internal->Run();
}

// -----------------------------------------------------------------------------
// misc accessors
// -----------------------------------------------------------------------------

void wxThread::SetPriority(unsigned int prio)
{
    wxCHECK_RET( ((int)WXTHREAD_MIN_PRIORITY <= (int)prio) &&
                 ((int)prio <= (int)WXTHREAD_MAX_PRIORITY),
                 wxT("invalid thread priority") );

    wxCriticalSectionLocker lock(m_critsect);

    switch ( m_internal->GetState() )
    {
        case STATE_NEW:
            // thread not yet started, priority will be set when it is
            m_internal->SetPriority(prio);
            break;

        case STATE_RUNNING:
        case STATE_PAUSED:
#ifdef HAVE_THREAD_PRIORITY_FUNCTIONS
#if defined(__LINUX__)
            // On Linux, pthread_setschedparam with SCHED_OTHER does not allow
            // a priority other than 0.  Instead, we use the BSD setpriority
            // which alllows us to set a 'nice' value between 20 to -20.  Only
            // super user can set a value less than zero (more negative yields
            // higher priority).  setpriority set the static priority of a
            // process, but this is OK since Linux is configured as a thread
            // per process.
            //
            // FIXME this is not true for 2.6!!

            // map wx priorites WXTHREAD_MIN_PRIORITY..WXTHREAD_MAX_PRIORITY
            // to Unix priorities 20..-20
            if ( setpriority(PRIO_PROCESS, 0, -(2*(int)prio)/5 + 20) == -1 )
            {
                wxLogError(_("Failed to set thread priority %d."), prio);
            }
#else // __LINUX__
            {
                struct sched_param sparam;
                sparam.sched_priority = prio;

                if ( pthread_setschedparam(m_internal->GetId(),
                                           SCHED_OTHER, &sparam) != 0 )
                {
                    wxLogError(_("Failed to set thread priority %d."), prio);
                }
            }
#endif // __LINUX__
#endif // HAVE_THREAD_PRIORITY_FUNCTIONS
            break;

        case STATE_EXITED:
        default:
            wxFAIL_MSG(wxT("impossible to set thread priority in this state"));
    }
}

unsigned int wxThread::GetPriority() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect);

    return m_internal->GetPriority();
}

wxThreadIdType wxThread::GetId() const
{
    return (wxThreadIdType) m_internal->GetId();
}

// -----------------------------------------------------------------------------
// pause/resume
// -----------------------------------------------------------------------------

wxThreadError wxThread::Pause()
{
    wxCHECK_MSG( This() != this, wxTHREAD_MISC_ERROR,
                 _T("a thread can't pause itself") );

    wxCriticalSectionLocker lock(m_critsect);

    if ( m_internal->GetState() != STATE_RUNNING )
    {
        wxLogDebug(wxT("Can't pause thread which is not running."));

        return wxTHREAD_NOT_RUNNING;
    }

    // just set a flag, the thread will be really paused only during the next
    // call to TestDestroy()
    m_internal->SetState(STATE_PAUSED);

    return wxTHREAD_NO_ERROR;
}

wxThreadError wxThread::Resume()
{
    wxCHECK_MSG( This() != this, wxTHREAD_MISC_ERROR,
                 _T("a thread can't resume itself") );

    wxCriticalSectionLocker lock(m_critsect);

    wxThreadState state = m_internal->GetState();

    switch ( state )
    {
        case STATE_PAUSED:
            wxLogTrace(TRACE_THREADS, _T("Thread %ld suspended, resuming."),
                       GetId());

            m_internal->Resume();

            return wxTHREAD_NO_ERROR;

        case STATE_EXITED:
            wxLogTrace(TRACE_THREADS, _T("Thread %ld exited, won't resume."),
                       GetId());
            return wxTHREAD_NO_ERROR;

        default:
            wxLogDebug(_T("Attempt to resume a thread which is not paused."));

            return wxTHREAD_MISC_ERROR;
    }
}

// -----------------------------------------------------------------------------
// exiting thread
// -----------------------------------------------------------------------------

wxThread::ExitCode wxThread::Wait()
{
    wxCHECK_MSG( This() != this, (ExitCode)-1,
                 _T("a thread can't wait for itself") );

    wxCHECK_MSG( !m_isDetached, (ExitCode)-1,
                 _T("can't wait for detached thread") );

    m_internal->Wait();

    return m_internal->GetExitCode();
}

wxThreadError wxThread::Delete(ExitCode *rc)
{
    wxCHECK_MSG( This() != this, wxTHREAD_MISC_ERROR,
                 _T("a thread can't delete itself") );

    bool isDetached = m_isDetached;

    m_critsect.Enter();
    wxThreadState state = m_internal->GetState();

    // ask the thread to stop
    m_internal->SetCancelFlag();

    m_critsect.Leave();

    switch ( state )
    {
        case STATE_NEW:
            // we need to wake up the thread so that PthreadStart() will
            // terminate - right now it's blocking on run semaphore in
            // PthreadStart()
            m_internal->SignalRun();

            // fall through

        case STATE_EXITED:
            // nothing to do
            break;

        case STATE_PAUSED:
            // resume the thread first
            m_internal->Resume();

            // fall through

        default:
            if ( !isDetached )
            {
                // wait until the thread stops
                m_internal->Wait();

                if ( rc )
                {
                    // return the exit code of the thread
                    *rc = m_internal->GetExitCode();
                }
            }
            //else: can't wait for detached threads
    }

    return wxTHREAD_NO_ERROR;
}

wxThreadError wxThread::Kill()
{
    wxCHECK_MSG( This() != this, wxTHREAD_MISC_ERROR,
                 _T("a thread can't kill itself") );

    switch ( m_internal->GetState() )
    {
        case STATE_NEW:
        case STATE_EXITED:
            return wxTHREAD_NOT_RUNNING;

        case STATE_PAUSED:
            // resume the thread first
            Resume();

            // fall through

        default:
#ifdef HAVE_PTHREAD_CANCEL
            if ( pthread_cancel(m_internal->GetId()) != 0 )
#endif // HAVE_PTHREAD_CANCEL
            {
                wxLogError(_("Failed to terminate a thread."));

                return wxTHREAD_MISC_ERROR;
            }

#ifdef HAVE_PTHREAD_CANCEL
            if ( m_isDetached )
            {
                // if we use cleanup function, this will be done from
                // wxPthreadCleanup()
#ifndef wxHAVE_PTHREAD_CLEANUP
                ScheduleThreadForDeletion();

                // don't call OnExit() here, it can only be called in the
                // threads context and we're in the context of another thread

                DeleteThread(this);
#endif // wxHAVE_PTHREAD_CLEANUP
            }
            else
            {
                m_internal->SetExitCode(EXITCODE_CANCELLED);
            }

            return wxTHREAD_NO_ERROR;
#endif // HAVE_PTHREAD_CANCEL
    }
}

void wxThread::Exit(ExitCode status)
{
    wxASSERT_MSG( This() == this,
                  _T("wxThread::Exit() can only be called in the context of the same thread") );

    if ( m_isDetached )
    {
        // from the moment we call OnExit(), the main program may terminate at
        // any moment, so mark this thread as being already in process of being
        // deleted or wxThreadModule::OnExit() will try to delete it again
        ScheduleThreadForDeletion();
    }

    // don't enter m_critsect before calling OnExit() because the user code
    // might deadlock if, for example, it signals a condition in OnExit() (a
    // common case) while the main thread calls any of functions entering
    // m_critsect on us (almost all of them do)
    OnExit();

    // delete C++ thread object if this is a detached thread - user is
    // responsible for doing this for joinable ones
    if ( m_isDetached )
    {
        // FIXME I'm feeling bad about it - what if another thread function is
        //       called (in another thread context) now? It will try to access
        //       half destroyed object which will probably result in something
        //       very bad - but we can't protect this by a crit section unless
        //       we make it a global object, but this would mean that we can
        //       only call one thread function at a time :-(
        DeleteThread(this);
        pthread_setspecific(gs_keySelf, 0);
    }
    else
    {
        m_critsect.Enter();
        m_internal->SetState(STATE_EXITED);
        m_critsect.Leave();
    }

    // terminate the thread (pthread_exit() never returns)
    pthread_exit(status);

    wxFAIL_MSG(_T("pthread_exit() failed"));
}

// also test whether we were paused
bool wxThread::TestDestroy()
{
    wxASSERT_MSG( This() == this,
                  _T("wxThread::TestDestroy() can only be called in the context of the same thread") );

    m_critsect.Enter();

    if ( m_internal->GetState() == STATE_PAUSED )
    {
        m_internal->SetReallyPaused(true);

        // leave the crit section or the other threads will stop too if they
        // try to call any of (seemingly harmless) IsXXX() functions while we
        // sleep
        m_critsect.Leave();

        m_internal->Pause();
    }
    else
    {
        // thread wasn't requested to pause, nothing to do
        m_critsect.Leave();
    }

    return m_internal->WasCancelled();
}

wxThread::~wxThread()
{
#ifdef __WXDEBUG__
    m_critsect.Enter();

    // check that the thread either exited or couldn't be created
    if ( m_internal->GetState() != STATE_EXITED &&
         m_internal->GetState() != STATE_NEW )
    {
        wxLogDebug(_T("The thread %ld is being destroyed although it is still running! The application may crash."),
                   (long)GetId());
    }

    m_critsect.Leave();
#endif // __WXDEBUG__

    delete m_internal;

    // remove this thread from the global array
    {
        wxMutexLocker lock(*gs_mutexAllThreads);

        gs_allThreads.Remove(this);
    }
}

// -----------------------------------------------------------------------------
// state tests
// -----------------------------------------------------------------------------

bool wxThread::IsRunning() const
{
    wxCriticalSectionLocker lock((wxCriticalSection &)m_critsect);

    return m_internal->GetState() == STATE_RUNNING;
}

bool wxThread::IsAlive() const
{
    wxCriticalSectionLocker lock((wxCriticalSection&)m_critsect);

    switch ( m_internal->GetState() )
    {
        case STATE_RUNNING:
        case STATE_PAUSED:
            return true;

        default:
            return false;
    }
}

bool wxThread::IsPaused() const
{
    wxCriticalSectionLocker lock((wxCriticalSection&)m_critsect);

    return (m_internal->GetState() == STATE_PAUSED);
}

//--------------------------------------------------------------------
// wxThreadModule
//--------------------------------------------------------------------

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
    int rc = pthread_key_create(&gs_keySelf, NULL /* dtor function */);
    if ( rc != 0 )
    {
        wxLogSysError(rc, _("Thread module initialization failed: failed to create thread key"));

        return false;
    }

    gs_tidMain = pthread_self();

    gs_mutexAllThreads = new wxMutex();

    gs_mutexGui = new wxMutex();
    gs_mutexGui->Lock();

    gs_mutexDeleteThread = new wxMutex();
    gs_condAllDeleted = new wxCondition(*gs_mutexDeleteThread);

    return true;
}

void wxThreadModule::OnExit()
{
    wxASSERT_MSG( wxThread::IsMain(), wxT("only main thread can be here") );

    // are there any threads left which are being deleted right now?
    size_t nThreadsBeingDeleted;

    {
        wxMutexLocker lock( *gs_mutexDeleteThread );
        nThreadsBeingDeleted = gs_nThreadsBeingDeleted;

        if ( nThreadsBeingDeleted > 0 )
        {
            wxLogTrace(TRACE_THREADS,
                       _T("Waiting for %lu threads to disappear"),
                       (unsigned long)nThreadsBeingDeleted);

            // have to wait until all of them disappear
            gs_condAllDeleted->Wait();
        }
    }

    size_t count;

    {
        wxMutexLocker lock(*gs_mutexAllThreads);

        // terminate any threads left
        count = gs_allThreads.GetCount();
        if ( count != 0u )
        {
            wxLogDebug(wxT("%lu threads were not terminated by the application."),
                       (unsigned long)count);
        }
    } // unlock mutex before deleting the threads as they lock it in their dtor

    for ( size_t n = 0u; n < count; n++ )
    {
        // Delete calls the destructor which removes the current entry. We
        // should only delete the first one each time.
        gs_allThreads[0]->Delete();
    }

    delete gs_mutexAllThreads;

    // destroy GUI mutex
    gs_mutexGui->Unlock();
    delete gs_mutexGui;

    // and free TLD slot
    (void)pthread_key_delete(gs_keySelf);

    delete gs_condAllDeleted;
    delete gs_mutexDeleteThread;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

static void ScheduleThreadForDeletion()
{
    wxMutexLocker lock( *gs_mutexDeleteThread );

    gs_nThreadsBeingDeleted++;

    wxLogTrace(TRACE_THREADS, _T("%lu thread%s waiting to be deleted"),
               (unsigned long)gs_nThreadsBeingDeleted,
               gs_nThreadsBeingDeleted == 1 ? _T("") : _T("s"));
}

static void DeleteThread(wxThread *This)
{
    // gs_mutexDeleteThread should be unlocked before signalling the condition
    // or wxThreadModule::OnExit() would deadlock
    wxMutexLocker locker( *gs_mutexDeleteThread );

    wxLogTrace(TRACE_THREADS, _T("Thread %ld auto deletes."), This->GetId());

    delete This;

    wxCHECK_RET( gs_nThreadsBeingDeleted > 0,
                 _T("no threads scheduled for deletion, yet we delete one?") );

    wxLogTrace(TRACE_THREADS, _T("%lu scheduled for deletion threads left."),
               (unsigned long)gs_nThreadsBeingDeleted - 1);

    if ( !--gs_nThreadsBeingDeleted )
    {
        // no more threads left, signal it
        gs_condAllDeleted->Signal();
    }
}

void wxMutexGuiEnter()
{
    gs_mutexGui->Lock();
}

void wxMutexGuiLeave()
{
    gs_mutexGui->Unlock();
}

// ----------------------------------------------------------------------------
// include common implementation code
// ----------------------------------------------------------------------------

#include "wx/thrimpl.cpp"

#endif // wxUSE_THREADS
