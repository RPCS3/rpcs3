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
	#ifdef _WINDOWS
	struct {SRWLOCK lock; CONDITION_VARIABLE notempty, empty; bool available;} m_cv;
	HMODULE m_kernel32;
	typedef void (WINAPI * InitializeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
	typedef void (WINAPI * WakeConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
	typedef void (WINAPI * WakeAllConditionVariablePtr)(CONDITION_VARIABLE* ConditionVariable);
	typedef void (WINAPI * SleepConditionVariableSRWPtr)(CONDITION_VARIABLE* ConditionVariable, SRWLOCK* SRWLock, DWORD dwMilliseconds, ULONG Flags);
	typedef void (WINAPI * InitializeSRWLockPtr)(SRWLOCK* SRWLock);
	typedef void (WINAPI * AcquireSRWLockExclusivePtr)(SRWLOCK* SRWLock);
	typedef void (WINAPI * ReleaseSRWLockExclusivePtr)(SRWLOCK* SRWLock);
	InitializeConditionVariablePtr pInitializeConditionVariable;
	WakeConditionVariablePtr pWakeConditionVariable;
	WakeAllConditionVariablePtr pWakeAllConditionVariable;
	SleepConditionVariableSRWPtr pSleepConditionVariableSRW;
	InitializeSRWLockPtr pInitializeSRWLock;;
	AcquireSRWLockExclusivePtr pAcquireSRWLockExclusive;
	ReleaseSRWLockExclusivePtr pReleaseSRWLockExclusive;
	#elif defined(_LINUX)
	struct {pthread_mutex_t lock; pthread_cond_t notempty, empty; bool available;} m_cv;
	#endif

	void ThreadProc()
	{

		if(m_cv.available)
		{
		#ifdef _WINDOWS
			pAcquireSRWLockExclusive(&m_cv.lock);

			while(true)
			{
				while(m_queue.empty())
				{
					pSleepConditionVariableSRW(&m_cv.notempty, &m_cv.lock, INFINITE, 0);

					if(m_exit) {pReleaseSRWLockExclusive(&m_cv.lock); return;}
				}

				{
					// NOTE: this is scoped because we must make sure the last item is no longer around when Wait detects an empty queue

					T item = m_queue.front();

					pReleaseSRWLockExclusive(&m_cv.lock);

					Process(item);

					pAcquireSRWLockExclusive(&m_cv.lock);

					m_queue.pop();
				}

				if(m_queue.empty())
				{
					pWakeConditionVariable(&m_cv.empty);
				}
			}
		#elif defined(_LINUX)
			pthread_mutex_lock(&m_cv.lock);

			while(true)
			{
				while(m_queue.empty())
				{
					pthread_cond_wait(&m_cv.notempty, &m_cv.lock);

					if(m_exit) {pthread_mutex_unlock(&m_cv.lock); return;}
				}

				{
					// NOTE: this is scoped because we must make sure the last item is no longer around when Wait detects an empty queue
					T item = m_queue.front();

					pthread_mutex_unlock(&m_cv.lock);

					Process(item);

					pthread_mutex_lock(&m_cv.lock);

					m_queue.pop();
				}

				if(m_queue.empty())
				{
					pthread_cond_signal(&m_cv.empty);
				}
			}
		#endif
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

		m_cv.available = false;

		m_kernel32 = LoadLibrary("kernel32.dll");

		pInitializeConditionVariable = (InitializeConditionVariablePtr)GetProcAddress(m_kernel32, "InitializeConditionVariable");
		pWakeConditionVariable = (WakeConditionVariablePtr)GetProcAddress(m_kernel32, "WakeConditionVariable");
		pWakeAllConditionVariable = (WakeAllConditionVariablePtr)GetProcAddress(m_kernel32, "WakeAllConditionVariable");
		pSleepConditionVariableSRW = (SleepConditionVariableSRWPtr)GetProcAddress(m_kernel32, "SleepConditionVariableSRW");
		pInitializeSRWLock = (InitializeSRWLockPtr)GetProcAddress(m_kernel32, "InitializeSRWLock");
		pAcquireSRWLockExclusive = (AcquireSRWLockExclusivePtr)GetProcAddress(m_kernel32, "AcquireSRWLockExclusive");
		pReleaseSRWLockExclusive = (ReleaseSRWLockExclusivePtr)GetProcAddress(m_kernel32, "ReleaseSRWLockExclusive");

		if(pInitializeConditionVariable != NULL)
		{
			pInitializeSRWLock(&m_cv.lock);
			pInitializeConditionVariable(&m_cv.notempty);
			pInitializeConditionVariable(&m_cv.empty);
			
			m_cv.available = true;
		}

		#elif defined(_LINUX)
			m_cv.available = true;
			// FIXME attribute
			pthread_cond_init(&m_cv.notempty, NULL);
			pthread_cond_init(&m_cv.empty, NULL);
			pthread_mutex_init(&m_cv.lock, NULL);
		#endif

		CreateThread();
	}
	
	virtual ~GSJobQueue()
	{
		m_exit = true;


		if(m_cv.available)
		{
		#ifdef _WINDOWS
			pWakeConditionVariable(&m_cv.notempty);
		#elif defined(_LINUX)
			pthread_cond_signal(&m_cv.notempty);

			pthread_mutex_destroy(&m_cv.lock);
			pthread_cond_destroy(&m_cv.notempty);
			pthread_cond_destroy(&m_cv.empty);
		#endif
		}
		else
		{
			m_ev.notempty.Set();
		}


		#ifdef _WINDOWS
		if(m_kernel32 != NULL)
		{
			FreeLibrary(m_kernel32); // lol, decrement the refcount anyway
		}
		#endif

	}

	int GetCount() const
	{
		return m_count;
	}

	virtual void Push(const T& item)
	{

		if(m_cv.available)
		{
		#ifdef _WINDOWS
			pAcquireSRWLockExclusive(&m_cv.lock);

			m_queue.push(item);

			pReleaseSRWLockExclusive(&m_cv.lock);

			pWakeConditionVariable(&m_cv.notempty);
		#elif defined(_LINUX)
			pthread_mutex_lock(&m_cv.lock);

			m_queue.push(item);

			pthread_mutex_unlock(&m_cv.lock);

			pthread_cond_signal(&m_cv.notempty);
		#endif
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
		#ifdef _WINDOWS
			pAcquireSRWLockExclusive(&m_cv.lock);

			while(!m_queue.empty()) 
			{
				pSleepConditionVariableSRW(&m_cv.empty, &m_cv.lock, INFINITE, 0);
			}

			pReleaseSRWLockExclusive(&m_cv.lock);
		#elif defined(_LINUX)
			pthread_mutex_lock(&m_cv.lock);

			while(!m_queue.empty()) 
			{
				pthread_cond_wait(&m_cv.empty, &m_cv.lock);
			}

			pthread_mutex_unlock(&m_cv.lock);
		#endif
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
