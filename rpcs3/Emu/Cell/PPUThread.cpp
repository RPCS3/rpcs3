#include "stdafx.h"
#include "Utilities/Config.h"
#include "Utilities/VirtualMemory.h"
#include "Crypto/sha1.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"
#include "lv2/sys_sync.h"
#include "lv2/sys_prx.h"

#ifdef LLVM_AVAILABLE
#include "restore_new.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/Support/FormattedStream.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/IR/Dominators.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
//#include "llvm/IR/Module.h"
//#include "llvm/IR/Function.h"
//#include "llvm/Analysis/Passes.h"
//#include "llvm/Analysis/BasicAliasAnalysis.h"
//#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
//#include "llvm/Analysis/LoopInfo.h"
//#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "define_new_memleakdetect.h"

#include "Utilities/JIT.h"
#include "PPUTranslator.h"
#include "Modules/cellMsgDialog.h"
#endif

#include <cfenv>
#include "Utilities/GSL.h"

extern u64 get_system_time();

namespace vm { using namespace ps3; }

enum class join_status : u32
{
	joinable = 0,
	detached = 0u-1,
	exited = 0u-2,
	zombie = 0u-3,
};

template <>
void fmt_class_string<join_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](join_status js)
	{
		switch (js)
		{
		case join_status::joinable: return "";
		case join_status::detached: return "detached";
		case join_status::zombie: return "zombie";
		case join_status::exited: return "exited";
		}

		return unknown;
	});
}

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

extern void ppu_initialize();
extern void ppu_initialize(const ppu_module& info);
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);
extern void ppu_execute_function(ppu_thread& ppu, u32 index);

const auto s_ppu_compiled = static_cast<u32*>(utils::memory_reserve(0x100000000));

extern void ppu_finalize()
{
	utils::memory_decommit(s_ppu_compiled, 0x100000000);
}

// Get interpreter cache value
static u32 ppu_cache(u32 addr)
{
	// Select opcode table
	const auto& table = *(
		g_cfg_ppu_decoder.get() == ppu_decoder_type::precise ? &s_ppu_interpreter_precise.get_table() :
		g_cfg_ppu_decoder.get() == ppu_decoder_type::fast ? &s_ppu_interpreter_fast.get_table() :
		(fmt::throw_exception<std::logic_error>("Invalid PPU decoder"), nullptr));

	return ::narrow<u32>(reinterpret_cast<std::uintptr_t>(table[ppu_decode(vm::read32(addr))]));
}

static bool ppu_fallback(ppu_thread& ppu, ppu_opcode_t op)
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		fmt::throw_exception("Unregistered PPU function [0x%08x]", ppu.cia);
	}

	s_ppu_compiled[ppu.cia / 4] = ppu_cache(ppu.cia);
	return false;
}

extern void ppu_register_range(u32 addr, u32 size)
{
	if (!size)
	{
		LOG_ERROR(PPU, "ppu_register_range(0x%x): empty range", addr);
		return;	
	}

	// Register executable range at
	utils::memory_commit(s_ppu_compiled + addr / 4, size);

	const u32 fallback = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(ppu_fallback));

	while (size)
	{
		s_ppu_compiled[addr / 4] = fallback;
		addr += 4;
		size -= 4;
	}
}

extern void ppu_register_function_at(u32 addr, u32 size, ppu_function_t ptr)
{
	if (!size)
	{
		LOG_ERROR(PPU, "ppu_register_function_at(0x%x): empty range", addr);
		return;	
	}

	ppu_register_range(addr, size);

	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		s_ppu_compiled[addr / 4] = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(ptr));
		return;
	}

	// Initialize interpreter cache
	while (size)
	{
		s_ppu_compiled[addr / 4] = ppu_cache(addr);
		addr += 4;
		size -= 4;
	}
}

// Breakpoint entry point
static bool ppu_break(ppu_thread& ppu, ppu_opcode_t op)
{
	// Pause and wait if necessary
	if (!ppu.state.test_and_set(cpu_flag::dbg_pause) && ppu.check_state())
	{
		return false;
	}

	// Fallback to the interpreter function
	if (reinterpret_cast<decltype(&ppu_interpreter::UNK)>(std::uintptr_t{ppu_cache(ppu.cia)})(ppu, op))
	{
		ppu.cia += 4;
	}

	return false;
}

// Set or remove breakpoint
extern void ppu_breakpoint(u32 addr)
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		return;
	}

	const auto _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_break));

	if (s_ppu_compiled[addr / 4] == _break)
	{
		// Remove breakpoint
		s_ppu_compiled[addr / 4] = ppu_cache(addr);
	}
	else
	{
		// Set breakpoint
		s_ppu_compiled[addr / 4] = _break;
	}
}

void ppu_thread::on_init(const std::shared_ptr<void>& _this)
{
	if (!stack_addr)
	{
		const_cast<u32&>(stack_addr) = vm::alloc(stack_size, vm::stack);

		if (!stack_addr)
		{
			fmt::throw_exception("Out of stack memory (size=0x%x)" HERE, stack_size);
		}

		gpr[1] = ::align(stack_addr + stack_size, 0x200) - 0x200;

		cpu_thread::on_init(_this);
	}
}

std::string ppu_thread::get_name() const
{
	return fmt::format("PPU[0x%x] Thread (%s)", id, m_name);
}

std::string ppu_thread::dump() const
{
	std::string ret = cpu_thread::dump();
	ret += fmt::format("Priority: %d\n", +prio);
	ret += fmt::format("Stack: 0x%x..0x%x\n", stack_addr, stack_addr + stack_size - 1);
	ret += fmt::format("Joiner: %s\n", join_status(joiner.load()));
	ret += fmt::format("Commands: %u\n", cmd_queue.size());

	const auto _func = last_function;

	if (_func)
	{
		ret += "Last function: ";
		ret += _func;
		ret += '\n';
	}

	if (const auto _time = start_time)
	{
		ret += fmt::format("Waiting: %fs\n", (get_system_time() - _time) / 1000000.);
	}
	else
	{
		ret += '\n';
	}

	if (!_func)
	{
		ret += '\n';
	}
	
	ret += "\nRegisters:\n=========\n";
	for (uint i = 0; i < 32; ++i) ret += fmt::format("GPR[%d] = 0x%llx\n", i, gpr[i]);
	for (uint i = 0; i < 32; ++i) ret += fmt::format("FPR[%d] = %.6G\n", i, fpr[i]);
	for (uint i = 0; i < 32; ++i) ret += fmt::format("VR[%d] = %s [x: %g y: %g z: %g w: %g]\n", i, vr[i], vr[i]._f[3], vr[i]._f[2], vr[i]._f[1], vr[i]._f[0]);

	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm)
	{
		ret += fmt::format("CR = 0x%08x\n", cr_pack());
		ret += fmt::format("LR = 0x%llx\n", lr);
		ret += fmt::format("CTR = 0x%llx\n", ctr);
		ret += fmt::format("VRSAVE = 0x%08x\n", vrsave);
		ret += fmt::format("XER = [CA=%u | OV=%u | SO=%u | CNT=%u]\n", xer.ca, xer.ov, xer.so, xer.cnt);
		ret += fmt::format("VSCR = [SAT=%u | NJ=%u]\n", sat, nj);
		ret += fmt::format("FPSCR = [FL=%u | FG=%u | FE=%u | FU=%u]\n", fpscr.fl, fpscr.fg, fpscr.fe, fpscr.fu);

		// TODO: support foreign stack
		ret += "\nCall stack:\n=========\n";
		ret += fmt::format("0x%08x (0x0) called\n", cia);
		const u32 stack_max = ::align(stack_addr + stack_size, 0x200) - 0x200;
		for (u64 sp = vm::read64(static_cast<u32>(gpr[1])); sp >= stack_addr && sp < stack_max; sp = vm::read64(static_cast<u32>(sp)))
		{
			// TODO: print also function addresses
			ret += fmt::format("> from 0x%08llx (0x0)\n", vm::read64(static_cast<u32>(sp + 16)));
		}
	}

	return ret;
}

extern thread_local std::string(*g_tls_log_prefix)();

void ppu_thread::cpu_task()
{
	std::fesetround(FE_TONEAREST);

	// Execute cmd_queue
	while (cmd64 cmd = cmd_wait())
	{
		const u32 arg = cmd.arg2<u32>(); // 32-bit arg extracted

		switch (auto type = cmd.arg1<ppu_cmd>())
		{
		case ppu_cmd::opcode:
		{
			cmd_pop(), s_ppu_interpreter_fast.decode(arg)(*this, {arg});
			break;
		}
		case ppu_cmd::set_gpr:
		{
			if (arg >= 32)
			{
				fmt::throw_exception("Invalid ppu_cmd::set_gpr arg (0x%x)" HERE, arg);
			}

			gpr[arg % 32] = cmd_get(1).as<u64>();
			cmd_pop(1);
			break;
		}
		case ppu_cmd::set_args:
		{
			if (arg > 8)
			{
				fmt::throw_exception("Unsupported ppu_cmd::set_args size (0x%x)" HERE, arg);
			}

			for (u32 i = 0; i < arg; i++)
			{
				gpr[i + 3] = cmd_get(1 + i).as<u64>();
			}

			cmd_pop(arg);
			break;
		}
		case ppu_cmd::lle_call:
		{
			const vm::ptr<u32> opd(arg < 32 ? vm::cast(gpr[arg]) : vm::cast(arg));
			cmd_pop(), fast_call(opd[0], opd[1]);
			break;
		}
		case ppu_cmd::hle_call:
		{
			cmd_pop(), ppu_execute_function(*this, arg);
			break;
		}
		case ppu_cmd::initialize:
		{
			cmd_pop(), ppu_initialize();
			break;
		}
		case ppu_cmd::sleep:
		{
			cmd_pop(), lv2_obj::sleep(*this);
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown ppu_cmd(0x%x)" HERE, (u32)type);
		}
		}
	}
}

void ppu_thread::exec_task()
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		return reinterpret_cast<ppu_function_t>((std::uintptr_t)s_ppu_compiled[cia / 4])(*this);
	}

	const auto base = vm::_ptr<const u8>(0);
	const auto cache = reinterpret_cast<const u8*>(s_ppu_compiled);
	const auto bswap4 = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);

	v128 _op;
	using func_t = decltype(&ppu_interpreter::UNK);
	func_t func0, func1, func2, func3, func4, func5;

	while (true)
	{
		if (UNLIKELY(test(state)))
		{
			if (check_state()) return;

			// Decode single instruction (may be step)
			const u32 op = *reinterpret_cast<const be_t<u32>*>(base + cia);
			if (reinterpret_cast<func_t>((std::uintptr_t)s_ppu_compiled[cia / 4])(*this, {op})) { cia += 4; }
			continue;
		}

		if (cia % 16)
		{
			// Unaligned
			const u32 op = *reinterpret_cast<const be_t<u32>*>(base + cia);
			if (reinterpret_cast<func_t>((std::uintptr_t)s_ppu_compiled[cia / 4])(*this, {op})) { cia += 4; }
			continue;
		}

		// Reinitialize
		{
			const v128 x = v128::fromV(_mm_load_si128(reinterpret_cast<const __m128i*>(cache + cia)));
			func0 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[0]);
			func1 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[1]);
			func2 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[2]);
			func3 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[3]);
			_op.vi =  _mm_shuffle_epi8(_mm_load_si128(reinterpret_cast<const __m128i*>(base + cia)), bswap4);
		}

		while (LIKELY(func0(*this, {_op._u32[0]})))
		{
			cia += 4;

			if (LIKELY(func1(*this, {_op._u32[1]})))
			{
				cia += 4;

				const v128 x = v128::fromV(_mm_load_si128(reinterpret_cast<const __m128i*>(cache + cia + 8)));
				func0 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[0]);
				func1 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[1]);
				func4 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[2]);
				func5 = reinterpret_cast<func_t>((std::uintptr_t)x._u32[3]);

				if (LIKELY(func2(*this, {_op._u32[2]})))
				{
					cia += 4;

					if (LIKELY(func3(*this, {_op._u32[3]})))
					{
						cia += 4;

						func2 = func4;
						func3 = func5;

						if (UNLIKELY(test(state)))
						{
							break;
						}

						_op.vi = _mm_shuffle_epi8(_mm_load_si128(reinterpret_cast<const __m128i*>(base + cia)), bswap4);
						continue;
					}
					break;
				}
				break;
			}
			break;
		}
	}
}

ppu_thread::~ppu_thread()
{
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
	}
}

ppu_thread::ppu_thread(const std::string& name, u32 prio, u32 stack)
	: cpu_thread(idm::last_id())
	, prio(prio)
	, stack_size(std::max<u32>(stack, 0x4000))
	, stack_addr(0)
	, start_time(get_system_time())
	, m_name(name)
{
	// Trigger the scheduler
	state += cpu_flag::suspend + cpu_flag::memory;
}

void ppu_thread::cmd_push(cmd64 cmd)
{
	// Reserve queue space
	const u32 pos = cmd_queue.push_begin();

	// Write single command
	cmd_queue[pos] = cmd;
}

void ppu_thread::cmd_list(std::initializer_list<cmd64> list)
{
	// Reserve queue space
	const u32 pos = cmd_queue.push_begin(static_cast<u32>(list.size()));

	// Write command tail in relaxed manner
	for (u32 i = 1; i < list.size(); i++)
	{
		cmd_queue[pos + i].raw() = list.begin()[i];
	}

	// Write command head after all
	cmd_queue[pos] = *list.begin();
}

void ppu_thread::cmd_pop(u32 count)
{
	// Get current position
	const u32 pos = cmd_queue.peek();

	// Clean command buffer for command tail
	for (u32 i = 1; i <= count; i++)
	{
		cmd_queue[pos + i].raw() = cmd64{};
	}

	// Free
	cmd_queue.pop_end(count + 1);
}

cmd64 ppu_thread::cmd_wait()
{
	while (true)
	{
		if (UNLIKELY(test(state)))
		{
			if (test(state, cpu_flag::stop + cpu_flag::exit))
			{
				return cmd64{};
			}
		}

		if (cmd64 result = cmd_queue[cmd_queue.peek()].exchange(cmd64{}))
		{
			return result;
		}

		thread_ctrl::wait();
	}
}

be_t<u64>* ppu_thread::get_stack_arg(s32 i, u64 align)
{
	if (align != 1 && align != 2 && align != 4 && align != 8 && align != 16) fmt::throw_exception("Unsupported alignment: 0x%llx" HERE, align);
	return vm::_ptr<u64>(vm::cast((gpr[1] + 0x30 + 0x8 * (i - 1)) & (0 - align), HERE));
}

void ppu_thread::fast_call(u32 addr, u32 rtoc)
{
	const auto old_cia = cia;
	const auto old_rtoc = gpr[2];
	const auto old_lr = lr;
	const auto old_func = last_function;
	const auto old_fmt = g_tls_log_prefix;

	cia = addr;
	gpr[2] = rtoc;
	lr = Emu.GetCPUThreadStop();
	last_function = nullptr;

	g_tls_log_prefix = []
	{
		const auto _this = static_cast<ppu_thread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", _this->get_name(), _this->cia);
	};

	auto at_ret = gsl::finally([&]()
	{
		if (std::uncaught_exception())
		{
			if (last_function)
			{
				if (start_time)
				{
					LOG_WARNING(PPU, "'%s' aborted (%fs)", last_function, (get_system_time() - start_time) / 1000000.);
				}
				else
				{
					LOG_WARNING(PPU, "'%s' aborted", last_function);
				}
			}

			last_function = old_func;
		}
		else
		{
			state -= cpu_flag::ret;
			cia = old_cia;
			gpr[2] = old_rtoc;
			lr = old_lr;
			last_function = old_func;
			g_tls_log_prefix = old_fmt;
		}
	});

	try
	{
		exec_task();
	}
	catch (cpu_flag _s)
	{
		state += _s;

		if (_s != cpu_flag::ret)
		{
			throw;
		}
	}
}

u32 ppu_thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id_type() == 1)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		const u32 old_pos = vm::cast(context.gpr[1], HERE);
		context.gpr[1] -= align(size + 4, 8); // room minimal possible size
		context.gpr[1] &= ~(align_v - 1); // fix stack alignment

		if (old_pos >= context.stack_addr && old_pos < context.stack_addr + context.stack_size && context.gpr[1] < context.stack_addr)
		{
			fmt::throw_exception("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)" HERE, size, align_v, old_pos, context.stack_addr);
		}
		else
		{
			const u32 addr = static_cast<u32>(context.gpr[1]);
			vm::ps3::_ref<nse_t<u32>>(addr + size) = old_pos;
			std::memset(vm::base(addr), 0, size);
			return addr;
		}
	}

	fmt::throw_exception("Invalid thread" HERE);
}

void ppu_thread::stack_pop_verbose(u32 addr, u32 size) noexcept
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id_type() == 1)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		if (context.gpr[1] != addr)
		{
			LOG_ERROR(PPU, "Stack inconsistency (addr=0x%x, SP=0x%llx, size=0x%x)", addr, context.gpr[1], size);
			return;
		}

		context.gpr[1] = vm::ps3::_ref<nse_t<u32>>(context.gpr[1] + size);
		return;
	}

	LOG_ERROR(PPU, "Invalid thread" HERE);
}

const ppu_decoder<ppu_itype> s_ppu_itype;

extern u64 get_timebased_time();
extern ppu_function_t ppu_get_syscall(u64 code);
extern std::string ppu_get_syscall_name(u64 code);
extern ppu_function_t ppu_get_function(u32 index);
extern std::string ppu_get_module_function_name(u32 index);

extern __m128 sse_exp2_ps(__m128 A);
extern __m128 sse_log2_ps(__m128 A);
extern __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_lvsl(u64 addr);
extern __m128i sse_altivec_lvsr(u64 addr);
extern __m128i sse_cellbe_lvlx(u64 addr);
extern __m128i sse_cellbe_lvrx(u64 addr);
extern void sse_cellbe_stvlx(u64 addr, __m128i a);
extern void sse_cellbe_stvrx(u64 addr, __m128i a);

[[noreturn]] static void ppu_trap(u64 addr)
{
	fmt::throw_exception("Trap! (0x%llx)", addr);
}

[[noreturn]] static void ppu_unreachable(u64 addr)
{
	fmt::throw_exception("Unreachable! (0x%llx)", addr);
}

static void ppu_check(ppu_thread& ppu, u64 addr)
{
	ppu.cia = addr;
	ppu.test_state();
}

static void ppu_trace(u64 addr)
{
	LOG_NOTICE(PPU, "Trace: 0x%llx", addr);
}

extern u32 ppu_lwarx(ppu_thread& ppu, u32 addr)
{
	ppu.rtime = vm::reservation_acquire(addr, sizeof(u32));
	_mm_lfence();
	ppu.raddr = addr;
	ppu.rdata = vm::_ref<const atomic_be_t<u32>>(addr);
	return static_cast<u32>(ppu.rdata);
}

extern u64 ppu_ldarx(ppu_thread& ppu, u32 addr)
{
	ppu.rtime = vm::reservation_acquire(addr, sizeof(u64));
	_mm_lfence();
	ppu.raddr = addr;
	ppu.rdata = vm::_ref<const atomic_be_t<u64>>(addr);
	return ppu.rdata;
}

extern bool ppu_stwcx(ppu_thread& ppu, u32 addr, u32 reg_value)
{
	atomic_be_t<u32>& data = vm::_ref<atomic_be_t<u32>>(addr);

	if (ppu.raddr != addr || ppu.rdata != data.load())
	{
		ppu.raddr = 0;
		return false;
	}

	vm::writer_lock lock(0);

	const bool result = ppu.rtime == vm::reservation_acquire(addr, sizeof(u32)) && data.compare_and_swap_test(static_cast<u32>(ppu.rdata), reg_value);
	
	if (result)
	{
		vm::reservation_update(addr, sizeof(u32));
		vm::notify(addr, sizeof(u32));
	}

	ppu.raddr = 0;
	return result;
}

extern bool ppu_stdcx(ppu_thread& ppu, u32 addr, u64 reg_value)
{
	atomic_be_t<u64>& data = vm::_ref<atomic_be_t<u64>>(addr);

	if (ppu.raddr != addr || ppu.rdata != data.load())
	{
		ppu.raddr = 0;
		return false;
	}

	vm::writer_lock lock(0);

	const bool result = ppu.rtime == vm::reservation_acquire(addr, sizeof(u64)) && data.compare_and_swap_test(ppu.rdata, reg_value);

	if (result)
	{
		vm::reservation_update(addr, sizeof(u64));
		vm::notify(addr, sizeof(u64));
	}

	ppu.raddr = 0;
	return result;
}

static bool adde_carry(u64 a, u64 b, bool c)
{
#ifdef _MSC_VER
	return _addcarry_u64(c, a, b, nullptr) != 0;
#else
	bool result;
	__asm__("addb $0xff, %[c] \n adcq %[a], %[b] \n setb %[result]" : [a] "+&r" (a), [b] "+&r" (b), [c] "+&r" (c), [result] "=r" (result));
	return result;
#endif
}

extern void ppu_initialize()
{
	const auto _funcs = fxm::withdraw<std::vector<ppu_function>>();

	if (!_funcs)
	{
		return;
	}

	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm || _funcs->empty())
	{
		if (!Emu.GetCPUThreadStop())
		{
			auto ppu_thr_stop_data = vm::ptr<u32>::make(vm::alloc(2 * 4, vm::main));
			Emu.SetCPUThreadStop(ppu_thr_stop_data.addr());
			ppu_thr_stop_data[0] = ppu_instructions::HACK(1);
			ppu_thr_stop_data[1] = ppu_instructions::BLR();
			ppu_register_function_at(ppu_thr_stop_data.addr(), 8, nullptr);
		}

		for (const auto& func : *_funcs)
		{
			ppu_register_function_at(func.addr, func.size, nullptr);
		}

		return;
	}

	std::size_t fpos = 0;

	while (fpos < _funcs->size())
	{
		// Split module (TODO)
		ppu_module info;
		info.name = fmt::format("%05X", _funcs->at(fpos).addr);
		info.funcs.reserve(2000);
		
		while (fpos < _funcs->size() && info.funcs.size() < 2000)
		{
			info.funcs.emplace_back(std::move(_funcs->at(fpos++)));
		}

		if (!Emu.IsStopped())
		{
			ppu_initialize(info);
		}
	}

	idm::select<lv2_obj, lv2_prx>([](u32, lv2_prx& prx)
	{
		if (!Emu.IsStopped())
		{
			ppu_initialize(prx);
		}
	});	
}

extern void ppu_initialize(const ppu_module& info)
{
	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm)
	{
		for (const auto& func : info.funcs)
		{
			ppu_register_function_at(func.addr, func.size, nullptr);
		}

		return;
	}

	// Compute module hash
	std::string obj_name;
	{
		sha1_context ctx;
		u8 output[20];
		sha1_starts(&ctx);

		for (const auto& func : info.funcs)
		{
			if (func.size == 0)
			{
				continue;
			}

			const be_t<u32> addr = func.addr;
			const be_t<u32> size = func.size;
			sha1_update(&ctx, reinterpret_cast<const u8*>(&addr), sizeof(addr));
			sha1_update(&ctx, reinterpret_cast<const u8*>(&size), sizeof(size));

			for (const auto& block : func.blocks)
			{
				if (block.second == 0)
				{
					continue;
				}

				sha1_update(&ctx, vm::ps3::_ptr<const u8>(block.first), block.second);
			}
		}
		
		sha1_finish(&ctx, output);

		// Version, module name and hash: vX-liblv2.sprx-0123456789ABCDEF.obj
		fmt::append(obj_name, "v0-%s-%016X.obj", info.name, reinterpret_cast<be_t<u64>&>(output));
	}

#ifdef LLVM_AVAILABLE
	using namespace llvm;

	if (!fxm::check<jit_compiler>())
	{
		std::unordered_map<std::string, std::uintptr_t> link_table
		{
			{ "__mptr", (u64)&vm::g_base_addr },
			{ "__cptr", (u64)&s_ppu_compiled },
			{ "__trap", (u64)&ppu_trap },
			{ "__end", (u64)&ppu_unreachable },
			{ "__check", (u64)&ppu_check },
			{ "__trace", (u64)&ppu_trace },
			{ "__hlecall", (u64)&ppu_execute_function },
			{ "__syscall", (u64)&ppu_execute_syscall },
			{ "__get_tb", (u64)&get_timebased_time },
			{ "__lwarx", (u64)&ppu_lwarx },
			{ "__ldarx", (u64)&ppu_ldarx },
			{ "__stwcx", (u64)&ppu_stwcx },
			{ "__stdcx", (u64)&ppu_stdcx },
			{ "__adde_get_ca", (u64)&adde_carry },
			{ "__vexptefp", (u64)&sse_exp2_ps },
			{ "__vlogefp", (u64)&sse_log2_ps },
			{ "__vperm", (u64)&sse_altivec_vperm },
			{ "__lvsl", (u64)&sse_altivec_lvsl },
			{ "__lvsr", (u64)&sse_altivec_lvsr },
			{ "__lvlx", (u64)&sse_cellbe_lvlx },
			{ "__lvrx", (u64)&sse_cellbe_lvrx },
			{ "__stvlx", (u64)&sse_cellbe_stvlx },
			{ "__stvrx", (u64)&sse_cellbe_stvrx },
		};

		for (u64 index = 0; index < 1024; index++)
		{
			if (auto sc = ppu_get_syscall(index))
			{
				link_table.emplace(ppu_get_syscall_name(index), (u64)sc);
			}
		}

		for (u64 index = 1; ; index++)
		{
			if (auto func = ppu_get_function(index))
			{
				link_table.emplace(ppu_get_module_function_name(index), (u64)func);
			}
			else
			{
				break;
			}
		}

		const auto jit = fxm::make<jit_compiler>(std::move(link_table));

		LOG_SUCCESS(PPU, "LLVM: JIT initialized (%s)", jit->cpu());
	}

	// Initialize compiler
	const auto jit = fxm::get<jit_compiler>();

	// Create LLVM module
	std::unique_ptr<Module> module = std::make_unique<Module>(obj_name, g_llvm_ctx);

	// Initialize target
	module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	
	// Initialize translator
	std::unique_ptr<PPUTranslator> translator = std::make_unique<PPUTranslator>(g_llvm_ctx, module.get(), 0);

	// Define some types
	const auto _void = Type::getVoidTy(g_llvm_ctx);
	const auto _func = FunctionType::get(_void, { translator->GetContextType()->getPointerTo() }, false);

	// Initialize function list
	for (const auto& func : info.funcs)
	{
		if (func.size)
		{
			const auto f = cast<Function>(module->getOrInsertFunction(fmt::format("__0x%x", func.addr), _func));
			f->addAttribute(1, Attribute::NoAlias);
			translator->AddFunction(func.addr, f);
		}
	}

	if (fs::file cached{Emu.GetCachePath() + obj_name})
	{
		std::string buf;
		buf.reserve(cached.size());
		cached.read(buf, cached.size());
		auto buffer = llvm::MemoryBuffer::getMemBuffer(buf, obj_name);
		auto result = llvm::object::ObjectFile::createObjectFile(*buffer);
		
		if (result)
		{
			jit->load(std::move(module), std::move(result.get()));

			for (const auto& func : info.funcs)
			{
				if (func.size)
				{
					const std::uintptr_t link = jit->get(fmt::format("__0x%x", func.addr));
					s_ppu_compiled[func.addr / 4] = ::narrow<u32>(link);
				}
			}

			LOG_SUCCESS(PPU, "LLVM: Loaded executable: %s", obj_name);
			return;
		}
		
		LOG_ERROR(PPU, "LLVM: Failed to load executable: %s", obj_name);
	}

	legacy::FunctionPassManager pm(module.get());
	
	// Basic optimizations
	pm.add(createCFGSimplificationPass());
	pm.add(createPromoteMemoryToRegisterPass());
	pm.add(createEarlyCSEPass());
	pm.add(createTailCallEliminationPass());
	pm.add(createReassociatePass());
	pm.add(createInstructionCombiningPass());
	//pm.add(createBasicAAWrapperPass());
	//pm.add(new MemoryDependenceAnalysis());
	pm.add(createLICMPass());
	pm.add(createLoopInstSimplifyPass());
	pm.add(createGVNPass());
	pm.add(createDeadStoreEliminationPass());
	pm.add(createSCCPPass());
	pm.add(createInstructionCombiningPass());
	pm.add(createInstructionSimplifierPass());
	pm.add(createAggressiveDCEPass());
	pm.add(createCFGSimplificationPass());
	//pm.add(createLintPass()); // Check

	// Initialize message dialog
	const auto dlg = Emu.GetCallbacks().get_msg_dialog();
	dlg->type.se_normal = true;
	dlg->type.bg_invisible = true;
	dlg->type.progress_bar_count = 1;
	dlg->on_close = [](s32 status)
	{
		Emu.CallAfter([]()
		{
			// Abort everything
			Emu.Stop();
		});
	};

	Emu.CallAfter([=]()
	{
		dlg->Create("Compiling PPU executable: " + info.name + "\nPlease wait...");
	});

	// Translate functions
	for (size_t fi = 0, fmax = info.funcs.size(); fi < fmax; fi++)
	{
		if (Emu.IsStopped())
		{
			LOG_SUCCESS(PPU, "LLVM: Translation cancelled");
			return;
		}

		if (info.funcs[fi].size)
		{
			// Update dialog		
			Emu.CallAfter([=, max = info.funcs.size()]()
			{
				dlg->ProgressBarSetMsg(0, fmt::format("Compiling %u of %u", fi + 1, fmax));

				if (fi * 100 / fmax != (fi + 1) * 100 / fmax)
					dlg->ProgressBarInc(0, 1);
			});

			// Translate
			const auto func = translator->TranslateToIR(info.funcs[fi], vm::_ptr<u32>(info.funcs[fi].addr));

			// Run optimization passes
			pm.run(*func);

			const auto _syscall = module->getFunction("__syscall");
			const auto _hlecall = module->getFunction("__hlecall");

			for (auto i = inst_begin(*func), end = inst_end(*func); i != end;)
			{
				const auto inst = &*i++;

				if (const auto ci = dyn_cast<CallInst>(inst))
				{
					const auto cif = ci->getCalledFunction();
					const auto op1 = ci->getNumArgOperands() > 1 ? ci->getArgOperand(1) : nullptr;

					if (cif == _syscall && op1 && isa<ConstantInt>(op1))
					{
						// Try to determine syscall using the value from r11 (requires constant propagation)
						const u64 index = cast<ConstantInt>(op1)->getZExtValue();

						if (const auto ptr = ppu_get_syscall(index))
						{
							const auto n = ppu_get_syscall_name(index);
							const auto f = cast<Function>(module->getOrInsertFunction(n, _func));

							// Call the syscall directly
							ReplaceInstWithInst(ci, CallInst::Create(f, {ci->getArgOperand(0)}));
						}
					}

					if (cif == _hlecall && op1 && isa<ConstantInt>(op1))
					{
						const u32 index = static_cast<u32>(cast<ConstantInt>(op1)->getZExtValue());

						if (const auto ptr = ppu_get_function(index))
						{
							const auto n = ppu_get_module_function_name(index);
							const auto f = cast<Function>(module->getOrInsertFunction(n, _func));

							// Call the function directly
							ReplaceInstWithInst(ci, CallInst::Create(f, {ci->getArgOperand(0)}));
						}
					}

					continue;
				}

				if (const auto li = dyn_cast<LoadInst>(inst))
				{
					// TODO: more careful check
					if (li->getNumUses() == 0)
					{
						// Remove unreferenced volatile loads
						li->eraseFromParent();
					}

					continue;
				}

				if (const auto si = dyn_cast<StoreInst>(inst))
				{
					// TODO: more careful check
					if (isa<UndefValue>(si->getOperand(0)) && si->getParent() == &func->getEntryBlock())
					{
						// Remove undef volatile stores
						si->eraseFromParent();
					}

					continue;
				}
			}
		}
	}

	legacy::PassManager mpm;

	// Remove unused functions, structs, global variables, etc
	mpm.add(createStripDeadPrototypesPass());
	//mpm.add(createFunctionInliningPass());
	mpm.add(createDeadInstEliminationPass());
	mpm.run(*module);

	// Update dialog
	Emu.CallAfter([=]()
	{
		dlg->ProgressBarSetMsg(0, "Generating code...");
		dlg->ProgressBarInc(0, 100);
	});

	std::string result;
	raw_string_ostream out(result);

	out << *module; // print IR
	fs::file(fs::get_config_dir() + "LLVM.log", fs::rewrite)
		.write(out.str());

	result.clear();

	if (verifyModule(*module, &out))
	{
		out.flush();
		LOG_ERROR(PPU, "LLVM: Translation failed:\n%s", result);
		return;
	}

	LOG_NOTICE(PPU, "LLVM: %zu functions generated", module->getFunctionList().size());

	jit->make(std::move(module), Emu.GetCachePath() + obj_name);

	// Get and install function addresses
	for (const auto& func : info.funcs)
	{
		if (func.size)
		{
			const std::uintptr_t link = jit->get(fmt::format("__0x%x", func.addr));
			s_ppu_compiled[func.addr / 4] = ::narrow<u32>(link);
		}
	}

	LOG_SUCCESS(PPU, "LLVM: Created executable: %s", obj_name);
#endif
}
