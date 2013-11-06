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
	memset(GPR, 0, sizeof(GPR[0]) * 15);
	APSR.APSR = 0;
	IPSR.IPSR = 0;
	SP = m_stack_point;
}

void ARMv7Thread::InitStack()
{
	if(!m_stack_addr)
	{
		m_stack_size = 0x10000;
		m_stack_addr = Memory.Alloc(0x10000, 1);
	}

	m_stack_point = m_stack_addr;
}

u64 ARMv7Thread::GetFreeStackSize() const
{
	return GetStackSize() - (SP - GetStackAddr());
}

void ARMv7Thread::SetArg(const uint pos, const u64 arg)
{
	assert(0);
}

wxString ARMv7Thread::RegsToString()
{
	wxString result;
	for(int i=0; i<15; ++i)
	{
		result += wxString::Format("%s\t= 0x%08x\n", g_arm_reg_name[i], GPR[i]);
	}

	result += wxString::Format("APSR\t= 0x%08x [N: %d, Z: %d, C: %d, V: %d, Q: %d]\n", APSR.APSR, APSR.N, APSR.Z, APSR.C, APSR.V, APSR.Q);
	
	return result;
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