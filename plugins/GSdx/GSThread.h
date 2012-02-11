/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSdx.h"

class IGSThread
{
protected:
	virtual void ThreadProc() = 0;
};

class IGSLock
{
public:
    virtual void Lock() = 0;
    virtual bool TryLock() = 0;
    virtual void Unlock() = 0;
};

class IGSEvent
{
public:
    virtual void Set() = 0;
	virtual bool Wait(IGSLock* l) = 0;
};

#ifdef _WINDOWS

typedef void (WINAPI * InitializeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef void (WINAPI * WakeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef void (WINAPI * WakeAllConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef BOOL (WINAPI * SleepConditionVariableSRWPtr)(CONDITION_VARIABLE* ConditionVariable, SRWLOCK* SRWLock, DWORD dwMilliseconds, ULONG Flags);
typedef void (WINAPI * InitializeSRWLockPtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * AcquireSRWLockExclusivePtr)(SRWLOCK* SRWLock);
typedef BOOLEAN (WINAPI * TryAcquireSRWLockExclusivePtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * ReleaseSRWLockExclusivePtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * AcquireSRWLockSharedPtr)(SRWLOCK* SRWLock);
typedef BOOLEAN (WINAPI * TryAcquireSRWLockSharedPtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * ReleaseSRWLockSharedPtr)(SRWLOCK* SRWLock);

extern InitializeConditionVariablePtr pInitializeConditionVariable;
extern WakeConditionVariablePtr pWakeConditionVariable;
extern WakeAllConditionVariablePtr pWakeAllConditionVariable;
extern SleepConditionVariableSRWPtr pSleepConditionVariableSRW;
extern InitializeSRWLockPtr pInitializeSRWLock;
extern AcquireSRWLockExclusivePtr pAcquireSRWLockExclusive;
extern TryAcquireSRWLockExclusivePtr pTryAcquireSRWLockExclusive;
extern ReleaseSRWLockExclusivePtr pReleaseSRWLockExclusive;
extern AcquireSRWLockSharedPtr pAcquireSRWLockShared;
extern TryAcquireSRWLockSharedPtr pTryAcquireSRWLockShared;
extern ReleaseSRWLockSharedPtr pReleaseSRWLockShared;

class GSThread : public IGSThread
{
    DWORD m_ThreadId;
    HANDLE m_hThread;

	static DWORD WINAPI StaticThreadProc(void* lpParam);

protected:
	void CreateThread();
	void CloseThread();

public:
	GSThread();
	virtual ~GSThread();
};

class GSCritSec : public IGSLock
{
    CRITICAL_SECTION m_cs;

public:
    GSCritSec() {InitializeCriticalSection(&m_cs);}
    ~GSCritSec() {DeleteCriticalSection(&m_cs);}

	void Lock() {EnterCriticalSection(&m_cs);}
	bool TryLock() {return TryEnterCriticalSection(&m_cs) == TRUE;}
	void Unlock() {LeaveCriticalSection(&m_cs);}
};

class GSEvent : public IGSEvent
{
protected:
    HANDLE m_hEvent;

public:
	GSEvent() {m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);}
	~GSEvent() {CloseHandle(m_hEvent);}

    void Set() {SetEvent(m_hEvent);}
	bool Wait(IGSLock* l) {if(l) l->Unlock(); bool b = WaitForSingleObject(m_hEvent, INFINITE) == WAIT_OBJECT_0; if(l) l->Lock(); return b;}
};

class GSCondVarLock : public IGSLock
{
	SRWLOCK m_lock;

public:
	GSCondVarLock() {pInitializeSRWLock(&m_lock);}

	void Lock() {pAcquireSRWLockExclusive(&m_lock);}
	bool TryLock() {return pTryAcquireSRWLockExclusive(&m_lock) == TRUE;}
	void Unlock() {pReleaseSRWLockExclusive(&m_lock);}

	operator SRWLOCK* () {return &m_lock;}
};

class GSCondVar : public IGSEvent
{
	CONDITION_VARIABLE m_cv;

public:
	GSCondVar() {pInitializeConditionVariable(&m_cv);}

	void Set() {pWakeConditionVariable(&m_cv);}
	bool Wait(IGSLock* l) {return pSleepConditionVariableSRW(&m_cv, *(GSCondVarLock*)l, INFINITE, 0) != 0;}

	operator CONDITION_VARIABLE* () {return &m_cv;}
};

#else

#include <pthread.h>
#include <semaphore.h>

class GSThread : public IGSThread
{
    pthread_attr_t m_thread_attr;
    pthread_t m_thread;

    static void* StaticThreadProc(void* param);

protected:
	void CreateThread();
	void CloseThread();

public:
	GSThread();
	virtual ~GSThread();
};

class GSCritSec : public IGSLock
{
    pthread_mutexattr_t m_mutex_attr;
    pthread_mutex_t m_mutex;

public:
    GSCritSec(bool recursive = true)
    {
        pthread_mutexattr_init(&m_mutex_attr);
        pthread_mutexattr_settype(&m_mutex_attr, recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&m_mutex, &m_mutex_attr);
    }

    ~GSCritSec()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_mutexattr_destroy(&m_mutex_attr);
    }

    void Lock() {pthread_mutex_lock(&m_mutex);}
    bool TryLock() {return pthread_mutex_trylock(&m_mutex) == 0;}
    void Unlock() {pthread_mutex_unlock(&m_mutex);}

	operator pthread_mutex_t* () {return &m_mutex;}
};

class GSEvent : public IGSEvent
{
protected:
    sem_t m_sem;

public:
    GSEvent() {sem_init(&m_sem, 0, 0);}
    ~GSEvent() {sem_destroy(&m_sem);}

    void Set() {sem_post(&m_sem);}
	bool Wait(IGSLock* l) {if(l) l->Unlock(); bool b = sem_wait(&m_sem) == 0; if(l) l->Lock(); return b;}
};

class GSCondVarLock : public GSCritSec
{
public:
	GSCondVarLock() : GSCritSec(false)
	{
	}
};

class GSCondVar : public IGSEvent
{
	pthread_cond_t m_cv;
	pthread_condattr_t m_cv_attr;

public:
	GSCondVar() 
	{
		pthread_condattr_init(&m_cv_attr);
		pthread_cond_init(&m_cv, &m_cv_attr);
	}

	virtual ~GSCondVar() 
	{
		pthread_condattr_destroy(&m_cv_attr);
		pthread_cond_destroy(&m_cv);
	}

	void Set() {pthread_cond_signal(&m_cv);}
	bool Wait(IGSLock* l) {pthread_cond_wait(&m_cv, *(GSCondVarLock*)l) == 0;}

	operator pthread_cond_t* () {return &m_cv;}
};

#endif

class GSAutoLock
{
    IGSLock* m_lock;

public:
    GSAutoLock(IGSLock* l) {(m_lock = l)->Lock();}
    ~GSAutoLock() {m_lock->Unlock();}
};

template<class T> class GSJobQueue : private GSThread
{
protected:
	queue<T> m_queue;
	volatile long m_count; // NOTE: it is the safest to have our own counter because m_queue.pop() might decrement its own before the last item runs out of its scope and gets destroyed (implementation dependent)
	volatile bool m_exit;
	IGSEvent* m_notempty;
	IGSEvent* m_empty;
	IGSLock* m_lock;

	void ThreadProc()
	{
		m_lock->Lock();

		while(true)
		{
			while(m_queue.empty())
			{
				m_notempty->Wait(m_lock);

				if(m_exit) {m_lock->Unlock(); return;}
			}

			T& item = m_queue.front();

			m_lock->Unlock();

			Process(item);

			m_lock->Lock();

			m_queue.pop();

			if(--m_count == 0)
			{
				m_empty->Set();
			}
		}
	}

public:
	GSJobQueue()
		: m_count(0)
		, m_exit(false)
	{
		bool condvar = !!theApp.GetConfig("condvar", 1);

		#ifdef _WINDOWS

		if(pInitializeConditionVariable == NULL) 
		{
			condvar = false;
		}

		#endif

		if(condvar)
		{
			m_notempty = new GSCondVar();
			m_empty = new GSCondVar();
			m_lock = new GSCondVarLock();
		}
		else
		{
			m_notempty = new GSEvent();
			m_empty = new GSEvent();
			m_lock = new GSCritSec();
		}

		CreateThread();
	}
	
	virtual ~GSJobQueue()
	{
		m_exit = true;

		m_notempty->Set();

		CloseThread();

		delete m_notempty;
		delete m_empty;
		delete m_lock;
	}

	bool IsEmpty() const
	{
		ASSERT(m_count >= 0);

		return m_count == 0;
	}

	void Push(const T& item)
	{
		m_lock->Lock();
	
		m_queue.push(item);

		if(m_count++ == 0)
		{
			m_notempty->Set();
		}

		m_lock->Unlock();
	}

	void Wait()
	{
		if(m_count > 0)
		{
			m_lock->Lock();

			while(m_count != 0) 
			{
				m_empty->Wait(m_lock);
			}

			ASSERT(m_queue.empty());
	
			m_lock->Unlock();
		}
	}

	virtual void Process(T& item) = 0;
};
