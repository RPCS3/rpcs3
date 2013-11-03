#include "stdafx.h"
#include "ARM9Thread.h"

ARM9Thread::ARM9Thread() : CPUThread(CPU_THREAD_ARM9)
{
}

void ARM9Thread::InitRegs()
{
}

void ARM9Thread::InitStack()
{
}

u64 ARM9Thread::GetFreeStackSize() const
{
	return GetStackSize() - m_stack_point;
}

void ARM9Thread::SetArg(const uint pos, const u64 arg)
{
	assert(0);
}

void ARM9Thread::SetPc(const u64 pc)
{
	PC = pc;
	nPC = pc + 2;
}

wxString ARM9Thread::RegsToString()
{
	return wxEmptyString;
}

wxString ARM9Thread::ReadRegString(wxString reg)
{
	return wxEmptyString;
}

bool ARM9Thread::WriteRegString(wxString reg, wxString value)
{
	return true;
}

void ARM9Thread::DoReset()
{
}

void ARM9Thread::DoRun()
{
}

void ARM9Thread::DoPause()
{
}

void ARM9Thread::DoResume()
{
}

void ARM9Thread::DoStop()
{
}

void ARM9Thread::DoCode()
{
}