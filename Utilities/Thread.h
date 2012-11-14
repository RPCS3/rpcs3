#pragma once
#include "Array.h"

class ThreadExec;

class ThreadBase
{
protected:
	wxString m_name;
	bool m_detached;

protected:
	ThreadBase(bool detached = true, const wxString& name = "Unknown ThreadBase")
		: m_detached(detached)
		, m_name(name)
		, m_executor(nullptr)
	{
	}

public:
	ThreadExec* m_executor;

	virtual void Task()=0;

	void Start();
	void Stop(bool wait = true);

	bool IsAlive();
	bool TestDestroy();
};

class ThreadExec : public wxThread
{
	ThreadBase* m_parent;
	wxSemaphore m_wait_for_exit;
	volatile bool m_alive;

public:
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
		Delete();
		if(wait && m_alive) m_wait_for_exit.Wait();
	}

	ExitCode Entry()
	{
		m_parent->Task();
		m_alive = false;
		m_wait_for_exit.Post();
		if(m_parent) m_parent->m_executor = nullptr;
		return (ExitCode)0;
	}
};

template<typename T> class MTPacketBuffer
{
protected:
	volatile bool m_busy;
	volatile u32 m_put, m_get;
	Array<u8> m_buffer;
	u32 m_max_buffer_size;

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
		m_put = m_get = 0;
		m_buffer.Clear();
		m_busy = false;
	}

	virtual void Push(const T& v) = 0;
	virtual T Pop() = 0;

	bool HasNewPacket() const { return m_put != m_get; }
	bool IsBusy() const { return m_busy; }
};

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