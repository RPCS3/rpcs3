#pragma once
#include "Array.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThreadExec;

class ThreadBase
{
protected:
	wxString m_name;
	bool m_detached;

public:
	wxMutex m_main_mutex;

protected:
	ThreadBase(bool detached = true, const wxString& name = "Unknown ThreadBase");

public:
	ThreadExec* m_executor;

	virtual void Task()=0;

	virtual void Start();
	virtual void Resume();
	virtual void Pause();
	virtual void Stop(bool wait = true);

	virtual bool Wait() const;
	virtual bool IsRunning() const;
	virtual bool IsPaused() const;
	virtual bool IsAlive() const;
	virtual bool TestDestroy() const;
	virtual wxString GetThreadName() const;
	virtual void SetThreadName(const wxString& name);
};

ThreadBase* GetCurrentNamedThread();

class ThreadExec : public wxThread
{
	wxCriticalSection m_wait_for_exit;
	volatile bool m_alive;

public:
	ThreadBase* m_parent;

	ThreadExec(bool detached, ThreadBase* parent)
		: wxThread(detached ? wxTHREAD_DETACHED : wxTHREAD_JOINABLE)
		, m_parent(parent)
		, m_alive(true)
	{
		Create();
		Run();
	}

	void Stop(bool wait = true)
	{
		if(!m_alive) return;

		m_parent = nullptr;

		if(wait)
		{
			Delete();
			//wxCriticalSectionLocker lock(m_wait_for_exit);
		}
	}

	ExitCode Entry()
	{
		//wxCriticalSectionLocker lock(m_wait_for_exit);
		m_parent->Task();
		m_alive = false;
		if(m_parent) m_parent->m_executor = nullptr;
		return (ExitCode)0;
	}
};

//ThreadBase* GetCurrentThread();

template<typename T> class MTPacketBuffer
{
protected:
	volatile bool m_busy;
	volatile u32 m_put, m_get;
	Array<u8> m_buffer;
	u32 m_max_buffer_size;
	mutable wxCriticalSection m_cs_main;

	void CheckBusy()
	{
		m_busy = m_put >= m_max_buffer_size;
	}

public:
	MTPacketBuffer(u32 max_buffer_size)
		: m_max_buffer_size(max_buffer_size)
	{
		Flush();
	}

	~MTPacketBuffer()
	{
		Flush();
	}

	void Flush()
	{
		wxCriticalSectionLocker lock(m_cs_main);
		m_put = m_get = 0;
		m_buffer.Clear();
		m_busy = false;
	}

private:
	virtual void _push(const T& v) = 0;
	virtual T _pop() = 0;

public:
	void Push(const T& v)
	{
		wxCriticalSectionLocker lock(m_cs_main);
		_push(v);
	}

	T Pop()
	{
		wxCriticalSectionLocker lock(m_cs_main);
		return _pop();
	}

	bool HasNewPacket() const { wxCriticalSectionLocker lock(m_cs_main); return m_put != m_get; }
	bool IsBusy() const { return m_busy; }
};

static __forceinline bool SemaphorePostAndWait(wxSemaphore& sem)
{
	if(sem.TryWait() != wxSEMA_BUSY) return false;

	sem.Post();
	sem.Wait();

	return true;
}

/*
class StepThread : public ThreadBase
{
	wxSemaphore m_main_sem;
	wxSemaphore m_destroy_sem;
	volatile bool m_exit;

protected:
	StepThread(const wxString& name = "Unknown StepThread")
		: ThreadBase(true, name)
		, m_exit(false)
	{
	}

	virtual ~StepThread() throw()
	{
	}

private:
	virtual void Task()
	{
		m_exit = false;

		while(!TestDestroy())
		{
			m_main_sem.Wait();

			if(TestDestroy() || m_exit) break;

			Step();
		}

		while(!TestDestroy()) Sleep(0);
		if(m_destroy_sem.TryWait() != wxSEMA_NO_ERROR) m_destroy_sem.Post();
	}

	virtual void Step()=0;

public:
	void DoStep()
	{
		if(IsRunning()) m_main_sem.Post();
	}

	void WaitForExit()
	{
		if(TestDestroy()) m_destroy_sem.Wait();
	}

	void WaitForNextStep()
	{
		if(!IsRunning()) return;

		while(m_main_sem.TryWait() != wxSEMA_NO_ERROR) Sleep(0);
	}

	void Exit(bool wait = false)
	{
		if(!IsAlive()) return;

		if(m_main_sem.TryWait() != wxSEMA_NO_ERROR)
		{
			m_exit = true;
			m_main_sem.Post();
		}

		Delete();

		if(wait) WaitForExit();
	}
};
*/