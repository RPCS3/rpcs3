/////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/thrimpl.cpp
// Purpose:     common part of wxThread Implementations
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.06.02 (extracted from src/*/thread.cpp files)
// RCS-ID:      $Id: thrimpl.cpp 66922 2011-02-16 22:26:57Z JS $
// Copyright:   (c) Vadim Zeitlin (2002)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// this file is supposed to be included only by the various thread.cpp

// ----------------------------------------------------------------------------
// wxMutex
// ----------------------------------------------------------------------------

wxMutex::wxMutex(wxMutexType mutexType)
{
    m_internal = new wxMutexInternal(mutexType);

    if ( !m_internal->IsOk() )
    {
        delete m_internal;
        m_internal = NULL;
    }
}

wxMutex::~wxMutex()
{
    delete m_internal;
}

bool wxMutex::IsOk() const
{
    return m_internal != NULL;
}

wxMutexError wxMutex::Lock()
{
    wxCHECK_MSG( m_internal, wxMUTEX_INVALID,
                 wxT("wxMutex::Lock(): not initialized") );

    return m_internal->Lock();
}

wxMutexError wxMutex::TryLock()
{
    wxCHECK_MSG( m_internal, wxMUTEX_INVALID,
                 wxT("wxMutex::TryLock(): not initialized") );

    return m_internal->TryLock();
}

wxMutexError wxMutex::Unlock()
{
    wxCHECK_MSG( m_internal, wxMUTEX_INVALID,
                 wxT("wxMutex::Unlock(): not initialized") );

    return m_internal->Unlock();
}

// --------------------------------------------------------------------------
// wxConditionInternal
// --------------------------------------------------------------------------

// Win32 and OS/2 don't have explicit support for the POSIX condition
// variables and their events/event semaphores have quite different semantics,
// so we reimplement the conditions from scratch using the mutexes and
// semaphores
#if defined(__WXMSW__) || defined(__OS2__) || defined(__EMX__)

class wxConditionInternal
{
public:
    wxConditionInternal(wxMutex& mutex);

    bool IsOk() const { return m_mutex.IsOk() && m_semaphore.IsOk(); }

    wxCondError Wait();
    wxCondError WaitTimeout(unsigned long milliseconds);

    wxCondError Signal();
    wxCondError Broadcast();

private:
    // the number of threads currently waiting for this condition
    LONG m_numWaiters;

    // the critical section protecting m_numWaiters
    wxCriticalSection m_csWaiters;

    wxMutex& m_mutex;
    wxSemaphore m_semaphore;

    DECLARE_NO_COPY_CLASS(wxConditionInternal)
};

wxConditionInternal::wxConditionInternal(wxMutex& mutex)
                   : m_mutex(mutex)
{
    // another thread can't access it until we return from ctor, so no need to
    // protect access to m_numWaiters here
    m_numWaiters = 0;
}

wxCondError wxConditionInternal::Wait()
{
    // increment the number of waiters
    {
        wxCriticalSectionLocker lock(m_csWaiters);
        m_numWaiters++;
    }

    m_mutex.Unlock();

    // after unlocking the mutex other threads may Signal() us, but it is ok
    // now as we had already incremented m_numWaiters so Signal() will post the
    // semaphore and decrement m_numWaiters back even if it is called before we
    // start to Wait()
    const wxSemaError err = m_semaphore.Wait();

    m_mutex.Lock();

    if ( err == wxSEMA_NO_ERROR )
    {
        // m_numWaiters was decremented by Signal()
        return wxCOND_NO_ERROR;
    }

    // but in case of an error we need to do it manually
    {
        wxCriticalSectionLocker lock(m_csWaiters);
        m_numWaiters--;
    }

    return err == wxSEMA_TIMEOUT ? wxCOND_TIMEOUT : wxCOND_MISC_ERROR;
}

wxCondError wxConditionInternal::WaitTimeout(unsigned long milliseconds)
{
    {
        wxCriticalSectionLocker lock(m_csWaiters);
        m_numWaiters++;
    }

    m_mutex.Unlock();

    wxSemaError err = m_semaphore.WaitTimeout(milliseconds);

    m_mutex.Lock();

    if ( err == wxSEMA_NO_ERROR )
        return wxCOND_NO_ERROR;

    if ( err == wxSEMA_TIMEOUT )
    {
        // a potential race condition exists here: it happens when a waiting
        // thread times out but doesn't have time to decrement m_numWaiters yet
        // before Signal() is called in another thread
        //
        // to handle this particular case, check the semaphore again after
        // acquiring m_csWaiters lock -- this will catch the signals missed
        // during this window
        wxCriticalSectionLocker lock(m_csWaiters);

        err = m_semaphore.WaitTimeout(0);
        if ( err == wxSEMA_NO_ERROR )
            return wxCOND_NO_ERROR;

        // we need to decrement m_numWaiters ourselves as it wasn't done by
        // Signal()
        m_numWaiters--;

        return err == wxSEMA_TIMEOUT ? wxCOND_TIMEOUT : wxCOND_MISC_ERROR;
    }

    // undo m_numWaiters++ above in case of an error
    {
        wxCriticalSectionLocker lock(m_csWaiters);
        m_numWaiters--;
    }

    return wxCOND_MISC_ERROR;
}

wxCondError wxConditionInternal::Signal()
{
    wxCriticalSectionLocker lock(m_csWaiters);

    if ( m_numWaiters > 0 )
    {
        // increment the semaphore by 1
        if ( m_semaphore.Post() != wxSEMA_NO_ERROR )
            return wxCOND_MISC_ERROR;

        m_numWaiters--;
    }

    return wxCOND_NO_ERROR;
}

wxCondError wxConditionInternal::Broadcast()
{
    wxCriticalSectionLocker lock(m_csWaiters);

    while ( m_numWaiters > 0 )
    {
        if ( m_semaphore.Post() != wxSEMA_NO_ERROR )
            return wxCOND_MISC_ERROR;

        m_numWaiters--;
    }

    return wxCOND_NO_ERROR;
}

#endif // MSW or OS2

// ----------------------------------------------------------------------------
// wxCondition
// ----------------------------------------------------------------------------

wxCondition::wxCondition(wxMutex& mutex)
{
    m_internal = new wxConditionInternal(mutex);

    if ( !m_internal->IsOk() )
    {
        delete m_internal;
        m_internal = NULL;
    }
}

wxCondition::~wxCondition()
{
    delete m_internal;
}

bool wxCondition::IsOk() const
{
    return m_internal != NULL;
}

wxCondError wxCondition::Wait()
{
    wxCHECK_MSG( m_internal, wxCOND_INVALID,
                 wxT("wxCondition::Wait(): not initialized") );

    return m_internal->Wait();
}

wxCondError wxCondition::WaitTimeout(unsigned long milliseconds)
{
    wxCHECK_MSG( m_internal, wxCOND_INVALID,
                 wxT("wxCondition::Wait(): not initialized") );

    return m_internal->WaitTimeout(milliseconds);
}

wxCondError wxCondition::Signal()
{
    wxCHECK_MSG( m_internal, wxCOND_INVALID,
                 wxT("wxCondition::Signal(): not initialized") );

    return m_internal->Signal();
}

wxCondError wxCondition::Broadcast()
{
    wxCHECK_MSG( m_internal, wxCOND_INVALID,
                 wxT("wxCondition::Broadcast(): not initialized") );

    return m_internal->Broadcast();
}

// --------------------------------------------------------------------------
// wxSemaphore
// --------------------------------------------------------------------------

wxSemaphore::wxSemaphore(int initialcount, int maxcount)
{
    m_internal = new wxSemaphoreInternal( initialcount, maxcount );
    if ( !m_internal->IsOk() )
    {
        delete m_internal;
        m_internal = NULL;
    }
}

wxSemaphore::~wxSemaphore()
{
    delete m_internal;
}

bool wxSemaphore::IsOk() const
{
    return m_internal != NULL;
}

wxSemaError wxSemaphore::Wait()
{
    wxCHECK_MSG( m_internal, wxSEMA_INVALID,
                 wxT("wxSemaphore::Wait(): not initialized") );

    return m_internal->Wait();
}

wxSemaError wxSemaphore::TryWait()
{
    wxCHECK_MSG( m_internal, wxSEMA_INVALID,
                 wxT("wxSemaphore::TryWait(): not initialized") );

    return m_internal->TryWait();
}

wxSemaError wxSemaphore::WaitTimeout(unsigned long milliseconds)
{
    wxCHECK_MSG( m_internal, wxSEMA_INVALID,
                 wxT("wxSemaphore::WaitTimeout(): not initialized") );

    return m_internal->WaitTimeout(milliseconds);
}

wxSemaError wxSemaphore::Post()
{
    wxCHECK_MSG( m_internal, wxSEMA_INVALID,
                 wxT("wxSemaphore::Post(): not initialized") );

    return m_internal->Post();
}

