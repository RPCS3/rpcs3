#include "stdafx.h"

rTimer::rTimer()
{
	handle = reinterpret_cast<void*>(new wxTimer());
}

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

bool rThread::IsMain()
{
	return wxThread::IsMain();
}

void rYieldIfNeeded()
{
	wxYieldIfNeeded();
}