#include "stdafx.h"
#include "ARMv7Thread.h"
#include "ARMv7Decoder.h"
#include "ARMv7DisAsm.h"
#include "ARMv7Interpreter.h"

ARMv7Thread::ARMv7Thread() : CPUThread(CPU_THREAD_ARMv7)
{
}

void ARMv7Thread::InitRegs()
{
}

void ARMv7Thread::InitStack()
{
}

u64 ARMv7Thread::GetFreeStackSize() const
{
	return GetStackSize() - (m_stack_point - GetStackAddr());
}

void ARMv7Thread::SetArg(const uint pos, const u64 arg)
{
	assert(0);
}

wxString ARMv7Thread::RegsToString()
{
	return wxEmptyString;
}

wxString ARMv7Thread::ReadRegString(wxString reg)
{
	return wxEmptyString;
}

bool ARMv7Thread::WriteRegString(wxString reg, wxString value)
{
	return true;
}

void ARMv7Thread::DoReset()
{
}

void ARMv7Thread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		//m_dec = new ARMv7Decoder(*new ARMv7DisAsm());
	break;

	case 1:
	case 2:
		m_dec = new ARMv7Decoder(*new ARMv7Interpreter(*this));
	break;
	}
}

void ARMv7Thread::DoPause()
{
}

void ARMv7Thread::DoResume()
{
}

void ARMv7Thread::DoStop()
{
}

void ARMv7Thread::DoCode()
{
}