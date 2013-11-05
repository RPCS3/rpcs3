#include "stdafx.h"
#include "ARM9Thread.h"
#include "ARM9Decoder.h"
#include "ARM9DisAsm.h"
#include "ARM9Interpreter.h"

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
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		//m_dec = new PPUDecoder(*new PPUDisAsm());
	break;

	case 1:
	case 2:
		m_dec = new ARM9Decoder(*new ARM9Interpreter(*this));
	break;
	}
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