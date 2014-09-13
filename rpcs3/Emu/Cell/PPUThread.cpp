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

void PPUThread::AddArgv(const std::string& arg)
{
	m_stack_point -= arg.length() + 1;
	m_stack_point = AlignAddr(m_stack_point, 0x10) - 0x10;
	m_argv_addr.push_back(m_stack_point);
	memcpy(vm::get_ptr<char>(m_stack_point), arg.c_str(), arg.size() + 1);
}

void PPUThread::InitRegs()
{
	const u32 pc = vm::read32(entry);
	const u32 rtoc = vm::read32(entry + 4);

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

	m_stack_point = AlignAddr(m_stack_point, 0x200) - 0x200;

	GPR[1] = m_stack_point;
	GPR[2] = rtoc;
	/*
	for(int i=4; i<32; ++i)
	{
		if(i != 6)
			GPR[i] = (i+1) * 0x10000;
	}
	*/
	if(m_argv_addr.size())
	{
		u64 argc = m_argv_addr.size();
		m_stack_point -= 0xc + 4 * argc;
		u64 argv = m_stack_point;

		auto argv_list = vm::ptr<be_t<u64>>::make((u32)argv);
		for(int i=0; i<argc; ++i) argv_list[i] = m_argv_addr[i];

		GPR[3] = argc;
		GPR[4] = argv;
		GPR[5] = argv ? argv + 0xc + 4 * argc : 0; //unk
	}
	else
	{
		GPR[3] = m_args[0];
		GPR[4] = m_args[1];
		GPR[5] = m_args[2];
		GPR[6] = m_args[3];
	}

	GPR[0] = pc;
	GPR[8] = entry;
	GPR[11] = 0x80;
	GPR[12] = Emu.GetMallocPageSize();
	GPR[13] = Memory.PS3.PRXMem.GetStartAddr() + 0x7060;
	GPR[28] = GPR[4];
	GPR[29] = GPR[3];
	GPR[31] = GPR[5];

	LR = Emu.GetPPUThreadExit();
	CTR = PC;
	CR.CR = 0x22000082;
	VSCR.NJ = 1;
	TB = 0;
}

u64 PPUThread::GetFreeStackSize() const
{
	return (GetStackAddr() + GetStackSize()) - GPR[1];
}

void PPUThread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		//m_dec = new PPUDecoder(*new PPUDisAsm());
	break;

	case 1:
	case 2:
	{
		auto ppui = new PPUInterpreter(*this);
		m_dec = new PPUDecoder(ppui);
	}
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

u64 PPUThread::FastCall(u64 addr, u64 rtoc, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6, u64 arg7, u64 arg8)
{
	GPR[3] = arg1;
	GPR[4] = arg2;
	GPR[5] = arg3;
	GPR[6] = arg4;
	GPR[7] = arg5;
	GPR[8] = arg6;
	GPR[9] = arg7;
	GPR[10] = arg8;
	
	return FastCall2(addr, rtoc);
}

u64 PPUThread::FastCall2(u64 addr, u64 rtoc)
{
	auto old_status = m_status;
	auto old_PC = PC;
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_thread = GetCurrentNamedThread();

	m_status = Running;
	PC = addr;
	GPR[2] = rtoc;
	LR = Emu.m_ppu_thr_stop;
	SetCurrentNamedThread(this);

	Task();

	m_status = old_status;
	PC = old_PC;
	GPR[2] = old_rtoc;
	LR = old_LR;
	SetCurrentNamedThread(old_thread);

	return GPR[3];
}

void PPUThread::FastStop()
{
	m_status = Stopped;
}