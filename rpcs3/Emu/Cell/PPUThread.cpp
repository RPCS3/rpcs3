#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/PPUInterpreter2.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
//#include "Emu/Cell/PPURecompiler.h"
#include "Emu/CPU/CPUThreadManager.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

u64 rotate_mask[64][64];

const ppu_inter_func_t g_ppu_inter_func_list[] =
{
	ppu_interpreter::NULL_OP,
	ppu_interpreter::NOP,

	ppu_interpreter::TDI,
	ppu_interpreter::TWI,

	ppu_interpreter::MFVSCR,
	ppu_interpreter::MTVSCR,
	ppu_interpreter::VADDCUW,
	ppu_interpreter::VADDFP,
	ppu_interpreter::VADDSBS,
	ppu_interpreter::VADDSHS,
	ppu_interpreter::VADDSWS,
	ppu_interpreter::VADDUBM,
	ppu_interpreter::VADDUBS,
	ppu_interpreter::VADDUHM,
	ppu_interpreter::VADDUHS,
	ppu_interpreter::VADDUWM,
	ppu_interpreter::VADDUWS,
	ppu_interpreter::VAND,
	ppu_interpreter::VANDC,
	ppu_interpreter::VAVGSB,
	ppu_interpreter::VAVGSH,
	ppu_interpreter::VAVGSW,
	ppu_interpreter::VAVGUB,
	ppu_interpreter::VAVGUH,
	ppu_interpreter::VAVGUW,
	ppu_interpreter::VCFSX,
	ppu_interpreter::VCFUX,
	ppu_interpreter::VCMPBFP,
	ppu_interpreter::VCMPBFP_,
	ppu_interpreter::VCMPEQFP,
	ppu_interpreter::VCMPEQFP_,
	ppu_interpreter::VCMPEQUB,
	ppu_interpreter::VCMPEQUB_,
	ppu_interpreter::VCMPEQUH,
	ppu_interpreter::VCMPEQUH_,
	ppu_interpreter::VCMPEQUW,
	ppu_interpreter::VCMPEQUW_,
	ppu_interpreter::VCMPGEFP,
	ppu_interpreter::VCMPGEFP_,
	ppu_interpreter::VCMPGTFP,
	ppu_interpreter::VCMPGTFP_,
	ppu_interpreter::VCMPGTSB,
	ppu_interpreter::VCMPGTSB_,
	ppu_interpreter::VCMPGTSH,
	ppu_interpreter::VCMPGTSH_,
	ppu_interpreter::VCMPGTSW,
	ppu_interpreter::VCMPGTSW_,
	ppu_interpreter::VCMPGTUB,
	ppu_interpreter::VCMPGTUB_,
	ppu_interpreter::VCMPGTUH,
	ppu_interpreter::VCMPGTUH_,
	ppu_interpreter::VCMPGTUW,
	ppu_interpreter::VCMPGTUW_,
	ppu_interpreter::VCTSXS,
	ppu_interpreter::VCTUXS,
	ppu_interpreter::VEXPTEFP,
	ppu_interpreter::VLOGEFP,
	ppu_interpreter::VMADDFP,
	ppu_interpreter::VMAXFP,
	ppu_interpreter::VMAXSB,
	ppu_interpreter::VMAXSH,
	ppu_interpreter::VMAXSW,
	ppu_interpreter::VMAXUB,
	ppu_interpreter::VMAXUH,
	ppu_interpreter::VMAXUW,
	ppu_interpreter::VMHADDSHS,
	ppu_interpreter::VMHRADDSHS,
	ppu_interpreter::VMINFP,
	ppu_interpreter::VMINSB,
	ppu_interpreter::VMINSH,
	ppu_interpreter::VMINSW,
	ppu_interpreter::VMINUB,
	ppu_interpreter::VMINUH,
	ppu_interpreter::VMINUW,
	ppu_interpreter::VMLADDUHM,
	ppu_interpreter::VMRGHB,
	ppu_interpreter::VMRGHH,
	ppu_interpreter::VMRGHW,
	ppu_interpreter::VMRGLB,
	ppu_interpreter::VMRGLH,
	ppu_interpreter::VMRGLW,
	ppu_interpreter::VMSUMMBM,
	ppu_interpreter::VMSUMSHM,
	ppu_interpreter::VMSUMSHS,
	ppu_interpreter::VMSUMUBM,
	ppu_interpreter::VMSUMUHM,
	ppu_interpreter::VMSUMUHS,
	ppu_interpreter::VMULESB,
	ppu_interpreter::VMULESH,
	ppu_interpreter::VMULEUB,
	ppu_interpreter::VMULEUH,
	ppu_interpreter::VMULOSB,
	ppu_interpreter::VMULOSH,
	ppu_interpreter::VMULOUB,
	ppu_interpreter::VMULOUH,
	ppu_interpreter::VNMSUBFP,
	ppu_interpreter::VNOR,
	ppu_interpreter::VOR,
	ppu_interpreter::VPERM,
	ppu_interpreter::VPKPX,
	ppu_interpreter::VPKSHSS,
	ppu_interpreter::VPKSHUS,
	ppu_interpreter::VPKSWSS,
	ppu_interpreter::VPKSWUS,
	ppu_interpreter::VPKUHUM,
	ppu_interpreter::VPKUHUS,
	ppu_interpreter::VPKUWUM,
	ppu_interpreter::VPKUWUS,
	ppu_interpreter::VREFP,
	ppu_interpreter::VRFIM,
	ppu_interpreter::VRFIN,
	ppu_interpreter::VRFIP,
	ppu_interpreter::VRFIZ,
	ppu_interpreter::VRLB,
	ppu_interpreter::VRLH,
	ppu_interpreter::VRLW,
	ppu_interpreter::VRSQRTEFP,
	ppu_interpreter::VSEL,
	ppu_interpreter::VSL,
	ppu_interpreter::VSLB,
	ppu_interpreter::VSLDOI,
	ppu_interpreter::VSLH,
	ppu_interpreter::VSLO,
	ppu_interpreter::VSLW,
	ppu_interpreter::VSPLTB,
	ppu_interpreter::VSPLTH,
	ppu_interpreter::VSPLTISB,
	ppu_interpreter::VSPLTISH,
	ppu_interpreter::VSPLTISW,
	ppu_interpreter::VSPLTW,
	ppu_interpreter::VSR,
	ppu_interpreter::VSRAB,
	ppu_interpreter::VSRAH,
	ppu_interpreter::VSRAW,
	ppu_interpreter::VSRB,
	ppu_interpreter::VSRH,
	ppu_interpreter::VSRO,
	ppu_interpreter::VSRW,
	ppu_interpreter::VSUBCUW,
	ppu_interpreter::VSUBFP,
	ppu_interpreter::VSUBSBS,
	ppu_interpreter::VSUBSHS,
	ppu_interpreter::VSUBSWS,
	ppu_interpreter::VSUBUBM,
	ppu_interpreter::VSUBUBS,
	ppu_interpreter::VSUBUHM,
	ppu_interpreter::VSUBUHS,
	ppu_interpreter::VSUBUWM,
	ppu_interpreter::VSUBUWS,
	ppu_interpreter::VSUMSWS,
	ppu_interpreter::VSUM2SWS,
	ppu_interpreter::VSUM4SBS,
	ppu_interpreter::VSUM4SHS,
	ppu_interpreter::VSUM4UBS,
	ppu_interpreter::VUPKHPX,
	ppu_interpreter::VUPKHSB,
	ppu_interpreter::VUPKHSH,
	ppu_interpreter::VUPKLPX,
	ppu_interpreter::VUPKLSB,
	ppu_interpreter::VUPKLSH,
	ppu_interpreter::VXOR,
	ppu_interpreter::MULLI,
	ppu_interpreter::SUBFIC,
	ppu_interpreter::CMPLI,
	ppu_interpreter::CMPI,
	ppu_interpreter::ADDIC,
	ppu_interpreter::ADDIC_,
	ppu_interpreter::ADDI,
	ppu_interpreter::ADDIS,
	ppu_interpreter::BC,
	ppu_interpreter::HACK,
	ppu_interpreter::SC,
	ppu_interpreter::B,
	ppu_interpreter::MCRF,
	ppu_interpreter::BCLR,
	ppu_interpreter::CRNOR,
	ppu_interpreter::CRANDC,
	ppu_interpreter::ISYNC,
	ppu_interpreter::CRXOR,
	ppu_interpreter::CRNAND,
	ppu_interpreter::CRAND,
	ppu_interpreter::CREQV,
	ppu_interpreter::CRORC,
	ppu_interpreter::CROR,
	ppu_interpreter::BCCTR,
	ppu_interpreter::RLWIMI,
	ppu_interpreter::RLWINM,
	ppu_interpreter::RLWNM,
	ppu_interpreter::ORI,
	ppu_interpreter::ORIS,
	ppu_interpreter::XORI,
	ppu_interpreter::XORIS,
	ppu_interpreter::ANDI_,
	ppu_interpreter::ANDIS_,
	ppu_interpreter::RLDICL,
	ppu_interpreter::RLDICR,
	ppu_interpreter::RLDIC,
	ppu_interpreter::RLDIMI,
	ppu_interpreter::RLDC_LR,
	ppu_interpreter::CMP,
	ppu_interpreter::TW,
	ppu_interpreter::LVSL,
	ppu_interpreter::LVEBX,
	ppu_interpreter::SUBFC,
	ppu_interpreter::MULHDU,
	ppu_interpreter::ADDC,
	ppu_interpreter::MULHWU,
	ppu_interpreter::MFOCRF,
	ppu_interpreter::LWARX,
	ppu_interpreter::LDX,
	ppu_interpreter::LWZX,
	ppu_interpreter::SLW,
	ppu_interpreter::CNTLZW,
	ppu_interpreter::SLD,
	ppu_interpreter::AND,
	ppu_interpreter::CMPL,
	ppu_interpreter::LVSR,
	ppu_interpreter::LVEHX,
	ppu_interpreter::SUBF,
	ppu_interpreter::LDUX,
	ppu_interpreter::DCBST,
	ppu_interpreter::LWZUX,
	ppu_interpreter::CNTLZD,
	ppu_interpreter::ANDC,
	ppu_interpreter::TD,
	ppu_interpreter::LVEWX,
	ppu_interpreter::MULHD,
	ppu_interpreter::MULHW,
	ppu_interpreter::LDARX,
	ppu_interpreter::DCBF,
	ppu_interpreter::LBZX,
	ppu_interpreter::LVX,
	ppu_interpreter::NEG,
	ppu_interpreter::LBZUX,
	ppu_interpreter::NOR,
	ppu_interpreter::STVEBX,
	ppu_interpreter::SUBFE,
	ppu_interpreter::ADDE,
	ppu_interpreter::MTOCRF,
	ppu_interpreter::STDX,
	ppu_interpreter::STWCX_,
	ppu_interpreter::STWX,
	ppu_interpreter::STVEHX,
	ppu_interpreter::STDUX,
	ppu_interpreter::STWUX,
	ppu_interpreter::STVEWX,
	ppu_interpreter::SUBFZE,
	ppu_interpreter::ADDZE,
	ppu_interpreter::STDCX_,
	ppu_interpreter::STBX,
	ppu_interpreter::STVX,
	ppu_interpreter::MULLD,
	ppu_interpreter::SUBFME,
	ppu_interpreter::ADDME,
	ppu_interpreter::MULLW,
	ppu_interpreter::DCBTST,
	ppu_interpreter::STBUX,
	ppu_interpreter::ADD,
	ppu_interpreter::DCBT,
	ppu_interpreter::LHZX,
	ppu_interpreter::EQV,
	ppu_interpreter::ECIWX,
	ppu_interpreter::LHZUX,
	ppu_interpreter::XOR,
	ppu_interpreter::MFSPR,
	ppu_interpreter::LWAX,
	ppu_interpreter::DST,
	ppu_interpreter::LHAX,
	ppu_interpreter::LVXL,
	ppu_interpreter::MFTB,
	ppu_interpreter::LWAUX,
	ppu_interpreter::DSTST,
	ppu_interpreter::LHAUX,
	ppu_interpreter::STHX,
	ppu_interpreter::ORC,
	ppu_interpreter::ECOWX,
	ppu_interpreter::STHUX,
	ppu_interpreter::OR,
	ppu_interpreter::DIVDU,
	ppu_interpreter::DIVWU,
	ppu_interpreter::MTSPR,
	ppu_interpreter::DCBI,
	ppu_interpreter::NAND,
	ppu_interpreter::STVXL,
	ppu_interpreter::DIVD,
	ppu_interpreter::DIVW,
	ppu_interpreter::LVLX,
	ppu_interpreter::LDBRX,
	ppu_interpreter::LSWX,
	ppu_interpreter::LWBRX,
	ppu_interpreter::LFSX,
	ppu_interpreter::SRW,
	ppu_interpreter::SRD,
	ppu_interpreter::LVRX,
	ppu_interpreter::LSWI,
	ppu_interpreter::LFSUX,
	ppu_interpreter::SYNC,
	ppu_interpreter::LFDX,
	ppu_interpreter::LFDUX,
	ppu_interpreter::STVLX,
	ppu_interpreter::STDBRX,
	ppu_interpreter::STSWX,
	ppu_interpreter::STWBRX,
	ppu_interpreter::STFSX,
	ppu_interpreter::STVRX,
	ppu_interpreter::STFSUX,
	ppu_interpreter::STSWI,
	ppu_interpreter::STFDX,
	ppu_interpreter::STFDUX,
	ppu_interpreter::LVLXL,
	ppu_interpreter::LHBRX,
	ppu_interpreter::SRAW,
	ppu_interpreter::SRAD,
	ppu_interpreter::LVRXL,
	ppu_interpreter::DSS,
	ppu_interpreter::SRAWI,
	ppu_interpreter::SRADI,
	ppu_interpreter::EIEIO,
	ppu_interpreter::STVLXL,
	ppu_interpreter::STHBRX,
	ppu_interpreter::EXTSH,
	ppu_interpreter::STVRXL,
	ppu_interpreter::EXTSB,
	ppu_interpreter::STFIWX,
	ppu_interpreter::EXTSW,
	ppu_interpreter::ICBI,
	ppu_interpreter::DCBZ,
	ppu_interpreter::LWZ,
	ppu_interpreter::LWZU,
	ppu_interpreter::LBZ,
	ppu_interpreter::LBZU,
	ppu_interpreter::STW,
	ppu_interpreter::STWU,
	ppu_interpreter::STB,
	ppu_interpreter::STBU,
	ppu_interpreter::LHZ,
	ppu_interpreter::LHZU,
	ppu_interpreter::LHA,
	ppu_interpreter::LHAU,
	ppu_interpreter::STH,
	ppu_interpreter::STHU,
	ppu_interpreter::LMW,
	ppu_interpreter::STMW,
	ppu_interpreter::LFS,
	ppu_interpreter::LFSU,
	ppu_interpreter::LFD,
	ppu_interpreter::LFDU,
	ppu_interpreter::STFS,
	ppu_interpreter::STFSU,
	ppu_interpreter::STFD,
	ppu_interpreter::STFDU,
	ppu_interpreter::LD,
	ppu_interpreter::LDU,
	ppu_interpreter::LWA,
	ppu_interpreter::FDIVS,
	ppu_interpreter::FSUBS,
	ppu_interpreter::FADDS,
	ppu_interpreter::FSQRTS,
	ppu_interpreter::FRES,
	ppu_interpreter::FMULS,
	ppu_interpreter::FMADDS,
	ppu_interpreter::FMSUBS,
	ppu_interpreter::FNMSUBS,
	ppu_interpreter::FNMADDS,
	ppu_interpreter::STD,
	ppu_interpreter::STDU,
	ppu_interpreter::MTFSB1,
	ppu_interpreter::MCRFS,
	ppu_interpreter::MTFSB0,
	ppu_interpreter::MTFSFI,
	ppu_interpreter::MFFS,
	ppu_interpreter::MTFSF,

	ppu_interpreter::FCMPU,
	ppu_interpreter::FRSP,
	ppu_interpreter::FCTIW,
	ppu_interpreter::FCTIWZ,
	ppu_interpreter::FDIV,
	ppu_interpreter::FSUB,
	ppu_interpreter::FADD,
	ppu_interpreter::FSQRT,
	ppu_interpreter::FSEL,
	ppu_interpreter::FMUL,
	ppu_interpreter::FRSQRTE,
	ppu_interpreter::FMSUB,
	ppu_interpreter::FMADD,
	ppu_interpreter::FNMSUB,
	ppu_interpreter::FNMADD,
	ppu_interpreter::FCMPO,
	ppu_interpreter::FNEG,
	ppu_interpreter::FMR,
	ppu_interpreter::FNABS,
	ppu_interpreter::FABS,
	ppu_interpreter::FCTID,
	ppu_interpreter::FCTIDZ,
	ppu_interpreter::FCFID,

	ppu_interpreter::UNK,
};

extern u32 ppu_get_tls(u32 thread);
extern void ppu_free_tls(u32 thread);

void* g_ppu_exec_map = nullptr;

void finalize_ppu_exec_map()
{
	if (g_ppu_exec_map)
	{
#ifdef _WIN32
		VirtualFree(g_ppu_exec_map, 0, MEM_RELEASE);
#else
		munmap(g_ppu_exec_map, 0x100000000);
#endif
		g_ppu_exec_map = nullptr;
	}
}

void initialize_ppu_exec_map()
{
	finalize_ppu_exec_map();

#ifdef _WIN32
	g_ppu_exec_map = VirtualAlloc(NULL, 0x100000000, MEM_RESERVE, PAGE_NOACCESS);
#else
	g_ppu_exec_map = mmap(nullptr, 0x100000000, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
}

void fill_ppu_exec_map(u32 addr, u32 size)
{
#ifdef _WIN32
	VirtualAlloc((u8*)g_ppu_exec_map + addr, size, MEM_COMMIT, PAGE_READWRITE);
#else
	mprotect((u8*)g_ppu_exec_map + addr, size, PROT_READ | PROT_WRITE);
#endif

	PPUInterpreter2* inter;
	PPUDecoder dec(inter = new PPUInterpreter2);

	for (u32 pos = addr; pos < addr + size; pos += 4)
	{
		inter->func = ppu_interpreter::NULL_OP;

		// decode PPU opcode
		dec.Decode(vm::read32(pos));

		u32 index = 0;

		// find function index
		for (; index < sizeof(g_ppu_inter_func_list) / sizeof(ppu_inter_func_t); index++)
		{
			if (inter->func == g_ppu_inter_func_list[index])
			{
				break;
			}
		}

		// write index in memory
		*(u32*)((u8*)g_ppu_exec_map + pos) = index;
	}
}

PPUThread& GetCurrentPPUThread()
{
	CPUThread* thread = GetCurrentCPUThread();

	if(!thread || thread->GetType() != CPU_THREAD_PPU) throw std::string("GetCurrentPPUThread: bad thread");

	return *(PPUThread*)thread;
}

PPUThread::PPUThread() : CPUThread(CPU_THREAD_PPU)
{
	Reset();
	InitRotateMask();
}

PPUThread::~PPUThread()
{
	ppu_free_tls(GetId());
}

void PPUThread::DoReset()
{
	//reset regs
	memset(VPR,  0, sizeof(VPR));
	memset(FPR,  0, sizeof(FPR));
	memset(GPR,  0, sizeof(GPR));
	memset(SPRG, 0, sizeof(SPRG));

	CR.CR       = 0;
	LR          = 0;
	CTR         = 0;
	TB          = 0;
	XER.XER     = 0;
	FPSCR.FPSCR = 0;
	VSCR.VSCR   = 0;
	VRSAVE      = 0;
}

void PPUThread::InitRegs()
{
	const u32 pc = entry ? vm::read32(entry) : 0;
	const u32 rtoc = entry ? vm::read32(entry + 4) : 0;

	SetPc(pc);

	GPR[1] = align(m_stack_addr + m_stack_size, 0x200) - 0x200;
	GPR[2] = rtoc;
	//GPR[11] = entry;
	//GPR[12] = Emu.GetMallocPageSize();
	GPR[13] = ppu_get_tls(GetId()) + 0x7000; // 0x7000 is usually subtracted from r13 to access first TLS element (details are not clear)

	LR = 0;
	CTR = PC;
	CR.CR = 0x22000082;
	VSCR.NJ = 1;
	TB = 0;
}

void PPUThread::InitStack()
{
	if (!m_stack_addr)
	{
		assert(m_stack_size);
		m_stack_addr = Memory.StackMem.AllocAlign(m_stack_size, 4096);
	}
}

void PPUThread::CloseStack()
{
	if (m_stack_addr)
	{
		Memory.StackMem.Free(m_stack_addr);
		m_stack_addr = 0;
	}
}

void PPUThread::DoRun()
{
	m_dec = nullptr;

	switch (auto mode = Ini.CPUDecoderMode.GetValue())
	{
	case 0: // original interpreter
	{
		auto ppui = new PPUInterpreter(*this);
		m_dec = new PPUDecoder(ppui);
		break;
	}

	case 1: // alternative interpreter
	{
		break;
	}

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
	{
		LOG_ERROR(PPU, "Invalid CPU decoder mode: %d", mode);
		Emu.Pause();
	}
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

void PPUThread::FastCall2(u32 addr, u32 rtoc)
{
	auto old_status = m_status;
	auto old_PC = PC;
	auto old_stack = GPR[1]; // only saved and restored (may be wrong)
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_thread = GetCurrentNamedThread();
	auto old_task = custom_task;

	m_status = Running;
	PC = addr;
	GPR[2] = rtoc;
	LR = Emu.GetCPUThreadStop();
	SetCurrentNamedThread(this);
	custom_task = nullptr;

	Task();

	m_status = old_status;
	PC = old_PC;
	GPR[1] = old_stack;
	GPR[2] = old_rtoc;
	LR = old_LR;
	SetCurrentNamedThread(old_thread);
	custom_task = old_task;
}

void PPUThread::FastStop()
{
	m_status = Stopped;
	m_events |= CPU_EVENT_STOP;
}

void PPUThread::Task()
{
	SetHostRoundingMode(FPSCR_RN_NEAR);

	if (custom_task)
	{
		return custom_task(*this);
	}

	if (m_dec)
	{
		return CPUThread::Task();
	}

	while (true)
	{
		// get interpreter function
		const auto func = g_ppu_inter_func_list[*(u32*)((u8*)g_ppu_exec_map + PC)];

		if (m_events)
		{
			// process events
			if (m_events & CPU_EVENT_STOP && (Emu.IsStopped() || IsStopped() || IsPaused()))
			{
				m_events &= ~CPU_EVENT_STOP;
				return;
			}
		}

		// read opcode
		const ppu_opcode_t opcode = { vm::read32(PC) };

		// call interpreter function
		func(*this, opcode);

		// next instruction
		//PC += 4;
		NextPc(4);
	}
}

ppu_thread::ppu_thread(u32 entry, const std::string& name, u32 stack_size, u32 prio)
{
	thread = Emu.GetCPU().AddThread(CPU_THREAD_PPU);

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

	static_cast<PPUThread&>(*thread).GPR[index] = value;

	return *this;
}
