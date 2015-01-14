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
//#include "Emu/Cell/PPURecompiler.h"
#include "Emu/CPU/CPUThreadManager.h"

u64 rotate_mask[64][64];

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
	memset(USPRG, 0, sizeof(USPRG));

	CR.CR       = 0;
	LR          = 0;
	CTR         = 0;
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

	GPR[1] = align(m_stack_addr + m_stack_size, 0x200) - 0x200;
	GPR[2] = rtoc;
	//GPR[11] = entry;
	//GPR[12] = Emu.GetMallocPageSize();
	GPR[13] = Memory.PRXMem.GetStartAddr() + 0x7060;

	LR = Emu.GetCPUThreadExit();
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
		SetCallStackTracing(false);
		if (!m_dec) {
			m_dec = new ppu_recompiler_llvm::ExecutionEngine(*this);
		}
#else
		LOG_ERROR(PPU, "This image does not include PPU JIT (LLVM)");
		Emu.Pause();
#endif
	break;

	//case 3: m_dec = new PPURecompiler(*this); break;

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
	return vm::read64(vm::cast(GPR[1] + 0x70 + 0x8 * (i - 9)));
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
	LR = Emu.GetCPUThreadStop();
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
	if (custom_task)
	{
		custom_task(*this);
	}
	else
	{
		CPUThread::Task();
	}
}

ppu_thread::ppu_thread(u32 entry, const std::string& name, u32 stack_size, u32 prio)
{
	thread = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	thread->SetName(name);
	thread->SetEntry(entry);
	thread->SetStackSize(stack_size ? stack_size : Emu.GetInfo().GetProcParam().primary_stacksize);
	thread->SetPrio(prio ? prio : Emu.GetInfo().GetProcParam().primary_prio);

	argc = 0;
}

cpu_thread& ppu_thread::args(std::initializer_list<std::string> values)
{
	if (!values.size())
		return *this;

	assert(argc == 0);

	envp.set(vm::alloc(align((u32)sizeof(*envp), stack_align), vm::main));
	*envp = 0;
	argv.set(vm::alloc(sizeof(*argv) * values.size(), vm::main));

	for (auto &arg : values)
	{
		u32 arg_size = align(u32(arg.size() + 1), stack_align);
		u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::strcpy(vm::get_ptr<char>(arg_addr), arg.c_str());

		argv[argc++] = arg_addr;
	}

	return *this;
}

cpu_thread& ppu_thread::run()
{
	thread->Run();

	gpr(3, argc);
	gpr(4, argv.addr());
	gpr(5, envp.addr());

	return *this;
}

ppu_thread& ppu_thread::gpr(uint index, u64 value)
{
	assert(index < 32);

	static_cast<PPUThread*>(thread)->GPR[index] = value;

	return *this;
}
