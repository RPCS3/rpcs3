#include "stdafx.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPUDisAsm.h"

SPUThread& GetCurrentSPUThread()
{
	PPCThread* thread = GetCurrentPPCThread();

	if(!thread || thread->GetType() == PPC_THREAD_PPU) throw wxString("GetCurrentSPUThread: bad thread");

	return *(SPUThread*)thread;
}

SPUThread::SPUThread(PPCThreadType type) : PPCThread(type)
{
	Reset();
}

SPUThread::~SPUThread()
{
}

void SPUThread::DoReset()
{
	//reset regs
	for(u32 i=0; i<128; ++i) GPR[i].Reset();
}

void SPUThread::InitRegs()
{
	//GPR[1]._u64[0] = stack_point;
	GPR[3]._u64[1] = m_args[0];
	GPR[4]._u64[1] = m_args[1];
	GPR[5]._u64[1] = m_args[2];
	GPR[6]._u64[1] = m_args[3];
}

u64 SPUThread::GetFreeStackSize() const
{
	return (GetStackAddr() + GetStackSize()) - GPR[1]._u64[0];
}

void SPUThread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		m_dec = new SPU_Decoder(*new SPU_DisAsm(*this));
	break;

	case 1:
	case 2:
		m_dec = new SPU_Decoder(*new SPU_Interpreter(*this));
	break;
	}
}

void SPUThread::DoResume()
{
}

void SPUThread::DoPause()
{
}

void SPUThread::DoStop()
{
	delete m_dec;
	m_dec = 0;
}

void SPUThread::DoCode(const s32 code)
{
	m_dec->Decode(code);
}
