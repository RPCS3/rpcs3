#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"

enum class ppu_decoder_type
{
	precise,
	fast,
	llvm,
};

cfg::map_entry<ppu_decoder_type> g_cfg_ppu_decoder(cfg::root.core, "PPU Decoder", 1,
{
	{ "Interpreter (precise)", ppu_decoder_type::precise },
	{ "Interpreter (fast)", ppu_decoder_type::fast },
	{ "Recompiler (LLVM)", ppu_decoder_type::llvm },
});

const ppu_decoder<ppu_interpreter_precise> s_ppu_interpreter_precise;
const ppu_decoder<ppu_interpreter_fast> s_ppu_interpreter_fast;

struct ppu_addr_hash
{
	u32 operator()(u32 value) const
	{
		return value / sizeof(32);
	}
};

static std::unordered_map<u32, void(*)(), ppu_addr_hash> s_ppu_compiled;



std::string PPUThread::get_name() const
{
	return fmt::format("PPU[0x%x] Thread (%s)", id, name);
}

std::string PPUThread::dump() const
{
	std::string ret = "Registers:\n=========\n";

	for (uint i = 0; i<32; ++i) ret += fmt::format("GPR[%d] = 0x%llx\n", i, GPR[i]);
	for (uint i = 0; i<32; ++i) ret += fmt::format("FPR[%d] = %.6G\n", i, FPR[i]);
	for (uint i = 0; i<32; ++i) ret += fmt::format("VR[%d] = 0x%s [%s]\n", i, VR[i].to_hex().c_str(), VR[i].to_xyzw().c_str());
	ret += fmt::format("CR = 0x%08x\n", GetCR());
	ret += fmt::format("LR = 0x%llx\n", LR);
	ret += fmt::format("CTR = 0x%llx\n", CTR);
	ret += fmt::format("XER = [CA=%u | OV=%u | SO=%u | CNT=%u]\n", u32{ CA }, u32{ OV }, u32{ SO }, u32{ XCNT });
	//ret += fmt::format("FPSCR = 0x%x "
	//	"[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
	//	"VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
	//	"FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
	//	"VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
	//	"XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]\n",
	//	FPSCR.FPSCR,
	//	u32{ FPSCR.RN },
	//	u32{ FPSCR.NI }, u32{ FPSCR.XE }, u32{ FPSCR.ZE }, u32{ FPSCR.UE }, u32{ FPSCR.OE }, u32{ FPSCR.VE },
	//	u32{ FPSCR.VXCVI }, u32{ FPSCR.VXSQRT }, u32{ FPSCR.VXSOFT }, u32{ FPSCR.FPRF },
	//	u32{ FPSCR.FI }, u32{ FPSCR.FR }, u32{ FPSCR.VXVC }, u32{ FPSCR.VXIMZ },
	//	u32{ FPSCR.VXZDZ }, u32{ FPSCR.VXIDI }, u32{ FPSCR.VXISI }, u32{ FPSCR.VXSNAN },
	//	u32{ FPSCR.XX }, u32{ FPSCR.ZX }, u32{ FPSCR.UX }, u32{ FPSCR.OX }, u32{ FPSCR.VX }, u32{ FPSCR.FEX }, u32{ FPSCR.FX });

	return ret;
}

void PPUThread::cpu_init()
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

	GPR[1] = align(stack_addr + stack_size, 0x200) - 0x200;
}

extern thread_local std::string(*g_tls_log_prefix)();

void PPUThread::cpu_task()
{
	//SetHostRoundingMode(FPSCR_RN_NEAR);

	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}

	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		const auto found = s_ppu_compiled.find(pc);

		if (found != s_ppu_compiled.end())
		{
			return found->second();
		}
	}

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<PPUThread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", cpu->get_name(), cpu->pc);
	};

	const auto base = vm::_ptr<const u8>(0);

	// Select opcode table
	const auto& table = *(
		g_cfg_ppu_decoder.get() == ppu_decoder_type::precise ? &s_ppu_interpreter_precise.get_table() :
		g_cfg_ppu_decoder.get() == ppu_decoder_type::fast ? &s_ppu_interpreter_fast.get_table() :
		throw std::logic_error("Invalid PPU decoder"));

	v128 _op;
	decltype(&ppu_interpreter::UNK) func0, func1, func2, func3;

	while (true)
	{
		if (UNLIKELY(state.load()))
		{
			if (check_status()) return;
		}

		// Reinitialize
		{
			const auto _ops = _mm_shuffle_epi8(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(base + pc)), _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3));
			_op.vi = _ops;
			const v128 _i = v128::fromV(_mm_and_si128(_mm_or_si128(_mm_slli_epi32(_op.vi, 6), _mm_srli_epi32(_op.vi, 26)), _mm_set1_epi32(0x1ffff)));
			func0 = table[_i._u32[0]];
			func1 = table[_i._u32[1]];
			func2 = table[_i._u32[2]];
			func3 = table[_i._u32[3]];
		}

		while (LIKELY(func0(*this, { _op._u32[0] })))
		{
			if (pc += 4, LIKELY(func1(*this, { _op._u32[1] })))
			{
				if (pc += 4, LIKELY(func2(*this, { _op._u32[2] })))
				{
					pc += 4;
					func0 = func3;

					const auto _ops = _mm_shuffle_epi8(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(base + pc + 4)), _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3));
					_op.vi = _mm_alignr_epi8(_ops, _op.vi, 12);
					const v128 _i = v128::fromV(_mm_and_si128(_mm_or_si128(_mm_slli_epi32(_op.vi, 6), _mm_srli_epi32(_op.vi, 26)), _mm_set1_epi32(0x1ffff)));
					func1 = table[_i._u32[1]];
					func2 = table[_i._u32[2]];
					func3 = table[_i._u32[3]];

					if (UNLIKELY(state.load()))
					{
						break;
					}
					continue;
				}
				break;
			}
			break;
		}
	}
}

constexpr auto stop_state = make_bitset(cpu_state::stop, cpu_state::exit, cpu_state::suspend);

atomic_t<u32> g_ppu_core[2]{};

bool PPUThread::handle_interrupt()
{
	// Reschedule and wake up a new thread, possibly this one as well.
	return false;

	// Check virtual core allocation
	if (g_ppu_core[0] != id && g_ppu_core[1] != id)
	{
		auto cpu0 = idm::get<PPUThread>(g_ppu_core[0]);
		auto cpu1 = idm::get<PPUThread>(g_ppu_core[1]);

		if (cpu0 && cpu1)
		{
			if (cpu1->prio > cpu0->prio)
			{
				cpu0 = std::move(cpu1);
			}

			// Preempt thread with the lowest priority
			if (prio < cpu0->prio)
			{
				cpu0->state += cpu_state::interrupt;
			}
		}
		else
		{
			// Try to obtain a virtual core in optimistic way
			if (g_ppu_core[0].compare_and_swap_test(0, id) || g_ppu_core[1].compare_and_swap_test(0, id))
			{
				state -= cpu_state::interrupt;
				return true;
			}
		}

		return false;
	}

	// Select appropriate thread
	u32 top_prio = -1;
	u32 selected = -1;

	idm::select<PPUThread>([&](u32 id, PPUThread& ppu)
	{
		// Exclude suspended and low-priority threads
		if (!ppu.state.test(stop_state) && ppu.prio < top_prio /*&& (!ppu.is_sleep() || ppu.state & cpu_state::signal)*/)
		{
			top_prio = ppu.prio;
			selected = id;
		}
	});

	// If current thread selected
	if (selected == id)
	{
		state -= cpu_state::interrupt;
		VERIFY(g_ppu_core[0] == id || g_ppu_core[1] == id);
		return true;
	}

	// If another thread selected
	const auto thread = idm::get<PPUThread>(selected);

	// Lend virtual core to another thread
	if (thread && thread->state.test_and_reset(cpu_state::interrupt))
	{
		g_ppu_core[0].compare_and_swap(id, thread->id);
		g_ppu_core[1].compare_and_swap(id, thread->id);
		(*thread)->lock_notify();
	}
	else
	{
		g_ppu_core[0].compare_and_swap(id, 0);
		g_ppu_core[1].compare_and_swap(id, 0);
	}

	return false;
}

PPUThread::~PPUThread()
{
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
	}
}

PPUThread::PPUThread(const std::string& name)
	: cpu_thread(cpu_type::ppu, name)
{
}

be_t<u64>* PPUThread::get_stack_arg(s32 i, u64 align)
{
	if (align != 1 && align != 2 && align != 4 && align != 8 && align != 16) throw fmt::exception("Unsupported alignment: 0x%llx" HERE, align);
	return vm::_ptr<u64>(vm::cast((GPR[1] + 0x30 + 0x8 * (i - 1)) & (0 - align), HERE));
}

void PPUThread::fast_call(u32 addr, u32 rtoc)
{
	auto old_PC = pc;
	auto old_stack = GPR[1];
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_task = std::move(custom_task);

	pc = addr;
	GPR[2] = rtoc;
	LR = Emu.GetCPUThreadStop();
	custom_task = nullptr;

	try
	{
		cpu_task();
	}
	catch (cpu_state _s)
	{
		state += _s;
		if (_s != cpu_state::ret) throw;
	}

	state -= cpu_state::ret;

	pc = old_PC;

	if (GPR[1] != old_stack) // GPR[1] shouldn't change
	{
		throw EXCEPTION("Stack inconsistency (addr=0x%x, rtoc=0x%x, SP=0x%llx, old=0x%llx)", addr, rtoc, GPR[1], old_stack);
	}

	GPR[2] = old_rtoc;
	LR = old_LR;
	custom_task = std::move(old_task);

	//if (custom_task)
	//{
	//	state += cpu_state::interrupt;
	//	handle_interrupt();
	//}
}

#ifdef LLVM_AVAILABLE
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
//#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
//#include "llvm/Support/Host.h"
#include "llvm/Support/FormattedStream.h"
//#include "llvm/Support/Debug.h"
//#include "llvm/CodeGen/CommandFlags.h"
//#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Dominators.h"
#include "llvm/IR/Verifier.h"
//#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Function.h"
//#include "llvm/Analysis/Passes.h"
//#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
//#include "llvm/Analysis/LoopInfo.h"
//#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
//#include "llvm/Object/ObjectFile.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "PPUTranslator.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#endif

const ppu_decoder<ppu_itype> s_ppu_itype;

extern u64 get_timebased_time();
extern void ppu_execute_syscall(PPUThread& ppu, u64 code);
extern void ppu_execute_function(PPUThread& ppu, u32 index);

extern __m128 sse_exp2_ps(__m128 A);
extern __m128 sse_log2_ps(__m128 A);
extern __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_lvsl(u64 addr);
extern __m128i sse_altivec_lvsr(u64 addr);
extern __m128i sse_cellbe_lvlx(u64 addr);
extern __m128i sse_cellbe_lvrx(u64 addr);
extern void sse_cellbe_stvlx(u64 addr, __m128i a);
extern void sse_cellbe_stvrx(u64 addr, __m128i a);

struct Listener final : llvm::JITEventListener
{
	virtual void NotifyObjectEmitted(const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& inf) override
	{
		const llvm::StringRef elf = obj.getData();
		fs::file(fs::get_config_dir() + "LLVM.obj", fs::rewrite)
			.write(elf.data(), elf.size());
	}
};

static Listener s_listener;

// Memory size: 512 MB
static const u64 s_memory_size = 0x20000000;

// Try to reserve a portion of virtual memory in the first 2 GB address space, if possible.
static void* const s_memory = []() -> void*
{
#ifdef _WIN32
	for (u64 addr = 0x1000000; addr <= 0x60000000; addr += 0x1000000)
	{
		if (VirtualAlloc((void*)addr, s_memory_size, MEM_RESERVE, PAGE_NOACCESS))
		{
			return (void*)addr;
		}
	}

	return VirtualAlloc(NULL, s_memory_size, MEM_RESERVE, PAGE_NOACCESS);
#else
	return ::mmap((void*)0x10000000, s_memory_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
}();

// EH frames
static u8* s_unwind_info;
static u64 s_unwind_size;

#ifdef _WIN32
// Custom .pdata section replacement
static std::vector<RUNTIME_FUNCTION> s_unwind;
#endif

struct MemoryManager final : llvm::RTDyldMemoryManager
{
	static PPUThread* context(u64 addr)
	{
		//trace(addr);
		return static_cast<PPUThread*>(get_current_cpu_thread());
	}

	[[noreturn]] static void trap(u64 addr)
	{
		LOG_ERROR(PPU, "Trap! (0x%llx)", addr);
		throw fmt::exception("Trap! (0x%llx)", addr);
	}

	static void trace(u64 addr)
	{
		LOG_NOTICE(PPU, "Trace: 0x%llx", addr);
	}

	static void hack(u32 index)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		ppu_execute_function(ppu, index);
		if (ppu.state.load() && ppu.check_status()) throw cpu_state::ret; // Temporarily
	}

	static void syscall(u64 code)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		ppu_execute_syscall(ppu, code);
		if (ppu.state.load() && ppu.check_status()) throw cpu_state::ret; // Temporarily
	}

	static u32 tbl()
	{
		return (u32)get_timebased_time();
	}

	static void call(u32 addr)
	{
		const auto found = s_ppu_compiled.find(addr);

		if (found != s_ppu_compiled.end())
		{
			return found->second();
		}

		const auto op = vm::read32(addr).value();
		const auto itype = s_ppu_itype.decode(op);

		// Allow HLE callbacks without compiling them
		if (itype == ppu_itype::HACK && vm::read32(addr + 4) == ppu_instructions::BLR())
		{
			return hack(op & 0x3ffffff);
		}

		trap(addr);
	}

	static __m128 sse_rcp_ps(__m128 A)
	{
		return _mm_rcp_ps(A);
	}

	static __m128 sse_rsqrt_ps(__m128 A)
	{
		return _mm_rsqrt_ps(A);
	}

	static float sse_rcp_ss(float A)
	{
		_mm_store_ss(&A, _mm_rcp_ss(_mm_load_ss(&A)));
		return A;
	}

	static float sse_rsqrt_ss(float A)
	{
		_mm_store_ss(&A, _mm_rsqrt_ss(_mm_load_ss(&A)));
		return A;
	}

	static u32 lwarx(u32 addr)
	{
		be_t<u32> reg_value;
		vm::reservation_acquire(&reg_value, addr, sizeof(reg_value));
		return reg_value;
	}

	static u64 ldarx(u32 addr)
	{
		be_t<u64> reg_value;
		vm::reservation_acquire(&reg_value, addr, sizeof(reg_value));
		return reg_value;
	}

	static bool stwcx(u32 addr, u32 reg_value)
	{
		const be_t<u32> data = reg_value;
		return vm::reservation_update(addr, &data, sizeof(data));
	}

	static bool stdcx(u32 addr, u64 reg_value)
	{
		const be_t<u64> data = reg_value;
		return vm::reservation_update(addr, &data, sizeof(data));
	}

	static bool sraw_carry(s32 arg, u64 shift)
	{
		return (arg < 0) && (shift > 31 || (arg >> shift << shift) != arg);
	}

	static bool srad_carry(s64 arg, u64 shift)
	{
		return (arg < 0) && (shift > 63 || (arg >> shift << shift) != arg);
	}

	static bool adde_carry(u64 a, u64 b, bool c)
	{
		return _addcarry_u64(c, a, b, nullptr) != 0;
	}

	// Interpreter call for simple vector instructions
	static __m128i vec3op(decltype(&ppu_interpreter::UNK) func, __m128i _a, __m128i _b, __m128i _c)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		ppu.VR[21].vi = _a;
		ppu.VR[22].vi = _b;
		ppu.VR[23].vi = _c;

		ppu_opcode_t op{};
		op.vd = 20;
		op.va = 21;
		op.vb = 22;
		op.vc = 23;
		func(ppu, op);

		return ppu.VR[20].vi;
	}

	// Interpreter call for simple vector instructions with immediate
	static __m128i veciop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, __m128i _b)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		ppu.VR[22].vi = _b;

		op.vd = 20;
		op.vb = 22; 
		func(ppu, op);

		return ppu.VR[20].vi;
	}

	// Interpreter call for FP instructions
	static f64 fpop(decltype(&ppu_interpreter::UNK) func, f64 _a, f64 _b, f64 _c)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		ppu.FPR[21] = _a;
		ppu.FPR[22] = _b;
		ppu.FPR[23] = _c;

		ppu_opcode_t op{};
		op.frd = 20;
		op.fra = 21;
		op.frb = 22;
		op.frc = 23;
		func(ppu, op);

		return ppu.FPR[20];
	}

	// Interpreter call for GPR instructions writing result to RA
	static u64 aimmop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, u64 _s)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		const u64 a = ppu.GPR[op.ra];
		const u64 s = ppu.GPR[op.rs];
		ppu.GPR[op.rs] = _s;

		func(ppu, op);

		const u64 r = ppu.GPR[op.ra];
		ppu.GPR[op.ra] = a;
		ppu.GPR[op.rs] = s;
		return r;
	}

	// Interpreter call for GPR instructions writing result to RA
	static u64 aimmbop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, u64 _s, u64 _b)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		const u64 a = ppu.GPR[op.ra];
		const u64 s = ppu.GPR[op.rs];
		const u64 b = ppu.GPR[op.rb];
		ppu.GPR[op.rs] = _s;
		ppu.GPR[op.rb] = _b;

		func(ppu, op);

		const u64 r = ppu.GPR[op.ra];
		ppu.GPR[op.ra] = a;
		ppu.GPR[op.rs] = s;
		ppu.GPR[op.rb] = b;
		return r;
	}

	// Interpreter call for GPR instructions writing result to RA (destructive)
	static u64 aaimmop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, u64 _s, u64 _a)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		const u64 s = ppu.GPR[op.rs];
		const u64 a = ppu.GPR[op.ra];
		ppu.GPR[op.rs] = _s;
		ppu.GPR[op.ra] = _a;

		func(ppu, op);

		const u64 r = ppu.GPR[op.ra];
		ppu.GPR[op.rs] = s;
		ppu.GPR[op.ra] = a;
		return r;
	}

	static u64 immaop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, u64 _a)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		const u64 a = ppu.GPR[op.ra];
		const u64 d = ppu.GPR[op.rd];
		ppu.GPR[op.ra] = _a;

		func(ppu, op);

		const u64 r = ppu.GPR[op.rd];
		ppu.GPR[op.ra] = a;
		ppu.GPR[op.rd] = d;
		return r;
	}

	static u64 immabop(decltype(&ppu_interpreter::UNK) func, ppu_opcode_t op, u64 _a, u64 _b)
	{
		PPUThread& ppu = static_cast<PPUThread&>(*get_current_cpu_thread());
		const u64 a = ppu.GPR[op.ra];
		const u64 b = ppu.GPR[op.rb];
		const u64 d = ppu.GPR[op.rd];
		ppu.GPR[op.ra] = _a;
		ppu.GPR[op.rb] = _b;

		func(ppu, op);

		const u64 r = ppu.GPR[op.rd];
		ppu.GPR[op.ra] = a;
		ppu.GPR[op.rb] = b;
		ppu.GPR[op.rd] = d;
		return r;
	}

	// No operation on specific u64 value (silly optimization barrier)
	static u64 nop64(u64 value)
	{
		return value;
	}

	std::unordered_map<std::string, u64> table
	{
		{ "__memory", (u64)vm::base(0) },
		{ "__context", (u64)&context },
		{ "__trap", (u64)&trap },
		{ "__trace", (u64)&trace },
		{ "__hlecall", (u64)&hack },
		{ "__syscall", (u64)&syscall },
		{ "__get_tbl", (u64)&tbl },
		{ "__call", (u64)&call },
		{ "__lwarx", (u64)&lwarx },
		{ "__ldarx", (u64)&ldarx },
		{ "__stwcx", (u64)&stwcx },
		{ "__stdcx", (u64)&stdcx },
		{ "__sraw_get_ca", (u64)&sraw_carry },
		{ "__srad_get_ca", (u64)&srad_carry },
		{ "__adde_get_ca", (u64)&adde_carry },
		{ "__vexptefp", (u64)&sse_exp2_ps },
		{ "__vlogefp", (u64)&sse_log2_ps },
		{ "__vperm", (u64)&sse_altivec_vperm },
		{ "__vrefp", (u64)&sse_rcp_ps },
		{ "__vrsqrtefp", (u64)&sse_rsqrt_ps },
		{ "__vec3op", (u64)&vec3op },
		{ "__veciop", (u64)&veciop },
		{ "__aimmop", (u64)&aimmop },
		{ "__aimmbop", (u64)&aimmbop },
		{ "__aaimmop", (u64)&aaimmop },
		{ "__immaop", (u64)&immaop },
		{ "__immabop", (u64)&immabop },
		{ "__fpop", (u64)&fpop },
		{ "__nop64", (u64)&nop64 },
		{ "__lvsl", (u64)&sse_altivec_lvsl },
		{ "__lvsr", (u64)&sse_altivec_lvsr },
		{ "__lvlx", (u64)&sse_cellbe_lvlx },
		{ "__lvrx", (u64)&sse_cellbe_lvrx },
		{ "__stvlx", (u64)&sse_cellbe_stvlx },
		{ "__stvrx", (u64)&sse_cellbe_stvrx },
		{ "__fre", (u64)&sse_rcp_ss },
		{ "__frsqrte", (u64)&sse_rsqrt_ss },
	};

	virtual u64 getSymbolAddress(const std::string& name) override
	{
		if (u64 addr = RTDyldMemoryManager::getSymbolAddress(name))
		{
			LOG_ERROR(GENERAL, "LLVM: Linkage requested %s -> 0x%016llx", name, addr);
			return addr;
		}

		const auto found = table.find(name);

		if (found != table.end())
		{
			return found->second;
		}

		LOG_FATAL(GENERAL, "LLVM: Linkage failed for %s", name);
		return (u64)trap;
	}

	virtual u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		// Simple allocation (TODO)
		const auto ptr = m_next; m_next = (void*)::align((u64)m_next + size, 4096);

#ifdef _WIN32
		if (!VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
#else
		if (::mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC))
#endif
		{
			LOG_FATAL(GENERAL, "LLVM: Failed to allocate code section '%s', error %u", sec_name.data(), GetLastError());
			return nullptr;
		}

		LOG_SUCCESS(GENERAL, "LLVM: Code section '%s' allocated -> 0x%p", sec_name.data(), ptr);
		return (u8*)ptr;
	}

	virtual u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		// Simple allocation (TODO)
		const auto ptr = m_next; m_next = (void*)::align((u64)m_next + size, 4096);

#ifdef _WIN32
		if (!VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE))
#else
		if (::mprotect(ptr, size, PROT_READ | PROT_WRITE))
#endif
		{
			LOG_FATAL(GENERAL, "LLVM: Failed to allocate data section '%s', error %u", sec_name.data(), GetLastError());
			return nullptr;
		}
		
		LOG_SUCCESS(GENERAL, "LLVM: Data section '%s' allocated -> 0x%p", sec_name.data(), ptr);
		return (u8*)ptr;
	}

	virtual bool finalizeMemory(std::string* = nullptr) override
	{
		// TODO: make sections read-only when necessary
		return false;
	}

	virtual void registerEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
		s_unwind_info = addr;
		s_unwind_size = size;

		return RTDyldMemoryManager::registerEHFrames(addr, load_addr, size);
	}

	virtual void deregisterEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
		LOG_ERROR(GENERAL, "deregisterEHFrames() called"); // Not expected

		return RTDyldMemoryManager::deregisterEHFrames(addr, load_addr, size);
	}

	~MemoryManager()
	{
#ifdef _WIN32
		if (!RtlDeleteFunctionTable(s_unwind.data()))
		{
			LOG_FATAL(GENERAL, "RtlDeleteFunctionTable(addr=0x%p) failed! Error %u", s_unwind_info, GetLastError());
		}

		if (!VirtualFree(s_memory, 0, MEM_DECOMMIT))
		{
			LOG_FATAL(GENERAL, "VirtualFree(0x%p) failed! Error %u", s_memory, GetLastError());
		}
#else
		if (::mprotect(s_memory, s_memory_size, PROT_NONE))
		{
			LOG_FATAL(GENERAL, "mprotect(0x%p) failed! Error %d", s_memory, errno);
		}

		// TODO: unregister EH frames if necessary
#endif
	}

private:
	void* m_next = s_memory;
};

llvm::LLVMContext g_context;

extern void ppu_initialize(const std::string& name, const std::vector<std::pair<u32, u32>>& funcs, u32 entry)
{
	if (!s_memory)
	{
		throw std::runtime_error("LLVM: Memory not allocated, report to the developers." HERE);
	}

	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm || funcs.empty())
	{
		return;
	}

	using namespace llvm;

	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	LLVMLinkInMCJIT();

	// Initialization
	const auto _pi8 = Type::getInt8PtrTy(g_context);
	const auto _void = Type::getVoidTy(g_context);
	const auto _func = FunctionType::get(Type::getVoidTy(g_context), false);

	// Create LLVM module
	std::unique_ptr<Module> module = std::make_unique<Module>(name, g_context);

	// Initialize target
	module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	
	// Initialize translator
	std::unique_ptr<PPUTranslator> translator = std::make_unique<PPUTranslator>(g_context, module.get(), 0, entry);

	// Initialize function list
	for (const auto& info : funcs)
	{
		if (info.second)
		{
			translator->AddFunction(info.first, cast<Function>(module->getOrInsertFunction(fmt::format("__sub_%x", info.first), _func)));
		}

		translator->AddBlockInfo(info.first);
	}

	legacy::FunctionPassManager pm(module.get());
	
	// Basic optimizations
	pm.add(createCFGSimplificationPass());
	pm.add(createPromoteMemoryToRegisterPass());
	pm.add(createEarlyCSEPass());
	pm.add(createTailCallEliminationPass());
	pm.add(createReassociatePass());
	pm.add(createInstructionCombiningPass());
	//pm.add(new DominatorTreeWrapperPass());
	//pm.add(createInstructionCombiningPass());
	//pm.add(new MemoryDependenceAnalysis());
	pm.add(createDeadStoreEliminationPass());
	//pm.add(createGVNPass());
	//pm.add(createBBVectorizePass());
	//pm.add(new LoopInfo());
	//pm.add(new ScalarEvolution());

	pm.add(createSCCPPass());
	//pm.addPass(new SyscallAnalysisPass()); // Requires constant propagation
	pm.add(createInstructionCombiningPass());
	pm.add(createAggressiveDCEPass());
	pm.add(createCFGSimplificationPass());
	//pm.add(createLintPass()); // Check
	
	// Translate functions
	for (const auto& info : funcs)
	{
		if (info.second)
		{
			pm.run(*translator->TranslateToIR(info.first, info.first + info.second, vm::_ptr<u32>(info.first)));
		}
	}

	//static auto s_current = &PPUTranslator::UNK;

	//for (const auto& info : s_test)
	//{
	//	const u64 pseudo_addr = (u64)&info.first + INT64_MIN;

	//	s_current = info.second;
	//	const auto func = translator->TranslateToIR(pseudo_addr, pseudo_addr, nullptr, [](PPUTranslator* _this)
	//	{
	//		(_this->*s_current)(op);
	//		_this->ReturnFromFunction();
	//	});

	//	pm.run(*func);
	//}

	legacy::PassManager mpm;

	// Remove unused functions, structs, global variables, etc
	mpm.add(createStripDeadPrototypesPass());
	mpm.run(*module);

	std::string result;
	raw_string_ostream out(result);

	out << *module; // print IR
	fs::file(fs::get_config_dir() + "LLVM.log", fs::rewrite)
		.write(out.str());

	result.clear();

	if (verifyModule(*module, &out))
	{
		out.flush();
		LOG_ERROR(PPU, "{%s} LLVM: Translation failed:\n%s", name, result);
		return;
	}

	LOG_SUCCESS(PPU, "LLVM: %zu functions generated", module->getFunctionList().size());

	Module* module_ptr = module.get();

	std::shared_ptr<ExecutionEngine> engine(EngineBuilder(std::move(module))
		.setErrorStr(&result)
		.setMCJITMemoryManager(std::make_unique<MemoryManager>())
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setRelocationModel(Reloc::PIC_)
		.setCodeModel((u64)s_memory <= 0x60000000 ? CodeModel::Medium : CodeModel::Large)
		.setMCPU(sys::getHostCPUName())
		.create());

	if (!engine)
	{
		throw fmt::exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}

	engine->setProcessAllSections(true);
	//engine->setVerifyModules(true);
	engine->RegisterJITEventListener(&s_listener);
	engine->finalizeObject();

	s_ppu_compiled.clear();

	// Get function addresses
	for (const auto& info : funcs)
	{
		if (info.second)
		{
			const std::uintptr_t link = engine->getFunctionAddress(fmt::format("__sub_%x", info.first));
			s_ppu_compiled.emplace(info.first, (void(*)())link);

			LOG_NOTICE(PPU, "** Function __sub_%x -> 0x%llx (addr=0x%x, size=0x%x)", info.first, link, info.first, info.second);
		}
	}

	// Delete IR to lower memory consumption
	for (auto& func : module_ptr->functions())
	{
		func.deleteBody();
	}

#ifdef _WIN32
	// Register .xdata UNWIND_INFO (.pdata section is empty for some reason)
	std::set<u64> func_set;

	for (const auto& pair : s_ppu_compiled)
	{
		// Get addresses
		func_set.emplace((u64)pair.second);
	}

	func_set.emplace(::align(*--func_set.end() + 4096, 4096));

	const u64 base = (u64)s_memory;
	const u8* bits = s_unwind_info;

	s_unwind.clear();
	s_unwind.reserve(s_ppu_compiled.size());

	for (auto it = func_set.begin(), end = --func_set.end(); it != end; it++)
	{
		const u64 addr = *it;
		const u64 next = *func_set.upper_bound(addr);

		// Generate RUNTIME_FUNCTION record
		RUNTIME_FUNCTION uw;
		uw.BeginAddress = static_cast<u32>(addr - base);
		uw.EndAddress   = static_cast<u32>(next - base);
		uw.UnwindData   = static_cast<u32>((u64)bits - base);
		s_unwind.emplace_back(uw);

		// Parse .xdata record
		VERIFY(*bits++ == 1); // Version and flags
		bits++; // Size of prolog
		const u8 count = *bits++; // Count of unwind codes
		bits++; // Frame Reg + Off
		bits += ::align(count, 2) * sizeof(u16); // UNWIND_CODE array
		while (!*bits && bits < s_unwind_info + s_unwind_size) bits++; // Skip strange zero padding (???)
	}

	VERIFY(bits == s_unwind_info + s_unwind_size);
	VERIFY(RtlAddFunctionTable(s_unwind.data(), (DWORD)s_unwind.size(), base));
	LOG_SUCCESS(GENERAL, "LLVM: UNWIND_INFO registered (addr=0x%p, size=0x%llx)", s_unwind_info, s_unwind_size);
#endif

	fxm::import<ExecutionEngine>(WRAP_EXPR(engine));

	LOG_SUCCESS(PPU, "LLVM: Compilation finished (%s)", sys::getHostCPUName().data());
}

#else

extern void ppu_initialize(const std::string& name, const std::vector<std::pair<u32, u32>>& funcs, u32 entry)
{
}

#endif
