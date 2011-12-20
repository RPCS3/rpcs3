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

// TODO: pthreads version (needs manual-reset event)

template<
	class T, 
	class ENQUEUE_EVENT = GSEvent, 
	class DEQUEUE_EVENT = GSEvent> 
class GSQueue : public GSCritSec
{
	std::list<T> m_queue;
	HANDLE m_put;
	HANDLE m_get;
	ENQUEUE_EVENT m_enqueue;
	DEQUEUE_EVENT m_dequeue;
	long m_count;
 
public:
	GSQueue(long count) 
		: m_enqueue(true)
		, m_dequeue(true)
		, m_count(count)
	{
		m_put = CreateSemaphore(NULL, count, count, NULL);
		m_get = CreateSemaphore(NULL, 0, count, NULL);

		m_dequeue.Set();
	}

	virtual ~GSQueue()
	{
		CloseHandle(m_put);
		CloseHandle(m_get);
	}
 
	size_t GetCount() const
	{
		// GSAutoLock cAutoLock(this);
 
		return m_queue.size();
	}
 
	size_t GetMaxCount() const
	{
		// GSAutoLock cAutoLock(this);
 
		return (size_t)m_count;
	}
 
	ENQUEUE_EVENT& GetEnqueueEvent()
	{
		return m_enqueue;
	}
 
	DEQUEUE_EVENT& GetDequeueEvent()
	{
		return m_dequeue;
	}
 
	void Enqueue(T item)
	{
		WaitForSingleObject(m_put, INFINITE);
 
		{
			GSAutoLock cAutoLock(this);
 
			m_queue.push_back(item);
 
			m_enqueue.Set();
			m_dequeue.Reset();
		}
 
		ReleaseSemaphore(m_get, 1, NULL);
	}
 
	T Dequeue()
	{
		T item;
 
		WaitForSingleObject(m_get, INFINITE);
 
		{
			GSAutoLock cAutoLock(this);
 
			item = m_queue.front();

			m_queue.pop_front();
 
			if(m_queue.empty())
			{
				m_enqueue.Reset();
				m_dequeue.Set();
			}
		}
 
		ReleaseSemaphore(m_put, 1, NULL);
 
		return item;
	}

	T Peek() // lock on "this"
	{
		return m_queue.front();
	}
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
