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

void PPCThread::InitStack()
{
	if(m_stack_addr) return;
	if(m_stack_size == 0) m_stack_size = 0x10000;
	m_stack_addr = (u32)Memory.StackMem.AllocAlign(m_stack_size, 0x100);
	/*
	m_stack_point += m_stack_size - 0x10;
	m_stack_point &= -0x10;
	vm::write64(m_stack_point, 0);
	m_stack_point -= 0x60;
	vm::write64(m_stack_point, m_stack_point + 0x60);
	*/
}

