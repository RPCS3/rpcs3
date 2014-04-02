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

	m_stack_point = m_stack_addr + m_stack_size;
}

u64 ARMv7Thread::GetFreeStackSize() const
{
	return SP - GetStackAddr();
}

void ARMv7Thread::SetArg(const uint pos, const u64 arg)
{
	assert(0);
}

std::string ARMv7Thread::RegsToString()
{
	std::string result = "Registers:\n=========\n";
	for(int i=0; i<15; ++i)
	{
		result += fmt::Format("%s\t= 0x%08x\n", g_arm_reg_name[i], GPR[i]);
	}

	result += fmt::Format("APSR\t= 0x%08x [N: %d, Z: %d, C: %d, V: %d, Q: %d]\n", 
		APSR.APSR,
		fmt::by_value(APSR.N),
		fmt::by_value(APSR.Z),
		fmt::by_value(APSR.C),
		fmt::by_value(APSR.V),
		fmt::by_value(APSR.Q));
	
	return result;
}

std::string ARMv7Thread::ReadRegString(const std::string& reg)
{
	return "";
}

bool ARMv7Thread::WriteRegString(const std::string& reg, std::string value)
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