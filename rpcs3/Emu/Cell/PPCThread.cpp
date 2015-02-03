#include "stdafx.h"
#include "PPCThread.h"
#include "Emu/Memory/Memory.h"

PPCThread* GetCurrentPPCThread()
{
	CPUThread* thread = GetCurrentCPUThread();

	if(!thread || (thread->GetType() != CPU_THREAD_PPU && thread->GetType() != CPU_THREAD_SPU && thread->GetType() != CPU_THREAD_RAW_SPU))
	{
		throw std::string("GetCurrentPPCThread: bad thread");
	}

	return (PPCThread*)thread;
}

PPCThread::PPCThread(CPUThreadType type) : CPUThread(type)
{
}

PPCThread::~PPCThread()
{
}

void PPCThread::DoReset()
{
}
