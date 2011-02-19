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

class GSAutoResetEvent
{
protected:
    HANDLE m_hEvent;

public:
	GSAutoResetEvent() {m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);}
	~GSAutoResetEvent() {CloseHandle(m_hEvent);}

    void Set() {SetEvent(m_hEvent);}
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

class GSAutoResetEvent
{
protected:
    sem_t m_sem;

public:
    GSAutoResetEvent() {sem_init(&m_sem, 0, 0);}
    ~GSAutoResetEvent() {sem_destroy(&m_sem);}

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
