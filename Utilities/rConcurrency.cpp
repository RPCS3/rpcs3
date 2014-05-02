#include "stdafx.h"

rSemaphore::rSemaphore()
{
	handle = reinterpret_cast<void*>(new wxSemaphore());
}

//rSemaphore::rSemaphore(rSemaphore& other)
//{
//	handle = reinterpret_cast<void*>(new wxSemaphore(*reinterpret_cast<wxSemaphore*>(other.handle)));
//}

rSemaphore::~rSemaphore()
{
	delete reinterpret_cast<wxSemaphore*>(handle);
}

rSemaphore::rSemaphore(int initial_count, int max_count)
{
	handle = reinterpret_cast<void*>(new wxSemaphore(initial_count,max_count));
}

void rSemaphore::Wait()
{
	reinterpret_cast<wxSemaphore*>(handle)->Wait();
}

rSemaStatus rSemaphore::TryWait()
{
	wxSemaError err = reinterpret_cast<wxSemaphore*>(handle)->TryWait();
	if (err == wxSEMA_BUSY)
	{
		return rSEMA_BUSY;
	}
	else
	{
		return rSEMA_OTHER;
	}
}

void rSemaphore::Post()
{
	reinterpret_cast<wxSemaphore*>(handle)->Post();
}

void rSemaphore::WaitTimeout(u64 timeout)
{
	reinterpret_cast<wxSemaphore*>(handle)->WaitTimeout(timeout);
}

rCriticalSection::rCriticalSection()
{
	handle = reinterpret_cast<void*>(new wxCriticalSection());
}

//rCriticalSection::rCriticalSection(rCriticalSection&)
//{
//	handle = reinterpret_cast<void*>(new wxCriticalSection(*reinterpret_cast<wxCriticalSection*>(other.handle)));
//}

rCriticalSection::~rCriticalSection()
{
	delete reinterpret_cast<wxCriticalSection*>(handle);
}

void rCriticalSection::Enter()
{
	reinterpret_cast<wxCriticalSection*>(handle)->Enter();
}

void rCriticalSection::Leave()
{
	reinterpret_cast<wxCriticalSection*>(handle)->Leave();
}

rTimer::rTimer()
{
	handle = reinterpret_cast<void*>(new wxTimer());
}

//rTimer::rTimer(rTimer&)
//{
//	handle = reinterpret_cast<void*>(new wxTimer(*reinterpret_cast<wxTimer*>(other.handle)));
//}

rTimer::~rTimer()
{
	delete reinterpret_cast<wxTimer*>(handle);
}

void rTimer::Start()
{
	reinterpret_cast<wxTimer*>(handle)->Start();
}

void rTimer::Stop()
{
	reinterpret_cast<wxTimer*>(handle)->Stop();
}

void rSleep(u32 time)
{
	wxSleep(time);
}

void rMicroSleep(u64 time)
{
	wxMicroSleep(time);
}

rCriticalSectionLocker::rCriticalSectionLocker(const rCriticalSection &sec)
{
	handle = reinterpret_cast<void*>(new wxCriticalSectionLocker(*reinterpret_cast<wxCriticalSection*>(sec.handle)));
}


rCriticalSectionLocker::~rCriticalSectionLocker()
{
	delete reinterpret_cast<wxCriticalSectionLocker*>(handle);
}

bool rThread::IsMain()
{
	return wxThread::IsMain();
}

void rYieldIfNeeded()
{
	wxYieldIfNeeded();
}