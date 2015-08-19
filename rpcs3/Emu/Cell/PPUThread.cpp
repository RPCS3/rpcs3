#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/PPUInterpreter2.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
//#include "Emu/Cell/PPURecompiler.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

u64 rotate_mask[64][64];

extern u32 ppu_get_tls(u32 thread);
extern void ppu_free_tls(u32 thread);

thread_local const std::weak_ptr<ppu_decoder_cache_t> g_tls_ppu_decoder_cache = fxm::get<ppu_decoder_cache_t>();

ppu_decoder_cache_t::ppu_decoder_cache_t()
#ifdef _WIN32
	: pointer(static_cast<decltype(pointer)>(VirtualAlloc(NULL, 0x200000000, MEM_RESERVE, PAGE_NOACCESS)))
#else
	: pointer(static_cast<decltype(pointer)>(mmap(nullptr, 0x200000000, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0)))
#endif
{
}

ppu_decoder_cache_t::~ppu_decoder_cache_t()
{
#ifdef _WIN32
	VirtualFree(pointer, 0, MEM_RELEASE);
#else
	munmap(pointer, 0x200000000);
#endif
}

void ppu_decoder_cache_t::initialize(u32 addr, u32 size)
{
#ifdef _WIN32
	VirtualAlloc(pointer + addr / 4, size * 2, MEM_COMMIT, PAGE_READWRITE);
#else
	mprotect(pointer + addr / 4, size * 2, PROT_READ | PROT_WRITE);
#endif

	PPUInterpreter2* inter;
	PPUDecoder dec(inter = new PPUInterpreter2);

	for (u32 pos = addr; pos < addr + size; pos += 4)
	{
		inter->func = ppu_interpreter::NULL_OP;

		// decode PPU opcode
		dec.Decode(vm::read32(pos));

		// store function address
		pointer[pos / 4] = inter->func;
	}
}

PPUThread::PPUThread(const std::string& name)
	: CPUThread(CPU_THREAD_PPU, name, WRAP_EXPR(fmt::format("PPU[0x%x] Thread (%s)[0x%08x]", m_id, m_name.c_str(), PC)))
{
	InitRotateMask();
}

PPUThread::~PPUThread()
{
	if (is_current())
	{
		detach();
	}
	else
	{
		join();
	}

	close_stack();
	ppu_free_tls(m_id);
}

void PPUThread::dump_info() const
{
	if (~hle_code < 1024)
	{
		LOG_SUCCESS(HLE, "Last syscall: %lld (%s)", ~hle_code, SysCalls::GetFuncName(hle_code));
	}
	else if (hle_code)
	{
		LOG_SUCCESS(HLE, "Last function: %s (0x%llx)", SysCalls::GetFuncName(hle_code), hle_code);
	}

	CPUThread::dump_info();
}

void PPUThread::init_regs()
{
	GPR[1] = align(stack_addr + stack_size, 0x200) - 0x200;
	GPR[13] = ppu_get_tls(m_id) + 0x7000; // 0x7000 is subtracted from r13 to access first TLS element

	LR = 0;
	CTR = PC;
	CR.CR = 0x22000082;
	VSCR.NJ = 1;
	TB = 0;

	//m_state |= CPU_STATE_INTR;
}

void PPUThread::init_stack()
{
	if (!stack_addr)
	{
		if (!stack_size)
		{
			throw EXCEPTION("Invalid stack size");
		}

		stack_addr = vm::alloc(stack_size, vm::stack);

		if (!stack_addr)
		{
			throw EXCEPTION("Out of stack memory");
		}
	}
}

void PPUThread::close_stack()
{
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
		stack_addr = 0;
	}
}

bool PPUThread::handle_interrupt()
{
	return false;
}

void PPUThread::do_run()
{
	m_dec.reset();

	switch (auto mode = Ini.CPUDecoderMode.GetValue())
	{
	case 0: // original interpreter
	{
		m_dec.reset(new PPUDecoder(new PPUInterpreter(*this)));
		break;
	}

	case 1: // alternative interpreter
	{
		break;
	}

	case 2:
	{
#ifdef PPU_LLVM_RECOMPILER
		m_dec.reset(new ppu_recompiler_llvm::CPUHybridDecoderRecompiler(*this));
#else
		LOG_ERROR(PPU, "This image does not include PPU JIT (LLVM)");
		Emu.Pause();
#endif
		break;
	}

	//case 3: m_dec.reset(new PPURecompiler(*this)); break;

	default:
	{
		LOG_ERROR(PPU, "Invalid CPU decoder mode: %d", mode);
		Emu.Pause();
	}
	}
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

u64 PPUThread::get_stack_arg(s32 i)
{
	return vm::read64(VM_CAST(GPR[1] + 0x70 + 0x8 * (i - 9)));
}

void PPUThread::fast_call(u32 addr, u32 rtoc)
{
	if (!is_current())
	{
		throw EXCEPTION("Called from the wrong thread");
	}

	auto old_PC = PC;
	auto old_stack = GPR[1];
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_task = std::move(custom_task);

	assert(!old_task || !custom_task);

	PC = addr;
	GPR[2] = rtoc;
	LR = Emu.GetCPUThreadStop();
	custom_task = nullptr;

	try
	{
		task();
	}
	catch (CPUThreadReturn)
	{
	}

	m_state &= ~CPU_STATE_RETURN;

	PC = old_PC;

	if (GPR[1] != old_stack) // GPR[1] shouldn't change
	{
		throw EXCEPTION("Stack inconsistency (addr=0x%x, rtoc=0x%x, SP=0x%llx, old=0x%llx)", addr, rtoc, GPR[1], old_stack);
	}

	GPR[2] = old_rtoc;
	LR = old_LR;
	custom_task = std::move(old_task);
}

void PPUThread::fast_stop()
{
	m_state |= CPU_STATE_RETURN;
}

void PPUThread::task()
{
	SetHostRoundingMode(FPSCR_RN_NEAR);

	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}

	const auto decoder_cache = g_tls_ppu_decoder_cache.lock();

	if (!decoder_cache)
	{
		throw EXCEPTION("PPU Decoder Cache not initialized");
	}

	const auto exec_map = decoder_cache->pointer;

	if (m_dec)
	{
		while (true)
		{
			if (m_state.load() && check_status()) break;

			// decode instruction using specified decoder
			m_dec->DecodeMemory(PC);

			// next instruction
			PC += 4;
		}
	}
	else
	{
		while (true)
		{
			// get cached interpreter function address
			const auto func = exec_map[PC / 4];

			// check status
			if (!m_state.load())
			{
				// call interpreter function
				func(*this, { vm::read32(PC) });

				// next instruction
				PC += 4;

				continue;
			}
			
			if (check_status())
			{
				break;
			}
		}
	}
}

ppu_thread::ppu_thread(u32 entry, const std::string& name, u32 stack_size, s32 prio)
{
	auto ppu = idm::make_ptr<PPUThread>(name);

	if (entry)
	{
		ppu->PC = vm::read32(entry);
		ppu->GPR[2] = vm::read32(entry + 4); // rtoc
	}

	ppu->stack_size = stack_size ? stack_size : Emu.GetPrimaryStackSize();
	ppu->prio = prio ? prio : Emu.GetPrimaryPrio();

	thread = std::move(ppu);

	argc = 0;
}

cpu_thread& ppu_thread::args(std::initializer_list<std::string> values)
{
	if (!values.size())
		return *this;

	assert(argc == 0);

	envp.set(vm::alloc(align(sizeof32(*envp), stack_align), vm::main));
	*envp = 0;
	argv.set(vm::alloc(sizeof32(*argv) * (u32)values.size(), vm::main));

	for (auto &arg : values)
	{
		const u32 arg_size = align(u32(arg.size() + 1), stack_align);
		const u32 arg_addr = vm::alloc(arg_size, vm::main);

		std::memcpy(vm::get_ptr(arg_addr), arg.c_str(), arg.size() + 1);

		argv[argc++] = arg_addr;
	}

	return *this;
}

cpu_thread& ppu_thread::run()
{
	thread->run();

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
