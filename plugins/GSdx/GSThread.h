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

#ifdef _WINDOWS

typedef void (WINAPI * InitializeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef void (WINAPI * WakeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef void (WINAPI * WakeAllConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
typedef void (WINAPI * SleepConditionVariableSRWPtr)(CONDITION_VARIABLE* ConditionVariable, SRWLOCK* SRWLock, DWORD dwMilliseconds, ULONG Flags);
typedef void (WINAPI * InitializeSRWLockPtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * AcquireSRWLockExclusivePtr)(SRWLOCK* SRWLock);
typedef void (WINAPI * ReleaseSRWLockExclusivePtr)(SRWLOCK* SRWLock);

extern InitializeConditionVariablePtr pInitializeConditionVariable;
extern WakeConditionVariablePtr pWakeConditionVariable;
extern WakeAllConditionVariablePtr pWakeAllConditionVariable;
extern SleepConditionVariableSRWPtr pSleepConditionVariableSRW;
extern InitializeSRWLockPtr pInitializeSRWLock;;
extern AcquireSRWLockExclusivePtr pAcquireSRWLockExclusive;
extern ReleaseSRWLockExclusivePtr pReleaseSRWLockExclusive;

class GSThread
{
    DWORD m_ThreadId;
    HANDLE m_hThread;

	static DWORD WINAPI StaticThreadProc(void* lpParam);

protected:
	virtual void ThreadProc() = 0;

	void CreateThread();
	void CloseThread();

public:
	GSThread();
	virtual ~GSThread();
};

class GSCritSec
{
    CRITICAL_SECTION m_cs;

public:
    GSCritSec() {InitializeCriticalSection(&m_cs);}
    ~GSCritSec() {DeleteCriticalSection(&m_cs);}

    void Lock() {EnterCriticalSection(&m_cs);}
    bool TryLock() {return TryEnterCriticalSection(&m_cs) == TRUE;}
    void Unlock() {LeaveCriticalSection(&m_cs);}
};

class GSEvent
{
protected:
    HANDLE m_hEvent;

public:
	GSEvent(bool manual = false, bool initial = false) {m_hEvent = CreateEvent(NULL, manual, initial, NULL);}
	~GSEvent() {CloseHandle(m_hEvent);}

    void Set() {SetEvent(m_hEvent);}
	void Reset() {ResetEvent(m_hEvent);}
    bool Wait() {return WaitForSingleObject(m_hEvent, INFINITE) == WAIT_OBJECT_0;}
};

class GSCondVarLock
{
	SRWLOCK m_lock;

public:
	GSCondVarLock() {pInitializeSRWLock(&m_lock);}

	void Lock() {pAcquireSRWLockExclusive(&m_lock);}
	void Unlock() {pReleaseSRWLockExclusive(&m_lock);}
		
	operator SRWLOCK* () {return &m_lock;}
};

class GSCondVar
{
	CONDITION_VARIABLE m_cv;

public:
	GSCondVar() {pInitializeConditionVariable(&m_cv);}

	void Set() {pWakeConditionVariable(&m_cv);}
	void Wait(GSCondVarLock& lock) {pSleepConditionVariableSRW(&m_cv, lock, INFINITE, 0);}

	operator CONDITION_VARIABLE* () {return &m_cv;}
};

#else

#include <pthread.h>
#include <semaphore.h>

class GSThread
{
    pthread_attr_t m_thread_attr;
    pthread_t m_thread;

    static void* StaticThreadProc(void* param);

protected:
	virtual void ThreadProc() = 0;

	void CreateThread();
	void CloseThread();

public:
	GSThread();
	virtual ~GSThread();
};

class GSCritSec
{
    pthread_mutexattr_t m_mutex_attr;
    pthread_mutex_t m_mutex;

public:
    GSCritSec()
    {
        pthread_mutexattr_init(&m_mutex_attr);
        pthread_mutexattr_settype(&m_mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
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
};

class GSEvent
{
protected:
    sem_t m_sem;

public:
    GSEvent() {sem_init(&m_sem, 0, 0);}
    ~GSEvent() {sem_destroy(&m_sem);}

    void Set() {sem_post(&m_sem);}
    bool Wait() {return sem_wait(&m_sem) == 0;}
};

class GSCondVarLock
{
	pthread_mutex_t m_lock;

public:
	GSCondVarLock() {pthread_mutex_init(&m_lock, NULL);}
	virtual ~GSCondVarLock() {pthread_mutex_destroy(&m_lock);}

	void Lock() {pthread_mutex_lock(&m_lock);}
	void Unlock() {pthread_mutex_unlock(&m_lock);}
		
	operator pthread_mutex_t* () {return &m_lock;}
};

class GSCondVar
{
	pthread_cond_t m_cv;

public:
	GSCondVar() {pthread_cond_init(&m_cv, NULL);}
	virtual ~GSCondVar() {pthread_cond_destroy(&m_cv);}

	void Set() {pthread_cond_signal(&m_cv);}
	void Wait(GSCondVarLock& lock) {pthread_cond_wait(&m_cv, lock);}

	operator pthread_cond_t* () {return &m_cv;}
};

#endif

class GSAutoLock
{
protected:
    GSCritSec* m_cs;

public:
    GSAutoLock(GSCritSec* cs) {m_cs = cs; m_cs->Lock();}
    ~GSAutoLock() {m_cs->Unlock();}
};

class GSEventSpin
{
protected:
    volatile long m_sync;
	volatile bool m_manual;

public:
	GSEventSpin(bool manual = false, bool initial = false) {m_sync = initial ? 1 : 0; m_manual = manual;}
	~GSEventSpin() {}

    void Set() {_interlockedbittestandset(&m_sync, 0);}
	void Reset() {_interlockedbittestandreset(&m_sync, 0);}
    bool Wait() 
	{
		if(m_manual) while(!m_sync) _mm_pause();
		else while(!_interlockedbittestandreset(&m_sync, 0)) _mm_pause();
		return true;
	}
};

template<class T> class GSJobQueue : private GSThread
{
protected:
	int m_count;
	queue<T> m_queue;
	volatile bool m_exit;
	struct {GSCritSec lock; GSEvent notempty; volatile long count;} m_ev;
	struct {GSCondVar notempty, empty; GSCondVarLock lock; bool available;} m_cv;

	void ThreadProc()
	{
		if(m_cv.available)
		{
			m_cv.lock.Lock();

			while(true)
			{
				while(m_queue.empty())
				{
					m_cv.notempty.Wait(m_cv.lock);

					if(m_exit) {m_cv.lock.Unlock(); return;}
				}

				{
					// NOTE: this is scoped because we must make sure the last item is no longer around when Wait detects an empty queue

					T item = m_queue.front();

					m_cv.lock.Unlock();

					Process(item);

					m_cv.lock.Lock();

					m_queue.pop();
				}

				if(m_queue.empty())
				{
					m_cv.empty.Set();
				}
			}
		}
		else
		{
			m_ev.lock.Lock();

			while(true)
			{
				while(m_queue.empty())
				{
					m_ev.lock.Unlock();

					m_ev.notempty.Wait();

					if(m_exit) {return;}

					m_ev.lock.Lock();
				}

				{
					// NOTE: this is scoped because we must make sure the last item is no longer around when Wait detects an empty queue

					T item = m_queue.front();

					m_ev.lock.Unlock();

					Process(item);

					m_ev.lock.Lock();

					m_queue.pop();
				}

				_InterlockedDecrement(&m_ev.count);
			}

		}
	}

public:
	GSJobQueue()
		: m_count(0)
		, m_exit(false)
	{
		m_ev.count = 0;

		#ifdef _WINDOWS

		m_cv.available = pInitializeConditionVariable != NULL;

		#elif defined(_LINUX)
	
		m_cv.available = true;

		#endif

		CreateThread();
	}
	
	virtual ~GSJobQueue()
	{
		m_exit = true;

		if(m_cv.available)
		{
			m_cv.notempty.Set();
		}
		else
		{
			m_ev.notempty.Set();
		}
	}

	int GetCount() const
	{
		return m_count;
	}

	virtual void Push(const T& item)
	{
		if(m_cv.available)
		{
			m_cv.lock.Lock();
	
			m_queue.push(item);

			m_cv.lock.Unlock();

			m_cv.notempty.Set();
		}
		else
		{
			GSAutoLock l(&m_ev.lock);

			m_queue.push(item);

			_InterlockedIncrement(&m_ev.count);

			m_ev.notempty.Set();
		}

		m_count++;
	}

	virtual void Wait()
	{
		if(m_cv.available)
		{
			m_cv.lock.Lock();

			while(!m_queue.empty()) 
			{
				m_cv.empty.Wait(m_cv.lock);
			}

			m_cv.lock.Unlock();
		}
		else
		{
			// NOTE: it is the safest to have our own counter because m_queue.pop() might decrement its own before the last item runs out of its scope and gets destroyed (implementation dependent)

			while(m_ev.count > 0) _mm_pause();
		}

		m_count++;
	}

	virtual void Process(T& item) = 0;
};
