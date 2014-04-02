#include "stdafx.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPUDisAsm.h"

SPUThread& GetCurrentSPUThread()
{
	PPCThread* thread = GetCurrentPPCThread();

	if(!thread || (thread->GetType() != CPU_THREAD_SPU && thread->GetType() != CPU_THREAD_RAW_SPU))
	{
		throw std::string("GetCurrentSPUThread: bad thread");
	}

	return *(SPUThread*)thread;
}

SPUThread::SPUThread(CPUThreadType type) : PPCThread(type)
{
	assert(type == CPU_THREAD_SPU || type == CPU_THREAD_RAW_SPU);

	group = nullptr;

	Reset();
}

SPUThread::~SPUThread()
{
}

void SPUThread::DoReset()
{
	PPCThread::DoReset();

	//reset regs
	memset(GPR, 0, sizeof(SPU_GPR_hdr) * 128);
}

void SPUThread::InitRegs()
{
	GPR[1]._u32[3] = 0x40000 - 120;
	GPR[3]._u64[1] = m_args[0];
	GPR[4]._u64[1] = m_args[1];
	GPR[5]._u64[1] = m_args[2];
	GPR[6]._u64[1] = m_args[3];

	cfg.Reset();

	dmac.ls_offset = m_offset;
	/*dmac.proxy_pos = 0;
	dmac.queue_pos = 0;
	dmac.proxy_lock = 0;
	dmac.queue_lock = 0;*/

	SPU.RunCntl.SetValue(SPU_RUNCNTL_STOP);
	SPU.Status.SetValue(SPU_STATUS_RUNNING);
	Prxy.QueryType.SetValue(0);
	MFC1.CMDStatus.SetValue(0);
	MFC2.CMDStatus.SetValue(0);
	//PC = SPU.NPC.GetValue();
}

u64 SPUThread::GetFreeStackSize() const
{
	return (GetStackAddr() + GetStackSize()) - GPR[1]._u32[3];
}

void SPUThread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		//m_dec = new SPUDecoder(*new SPUDisAsm());
	break;

	case 1:
	case 2:
		m_dec = new SPUDecoder(*new SPUInterpreter(*this));
	break;
	}

	//Pause();
	//Emu.Pause();
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
	m_dec = nullptr;
}

void SPUThread::DoClose()
{
	// disconnect all event ports
	if (Emu.IsStopped())
	{
		return;
	}
	for (u32 i = 0; i < 64; i++)
	{
		EventPort& port = SPUPs[i];
		SMutexLocker lock(port.mutex);
		if (port.eq)
		{
			port.eq->ports.remove(&port);
			port.eq = nullptr;
		}
	}
}