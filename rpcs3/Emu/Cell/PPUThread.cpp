#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/PPULLVMRecompiler.h"

PPUThread& GetCurrentPPUThread()
{
	PPCThread* thread = GetCurrentPPCThread();

	if(!thread || thread->GetType() != CPU_THREAD_PPU) throw std::string("GetCurrentPPUThread: bad thread");

	return *(PPUThread*)thread;
}

PPUThread::PPUThread() : PPCThread(CPU_THREAD_PPU)
{
	owned_mutexes = 0;
	Reset();
}

PPUThread::~PPUThread()
{
}

void PPUThread::DoReset()
{
	PPCThread::DoReset();

	//reset regs
	memset(VPR,  0, sizeof(VPR));
	memset(FPR,  0, sizeof(FPR));
	memset(GPR,  0, sizeof(GPR));
	memset(SPRG, 0, sizeof(SPRG));

	CR.CR       = 0;
	LR          = 0;
	CTR         = 0;
	USPRG0      = 0;
	TB          = 0;
	XER.XER     = 0;
	FPSCR.FPSCR = 0;
	VSCR.VSCR   = 0;

	cycle = 0;
}

void PPUThread::InitRegs()
{
	const u32 pc = entry ? vm::read32(entry) : 0;
	const u32 rtoc = entry ? vm::read32(entry + 4) : 0;

	//ConLog.Write("entry = 0x%x", entry);
	//ConLog.Write("rtoc = 0x%x", rtoc);

	SetPc(pc);

	/*
	const s32 thread_num = Emu.GetCPU().GetThreadNumById(GetType(), GetId());

	if(thread_num < 0)
	{
		LOG_ERROR(PPU, "GetThreadNumById failed.");
		Emu.Pause();
		return;
	}
	*/

	/*
	const s32 tls_size = Emu.GetTLSFilesz() * thread_num;

	if(tls_size >= Emu.GetTLSMemsz())
	{
		LOG_ERROR(PPU, "Out of TLS memory.");
		Emu.Pause();
		return;
	}
	*/

	GPR[1] = AlignAddr(m_stack_addr + m_stack_size, 0x200) - 0x200;
	GPR[2] = rtoc;
	GPR[13] = Memory.PRXMem.GetStartAddr() + 0x7060;

	LR = Emu.GetPPUThreadExit();
	CTR = PC;
	CR.CR = 0x22000082;
	VSCR.NJ = 1;
	TB = 0;
}

void PPUThread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		//m_dec = new PPUDecoder(*new PPUDisAsm());
	break;

	case 1:
	{
		auto ppui = new PPUInterpreter(*this);
		m_dec = new PPUDecoder(ppui);
	}
	break;

	case 2:
#ifdef PPU_LLVM_RECOMPILER
		if (!m_dec) {
			m_dec = new PPULLVMEmulator(*this);
		}
#else
		LOG_ERROR(PPU, "This image does not include PPU JIT (LLVM)");
		Emu.Pause();
#endif
	break;

	default:
		LOG_ERROR(PPU, "Invalid CPU decoder mode: %d", Ini.CPUDecoderMode.GetValue());
		Emu.Pause();
	}
}

void PPUThread::DoResume()
{
}

void PPUThread::DoPause()
{
}

void PPUThread::DoStop()
{
	delete m_dec;
	m_dec = nullptr;
}

bool dump_enable = false;

bool FPRdouble::IsINF(PPCdouble d)
{
	return ((u64&)d & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL;
}

bool FPRdouble::IsNaN(PPCdouble d)
{
	return std::isnan((double)d) ? 1 : 0;
}

bool FPRdouble::IsQNaN(PPCdouble d)
{
	return 
		((u64&)d & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL &&
		((u64&)d & 0x0007FFFFFFFFFFFULL) == 0ULL &&
		((u64&)d & 0x000800000000000ULL) != 0ULL;
}

bool FPRdouble::IsSNaN(PPCdouble d)
{
	return
		((u64&)d & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL &&
		((u64&)d & 0x000FFFFFFFFFFFFFULL) != 0ULL &&
		((u64&)d & 0x0008000000000000ULL) == 0ULL;
}

int FPRdouble::Cmp(PPCdouble a, PPCdouble b)
{
	if(a < b) return CR_LT;
	if(a > b) return CR_GT;
	if(a == b) return CR_EQ;

	return CR_SO;
}

u64 PPUThread::GetStackArg(s32 i)
{
	return vm::read64(GPR[1] + 0x70 + 0x8 * (i - 9));
}

u64 PPUThread::FastCall2(u32 addr, u32 rtoc)
{
	auto old_status = m_status;
	auto old_PC = PC;
	auto old_stack = GPR[1]; // only saved and restored (may be wrong)
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_thread = GetCurrentNamedThread();

	m_status = Running;
	PC = addr;
	GPR[2] = rtoc;
	LR = Emu.m_ppu_thr_stop;
	SetCurrentNamedThread(this);

	CPUThread::Task();

	m_status = old_status;
	PC = old_PC;
	GPR[1] = old_stack;
	GPR[2] = old_rtoc;
	LR = old_LR;
	SetCurrentNamedThread(old_thread);

	return GPR[3];
}

void PPUThread::FastStop()
{
	m_status = Stopped;
}

void PPUThread::Task()
{
	if (m_custom_task)
	{
		m_custom_task(*this);
	}
	else
	{
		CPUThread::Task();
	}
}
