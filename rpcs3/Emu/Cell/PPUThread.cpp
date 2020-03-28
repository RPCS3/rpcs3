#include "stdafx.h"
#include "Utilities/VirtualMemory.h"
#include "Utilities/sysinfo.h"
#include "Utilities/JIT.h"
#include "Crypto/sha1.h"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/VFS.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"
#include "SPURecompiler.h"
#include "lv2/sys_sync.h"
#include "lv2/sys_prx.h"
#include "Emu/GDB.h"

#ifdef LLVM_AVAILABLE
#include "restore_new.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/ADT/Triple.h"
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
//#include "llvm/Analysis/MemoryDependenceAnalysis.h"
//#include "llvm/Analysis/LoopInfo.h"
//#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
#include "define_new_memleakdetect.h"

#include "PPUTranslator.h"
#endif

#include <thread>
#include <cfenv>

const bool s_use_ssse3 = utils::has_ssse3();

extern u64 get_guest_system_time();

extern atomic_t<const char*> g_progr;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;

template <>
void fmt_class_string<ppu_join_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_join_status js)
	{
		switch (js)
		{
		case ppu_join_status::joinable: return "";
		case ppu_join_status::detached: return "detached";
		case ppu_join_status::zombie: return "zombie";
		case ppu_join_status::exited: return "exited";
		case ppu_join_status::max: break;
		}

		return unknown;
	});
}

template <>
void fmt_class_string<ppu_decoder_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_decoder_type type)
	{
		switch (type)
		{
		case ppu_decoder_type::precise: return "Interpreter (precise)";
		case ppu_decoder_type::fast: return "Interpreter (fast)";
		case ppu_decoder_type::llvm: return "Recompiler (LLVM)";
		}

		return unknown;
	});
}

constexpr ppu_decoder<ppu_interpreter_precise> g_ppu_interpreter_precise;
constexpr ppu_decoder<ppu_interpreter_fast> g_ppu_interpreter_fast;

extern void ppu_initialize();
extern void ppu_initialize(const ppu_module& info);
static void ppu_initialize2(class jit_compiler& jit, const ppu_module& module_part, const std::string& cache_path, const std::string& obj_name);
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);
static bool ppu_break(ppu_thread& ppu, ppu_opcode_t op);

// Get pointer to executable cache
template<typename T = u64>
static T& ppu_ref(u32 addr)
{
	return *reinterpret_cast<T*>(vm::g_exec_addr + u64{addr} * 2);
}

// Get interpreter cache value
static u64 ppu_cache(u32 addr)
{
	// Select opcode table
	const auto& table = *(
		g_cfg.core.ppu_decoder == ppu_decoder_type::precise ? &g_ppu_interpreter_precise.get_table() :
		g_cfg.core.ppu_decoder == ppu_decoder_type::fast ? &g_ppu_interpreter_fast.get_table() :
		(fmt::throw_exception("Invalid PPU decoder"), nullptr));

	const u32 value = vm::read32(addr);
	return u64{value} << 32 | ::narrow<u32>(reinterpret_cast<std::uintptr_t>(table[ppu_decode(value)]));
}

static bool ppu_fallback(ppu_thread& ppu, ppu_opcode_t op)
{
	if (g_cfg.core.ppu_debug)
	{
		ppu_log.error("Unregistered instruction: 0x%08x", op.opcode);
	}

	ppu_ref(ppu.cia) = ppu_cache(ppu.cia);

	return false;
}

// TODO: Make this a dispatch call
void ppu_recompiler_fallback(ppu_thread& ppu)
{
	if (g_cfg.core.ppu_debug)
	{
		ppu_log.error("Unregistered PPU Function (LR=0x%llx)", ppu.lr);
	}

	const auto& table = g_ppu_interpreter_fast.get_table();
	const auto cache = vm::g_exec_addr;

	while (true)
	{
		// Run instructions in interpreter
		if (const u32 op = *reinterpret_cast<u32*>(cache + u64{ppu.cia} * 2 + 4);
			table[ppu_decode(op)](ppu, { op })) [[likely]]
		{
			ppu.cia += 4;
			continue;
		}

		if (uptr func = *reinterpret_cast<u32*>(cache + u64{ppu.cia} * 2);
			func != reinterpret_cast<uptr>(ppu_recompiler_fallback))
		{
			// We found a recompiler function at cia, return
			return;
		}

		if (ppu.test_stopped())
		{
			return;
		}
	}
}

static std::unordered_map<u32, u32>* s_ppu_toc;

static bool ppu_check_toc(ppu_thread& ppu, ppu_opcode_t op)
{
	// Compare TOC with expected value
	const auto found = s_ppu_toc->find(ppu.cia);

	if (ppu.gpr[2] != found->second)
	{
		ppu_log.error("Unexpected TOC (0x%x, expected 0x%x)", ppu.gpr[2], found->second);

		if (!ppu.state.test_and_set(cpu_flag::dbg_pause) && ppu.check_state())
		{
			return false;
		}
	}

	// Fallback to the interpreter function
	if (reinterpret_cast<decltype(&ppu_interpreter::UNK)>(ppu_cache(ppu.cia) & 0xffffffff)(ppu, op))
	{
		ppu.cia += 4;
	}

	return false;
}

extern void ppu_register_range(u32 addr, u32 size)
{
	if (!size)
	{
		ppu_log.error("ppu_register_range(0x%x): empty range", addr);
		return;
	}

	// Register executable range at
	utils::memory_commit(&ppu_ref(addr), size * 2, utils::protection::rw);
	vm::page_protect(addr, align(size, 0x10000), 0, vm::page_executable);

	const u32 fallback = ::narrow<u32>(g_cfg.core.ppu_decoder == ppu_decoder_type::llvm ?
	reinterpret_cast<uptr>(ppu_recompiler_fallback) : reinterpret_cast<uptr>(ppu_fallback));

	size &= ~3; // Loop assumes `size = n * 4`, enforce that by rounding down
	while (size)
	{
		ppu_ref(addr) = u64{vm::read32(addr)} << 32 | fallback;
		addr += 4;
		size -= 4;
	}
}

extern void ppu_register_function_at(u32 addr, u32 size, ppu_function_t ptr)
{
	// Initialize specific function
	if (ptr)
	{
		ppu_ref<u32>(addr) = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(ptr));
		return;
	}

	if (!size)
	{
		if (g_cfg.core.ppu_debug)
		{
			ppu_log.error("ppu_register_function_at(0x%x): empty range", addr);
		}

		return;
	}

	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	// Initialize interpreter cache
	const u32 _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(ppu_break));

	while (size)
	{
		if (ppu_ref<u32>(addr) != _break)
		{
			ppu_ref(addr) = ppu_cache(addr);
		}

		addr += 4;
		size -= 4;
	}
}

// Breakpoint entry point
static bool ppu_break(ppu_thread& ppu, ppu_opcode_t op)
{
	// Pause and wait if necessary
	bool status = ppu.state.test_and_set(cpu_flag::dbg_pause);

	g_fxo->get<gdb_server>()->pause_from(&ppu);

	if (!status && ppu.check_state())
	{
		return false;
	}

	// Fallback to the interpreter function
	if (reinterpret_cast<decltype(&ppu_interpreter::UNK)>(ppu_cache(ppu.cia) & 0xffffffff)(ppu, op))
	{
		ppu.cia += 4;
	}

	return false;
}

// Set or remove breakpoint
extern void ppu_breakpoint(u32 addr, bool isAdding)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const auto _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_break));

	if (isAdding)
	{
		// Set breakpoint
		ppu_ref<u32>(addr) = _break;
	}
	else
	{
		// Remove breakpoint
		ppu_ref(addr) = ppu_cache(addr);
	}
}

//sets breakpoint, does nothing if there is a breakpoint there already
extern void ppu_set_breakpoint(u32 addr)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const auto _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_break));

	if (ppu_ref<u32>(addr) != _break)
	{
		ppu_ref<u32>(addr) = _break;
	}
}

//removes breakpoint, does nothing if there is no breakpoint at location
extern void ppu_remove_breakpoint(u32 addr)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		return;
	}

	const auto _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_break));

	if (ppu_ref<u32>(addr) == _break)
	{
		ppu_ref(addr) = ppu_cache(addr);
	}
}

extern bool ppu_patch(u32 addr, u32 value)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm && Emu.GetStatus() != system_state::ready)
	{
		// TODO: support recompilers
		ppu_log.fatal("Patch failed at 0x%x: LLVM recompiler is used.", addr);
		return false;
	}

	if (!vm::try_access(addr, &value, sizeof(value), true))
	{
		ppu_log.fatal("Patch failed at 0x%x: invalid memory address.", addr);
		return false;
	}

	const u32 _break = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_break));
	const u32 fallback = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_fallback));

	if (ppu_ref<u32>(addr) != _break && ppu_ref<u32>(addr) != fallback)
	{
		ppu_ref(addr) = ppu_cache(addr);
	}

	return true;
}

std::string ppu_thread::dump() const
{
	std::string ret = cpu_thread::dump();
	fmt::append(ret, "Priority: %d\n", +prio);
	fmt::append(ret, "Stack: 0x%x..0x%x\n", stack_addr, stack_addr + stack_size - 1);
	fmt::append(ret, "Joiner: %s\n", joiner.load());
	fmt::append(ret, "Commands: %u\n", cmd_queue.size());

	const char* _func = current_function;

	if (_func)
	{
		ret += "Current function: ";
		ret += _func;
		ret += '\n';

		for (u32 i = 3; i <= 6; i++)
			fmt::append(ret, " ** GPR[%d] = 0x%llx\n", i, syscall_args[i - 3]);
	}
	else if (is_paused())
	{
		if (const auto last_func = last_function)
		{
			_func = last_func;
			ret += "Last function: ";
			ret += _func;
			ret += '\n';
		}
	}

	if (const auto _time = start_time)
	{
		fmt::append(ret, "Waiting: %fs\n", (get_guest_system_time() - _time) / 1000000.);
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
	for (uint i = 0; i < 32; ++i) fmt::append(ret, "GPR[%d] = 0x%llx\n", i, gpr[i]);
	for (uint i = 0; i < 32; ++i) fmt::append(ret, "FPR[%d] = %.6G\n", i, fpr[i]);
	for (uint i = 0; i < 32; ++i) fmt::append(ret, "VR[%d] = %s [x: %g y: %g z: %g w: %g]\n", i, vr[i], vr[i]._f[3], vr[i]._f[2], vr[i]._f[1], vr[i]._f[0]);

	fmt::append(ret, "CR = 0x%08x\n", cr.pack());
	fmt::append(ret, "LR = 0x%llx\n", lr);
	fmt::append(ret, "CTR = 0x%llx\n", ctr);
	fmt::append(ret, "VRSAVE = 0x%08x\n", vrsave);
	fmt::append(ret, "XER = [CA=%u | OV=%u | SO=%u | CNT=%u]\n", xer.ca, xer.ov, xer.so, xer.cnt);
	fmt::append(ret, "VSCR = [SAT=%u | NJ=%u]\n", sat, nj);
	fmt::append(ret, "FPSCR = [FL=%u | FG=%u | FE=%u | FU=%u]\n", fpscr.fl, fpscr.fg, fpscr.fe, fpscr.fu);
	fmt::append(ret, "\nCall stack:\n=========\n0x%08x (0x0) called\n", cia);

	//std::shared_lock rlock(vm::g_mutex); // Needs optimizations

	// Determine stack range
	u32 stack_ptr = static_cast<u32>(gpr[1]);

	if (!vm::check_addr(stack_ptr, 1, vm::page_writable))
	{
		// Normally impossible unless the code does not follow ABI rules
		return ret;
	}

	u32 stack_min = stack_ptr & ~0xfff;
	u32 stack_max = stack_min + 4096;

	while (stack_min && vm::check_addr(stack_min - 4096, 4096, vm::page_writable))
	{
		stack_min -= 4096;
	}

	while (stack_max + 4096 && vm::check_addr(stack_max, 4096, vm::page_writable))
	{
		stack_max += 4096;
	}

	for (u64 sp = *vm::get_super_ptr<u64>(stack_ptr); sp >= stack_min && std::max(sp, sp + 0x200) < stack_max; sp = *vm::get_super_ptr<u64>(static_cast<u32>(sp)))
	{
		// TODO: print also function addresses
		fmt::append(ret, "> from 0x%08llx (0x0)\n", *vm::get_super_ptr<u64>(static_cast<u32>(sp + 16)));
	}

	return ret;
}

extern thread_local std::string(*g_tls_log_prefix)();

void ppu_thread::cpu_task()
{
	std::fesetround(FE_TONEAREST);

	if (g_cfg.core.set_daz_and_ftz && g_cfg.core.ppu_decoder != ppu_decoder_type::precise)
	{
		// Set DAZ and FTZ
		_mm_setcsr(_mm_getcsr() | 0x8840);
	}

	// Execute cmd_queue
	while (cmd64 cmd = cmd_wait())
	{
		const u32 arg = cmd.arg2<u32>(); // 32-bit arg extracted

		switch (auto type = cmd.arg1<ppu_cmd>())
		{
		case ppu_cmd::opcode:
		{
			cmd_pop(), g_cfg.core.ppu_decoder == ppu_decoder_type::precise
				? g_ppu_interpreter_precise.decode(arg)(*this, {arg})
				: g_ppu_interpreter_fast.decode(arg)(*this, {arg});
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
			cmd_pop(), ppu_function_manager::get().at(arg)(*this);
			break;
		}
		case ppu_cmd::ptr_call:
		{
			const ppu_function_t func = cmd_get(1).as<ppu_function_t>();
			cmd_pop(1), func(*this);
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
		case ppu_cmd::reset_stack:
		{
			cmd_pop(), gpr[1] = stack_addr + stack_size - 0x70;
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown ppu_cmd(0x%x)" HERE, static_cast<u32>(type));
		}
		}
	}
}

void ppu_thread::cpu_sleep()
{
	vm::temporary_unlock(*this);
	lv2_obj::awake(this);
}

void ppu_thread::cpu_mem()
{
	vm::passive_lock(*this);
}

void ppu_thread::cpu_unmem()
{
	state.test_and_set(cpu_flag::memory);
}

void ppu_thread::exec_task()
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		while (!(state & (cpu_flag::ret + cpu_flag::exit + cpu_flag::stop + cpu_flag::dbg_global_stop)))
		{
			reinterpret_cast<ppu_function_t>(static_cast<std::uintptr_t>(ppu_ref<u32>(cia)))(*this);
		}

		return;
	}

	const auto cache = vm::g_exec_addr;
	using func_t = decltype(&ppu_interpreter::UNK);

	while (true)
	{
		const auto exec_op = [this](u64 op)
		{
			return reinterpret_cast<func_t>(op & 0xffffffff)(*this, {static_cast<u32>(op >> 32)});
		};

		if (cia % 8 || state) [[unlikely]]
		{
			if (test_stopped()) return;

			// Decode single instruction (may be step)
			if (exec_op(*reinterpret_cast<u64*>(cache + u64{cia} * 2))) { cia += 4; }
			continue;
		}

		u64 op0, op1, op2, op3;
		u64 _pos = u64{cia} * 2;

		// Reinitialize
		{
			const v128 _op0 = *reinterpret_cast<const v128*>(cache + _pos);
			const v128 _op1 = *reinterpret_cast<const v128*>(cache + _pos + 16);
			op0 = _op0._u64[0];
			op1 = _op0._u64[1];
			op2 = _op1._u64[0];
			op3 = _op1._u64[1];
		}

		while (exec_op(op0)) [[likely]]
		{
			cia += 4;

			if (exec_op(op1)) [[likely]]
			{
				cia += 4;

				if (exec_op(op2)) [[likely]]
				{
					cia += 4;

					if (exec_op(op3)) [[likely]]
					{
						cia += 4;

						if (state) [[unlikely]]
						{
							break;
						}

						_pos += 32;
						const v128 _op0 = *reinterpret_cast<const v128*>(cache + _pos);
						const v128 _op1 = *reinterpret_cast<const v128*>(cache + _pos + 16);
						op0 = _op0._u64[0];
						op1 = _op0._u64[1];
						op2 = _op1._u64[0];
						op3 = _op1._u64[1];
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
	// Deallocate Stack Area
	vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
}

ppu_thread::ppu_thread(const ppu_thread_params& param, std::string_view name, u32 prio, int detached)
	: cpu_thread(idm::last_id())
	, prio(prio)
	, stack_size(param.stack_size)
	, stack_addr(param.stack_addr)
	, joiner(detached != 0 ? ppu_join_status::detached : ppu_join_status::joinable)
	, start_time(get_guest_system_time())
	, ppu_tname(stx::shared_cptr<std::string>::make(name))
{
	gpr[1] = stack_addr + stack_size - 0x70;

	gpr[13] = param.tls_addr;

	if (detached >= 0 && id != id_base)
	{
		// Initialize thread entry point
		cmd_list
		({
		    {ppu_cmd::set_args, 2}, param.arg0, param.arg1,
		    {ppu_cmd::lle_call, param.entry},
		});
	}
	else
	{
		// Save entry for further use (interrupt handler workaround)
		gpr[2] = param.entry;
	}

	// Trigger the scheduler
	state += cpu_flag::suspend;

	if (!g_use_rtm)
	{
		state += cpu_flag::memory;
	}
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
		if (state) [[unlikely]]
		{
			if (is_stopped())
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
	const auto old_func = current_function;
	const auto old_fmt = g_tls_log_prefix;

	cia = addr;
	gpr[2] = rtoc;
	lr = ppu_function_manager::addr + 8; // HLE stop address
	current_function = nullptr;

	g_tls_log_prefix = []
	{
		const auto _this = static_cast<ppu_thread*>(get_current_cpu_thread());

		static thread_local stx::shared_cptr<std::string> name_cache;

		if (!_this->ppu_tname.is_equal(name_cache)) [[unlikely]]
		{
			name_cache = _this->ppu_tname.load();
		}

		return fmt::format("PPU[0x%x] Thread (%s) [0x%08x]", _this->id, *name_cache.get(), _this->cia);
	};

	auto at_ret = [&]()
	{
		if (std::uncaught_exceptions())
		{
			if (current_function)
			{
				if (start_time)
				{
					ppu_log.warning("'%s' aborted (%fs)", current_function, (get_guest_system_time() - start_time) / 1000000.);
				}
				else
				{
					ppu_log.warning("'%s' aborted", current_function);
				}
			}

			current_function = old_func;
		}
		else
		{
			state -= cpu_flag::ret;
			cia = old_cia;
			gpr[2] = old_rtoc;
			lr = old_lr;
			current_function = old_func;
			g_tls_log_prefix = old_fmt;
		}
	};

	exec_task();
	at_ret();
}

u32 ppu_thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id_type() == 1)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		const u32 old_pos = vm::cast(context.gpr[1], HERE);
		context.gpr[1] -= align(size + 4, 8); // room minimal possible size
		context.gpr[1] &= ~(u64{align_v} - 1); // fix stack alignment

		if (old_pos >= context.stack_addr && old_pos < context.stack_addr + context.stack_size && context.gpr[1] < context.stack_addr)
		{
			fmt::throw_exception("Stack overflow (size=0x%x, align=0x%x, SP=0x%llx, stack=*0x%x)" HERE, size, align_v, old_pos, context.stack_addr);
		}
		else
		{
			const u32 addr = static_cast<u32>(context.gpr[1]);
			vm::_ref<nse_t<u32>>(addr + size) = old_pos;
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
			ppu_log.error("Stack inconsistency (addr=0x%x, SP=0x%llx, size=0x%x)", addr, context.gpr[1], size);
			return;
		}

		context.gpr[1] = vm::_ref<nse_t<u32>>(static_cast<u32>(context.gpr[1]) + size);
		return;
	}

	ppu_log.error("Invalid thread" HERE);
}

extern u64 get_timebased_time();
extern ppu_function_t ppu_get_syscall(u64 code);

extern __m128 sse_exp2_ps(__m128 A);
extern __m128 sse_log2_ps(__m128 A);
extern __m128i sse_altivec_vperm(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_vperm_v0(__m128i A, __m128i B, __m128i C);
extern __m128i sse_altivec_lvsl(u64 addr);
extern __m128i sse_altivec_lvsr(u64 addr);
extern __m128i sse_cellbe_lvlx(u64 addr);
extern __m128i sse_cellbe_lvrx(u64 addr);
extern void sse_cellbe_stvlx(u64 addr, __m128i a);
extern void sse_cellbe_stvrx(u64 addr, __m128i a);
extern __m128i sse_cellbe_lvlx_v0(u64 addr);
extern __m128i sse_cellbe_lvrx_v0(u64 addr);
extern void sse_cellbe_stvlx_v0(u64 addr, __m128i a);
extern void sse_cellbe_stvrx_v0(u64 addr, __m128i a);

[[noreturn]] static void ppu_trap(ppu_thread& ppu, u64 addr)
{
	ppu.cia = ::narrow<u32>(addr);
	fmt::throw_exception("Trap! (0x%llx)", addr);
}

[[noreturn]] static void ppu_error(ppu_thread& ppu, u64 addr, u32 op)
{
	ppu.cia = ::narrow<u32>(addr);
	fmt::throw_exception("Unknown/Illegal opcode 0x08x (0x%llx)", op, addr);
}

static void ppu_check(ppu_thread& ppu, u64 addr)
{
	ppu.cia = ::narrow<u32>(addr);

	if (ppu.test_stopped())
	{
		return;
	}
}

static void ppu_trace(u64 addr)
{
	ppu_log.notice("Trace: 0x%llx", addr);
}

template <typename T>
static T ppu_load_acquire_reservation(ppu_thread& ppu, u32 addr)
{
	// Always load aligned 64-bit value (unaligned reservation will fail to store)
	auto& data = vm::_ref<const atomic_be_t<u64>>(addr & -8);
	const u64 size_off = (sizeof(T) * 8) & 63;
	const u64 data_off = (addr & 7) * 8;

	ppu.raddr = addr;

	u64 count = 0;

	while (g_use_rtm) [[likely]]
	{
		ppu.rtime = vm::reservation_acquire(addr, sizeof(T)) & -128;
		ppu.rdata = data;

		if ((vm::reservation_acquire(addr, sizeof(T)) & -128) == ppu.rtime) [[likely]]
		{
			if (count >= 10) [[unlikely]]
			{
				ppu_log.error("%s took too long: %u", sizeof(T) == 4 ? "LWARX" : "LDARX", count);
			}

			return static_cast<T>(ppu.rdata << data_off >> size_off);
		}
		else
		{
			_mm_pause();
			count++;
		}
	}

	ppu.rtime = vm::reservation_acquire(addr, sizeof(T));

	if ((ppu.rtime & 127) == 0) [[likely]]
	{
		ppu.rdata = data;

		if (vm::reservation_acquire(addr, sizeof(T)) == ppu.rtime) [[likely]]
		{
			return static_cast<T>(ppu.rdata << data_off >> size_off);
		}
	}

	vm::passive_unlock(ppu);

	for (u64 i = 0;; i++)
	{
		ppu.rtime = vm::reservation_acquire(addr, sizeof(T));

		if ((ppu.rtime & 127) == 0) [[likely]]
		{
			ppu.rdata = data;

			if (vm::reservation_acquire(addr, sizeof(T)) == ppu.rtime) [[likely]]
			{
				break;
			}
		}

		if (i < 20)
		{
			busy_wait(300);
		}
		else
		{
			std::this_thread::yield();
		}
	}

	vm::passive_lock(ppu);
	return static_cast<T>(ppu.rdata << data_off >> size_off);
}

extern u32 ppu_lwarx(ppu_thread& ppu, u32 addr)
{
	return ppu_load_acquire_reservation<u32>(ppu, addr);
}

extern u64 ppu_ldarx(ppu_thread& ppu, u32 addr)
{
	return ppu_load_acquire_reservation<u64>(ppu, addr);
}

const auto ppu_stwcx_tx = build_function_asm<u32(*)(u32 raddr, u64 rtime, u64 rdata, u32 value)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();

	// Prepare registers
	c.mov(x86::rax, imm_ptr(&vm::g_reservations));
	c.mov(x86::r10, x86::qword_ptr(x86::rax));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::rax));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.shr(args[0], 7);
	c.lea(x86::r10, x86::qword_ptr(x86::r10, args[0], 3));
	c.xor_(args[0].r32(), args[0].r32());
	c.bswap(args[2].r32());
	c.bswap(args[3].r32());

	// Begin transaction
	build_transaction_enter(c, fall, args[0], 16);
	c.mov(x86::rax, x86::qword_ptr(x86::r10));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, args[1]);
	c.jne(fail);
	c.cmp(x86::dword_ptr(x86::r11), args[2].r32());
	c.jne(fail);
	c.mov(x86::dword_ptr(x86::r11), args[3].r32());
	c.sub(x86::qword_ptr(x86::r10), -128);
	c.xend();
	c.mov(x86::eax, 1);
	c.ret();

	// Return 2 after transaction failure
	c.bind(fall);
	c.sar(x86::eax, 24);
	c.js(fail);
	c.mov(x86::eax, 2);
	c.ret();

	c.bind(fail);
	build_transaction_abort(c, 0xff);
	c.xor_(x86::eax, x86::eax);
	c.ret();
});

extern bool ppu_stwcx(ppu_thread& ppu, u32 addr, u32 reg_value)
{
	auto& data = vm::_ref<atomic_be_t<u32>>(addr & -4);
	const u32 old_data = static_cast<u32>(ppu.rdata << ((addr & 7) * 8) >> 32);

	if (ppu.raddr != addr || addr & 3 || old_data != data.load() || ppu.rtime != (vm::reservation_acquire(addr, sizeof(u32)) & -128))
	{
		ppu.raddr = 0;
		return false;
	}

	if (g_use_rtm) [[likely]]
	{
		switch (ppu_stwcx_tx(addr, ppu.rtime, old_data, reg_value))
		{
		case 0:
		{
			// Reservation lost
			ppu.raddr = 0;
			return false;
		}
		case 1:
		{
			vm::reservation_notifier(addr, sizeof(u32)).notify_all();
			ppu.raddr = 0;
			return true;
		}
		}

		auto& res = vm::reservation_acquire(addr, sizeof(u32));

		ppu.raddr = 0;

		if (res == ppu.rtime && res.compare_and_swap_test(ppu.rtime, ppu.rtime | 1))
		{
			if (data.compare_and_swap_test(old_data, reg_value))
			{
				res += 127;
				vm::reservation_notifier(addr, sizeof(u32)).notify_all();
				return true;
			}

			res -= 1;
		}

		return false;
	}

	vm::passive_unlock(ppu);

	auto& res = vm::reservation_lock(addr, sizeof(u32));
	const u64 old_time = res.load() & -128;

	const bool result = ppu.rtime == old_time && data.compare_and_swap_test(old_data, reg_value);

	if (result)
	{
		res.release(old_time + 128);
		vm::reservation_notifier(addr, sizeof(u32)).notify_all();
	}
	else
	{
		res.release(old_time);
	}

	vm::passive_lock(ppu);
	ppu.raddr = 0;
	return result;
}

const auto ppu_stdcx_tx = build_function_asm<u32(*)(u32 raddr, u64 rtime, u64 rdata, u64 value)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();

	// Prepare registers
	c.mov(x86::rax, imm_ptr(&vm::g_reservations));
	c.mov(x86::r10, x86::qword_ptr(x86::rax));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::rax));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.shr(args[0], 7);
	c.lea(x86::r10, x86::qword_ptr(x86::r10, args[0], 3));
	c.xor_(args[0].r32(), args[0].r32());
	c.bswap(args[2]);
	c.bswap(args[3]);

	// Begin transaction
	build_transaction_enter(c, fall, args[0], 16);
	c.mov(x86::rax, x86::qword_ptr(x86::r10));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, args[1]);
	c.jne(fail);
	c.cmp(x86::qword_ptr(x86::r11), args[2]);
	c.jne(fail);
	c.mov(x86::qword_ptr(x86::r11), args[3]);
	c.sub(x86::qword_ptr(x86::r10), -128);
	c.xend();
	c.mov(x86::eax, 1);
	c.ret();

	// Return 2 after transaction failure
	c.bind(fall);
	c.sar(x86::eax, 24);
	c.js(fail);
	c.mov(x86::eax, 2);
	c.ret();

	c.bind(fail);
	build_transaction_abort(c, 0xff);
	c.xor_(x86::eax, x86::eax);
	c.ret();
});

extern bool ppu_stdcx(ppu_thread& ppu, u32 addr, u64 reg_value)
{
	auto& data = vm::_ref<atomic_be_t<u64>>(addr & -8);
	const u64 old_data = ppu.rdata << ((addr & 7) * 8);

	if (ppu.raddr != addr || addr & 7 || old_data != data.load() || ppu.rtime != (vm::reservation_acquire(addr, sizeof(u64)) & -128))
	{
		ppu.raddr = 0;
		return false;
	}

	if (g_use_rtm) [[likely]]
	{
		switch (ppu_stdcx_tx(addr, ppu.rtime, old_data, reg_value))
		{
		case 0:
		{
			// Reservation lost
			ppu.raddr = 0;
			return false;
		}
		case 1:
		{
			vm::reservation_notifier(addr, sizeof(u64)).notify_all();
			ppu.raddr = 0;
			return true;
		}
		}

		auto& res = vm::reservation_acquire(addr, sizeof(u64));

		ppu.raddr = 0;

		if (res == ppu.rtime && res.compare_and_swap_test(ppu.rtime, ppu.rtime | 1))
		{
			if (data.compare_and_swap_test(old_data, reg_value))
			{
				res += 127;
				vm::reservation_notifier(addr, sizeof(u64)).notify_all();
				return true;
			}

			res -= 1;
		}

		return false;
	}

	vm::passive_unlock(ppu);

	auto& res = vm::reservation_lock(addr, sizeof(u64));
	const u64 old_time = res.load() & -128;

	const bool result = ppu.rtime == old_time && data.compare_and_swap_test(old_data, reg_value);

	if (result)
	{
		res.release(old_time + 128);
		vm::reservation_notifier(addr, sizeof(u64)).notify_all();
	}
	else
	{
		res.release(old_time);
	}

	vm::passive_lock(ppu);
	ppu.raddr = 0;
	return result;
}

extern void ppu_initialize()
{
	const auto _main = g_fxo->get<ppu_module>();

	if (!_main)
	{
		return;
	}

	if (Emu.IsStopped())
	{
		return;
	}

	// Initialize main module
	if (!_main->segs.empty())
	{
		ppu_initialize(*_main);
	}

	std::vector<lv2_prx*> prx_list;

	idm::select<lv2_obj, lv2_prx>([&](u32, lv2_prx& prx)
	{
		prx_list.emplace_back(&prx);
	});

	// Initialize preloaded libraries
	for (auto ptr : prx_list)
	{
		ppu_initialize(*ptr);
	}

	// Initialize SPU cache
	spu_cache::initialize();
}

extern void ppu_initialize(const ppu_module& info)
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::llvm)
	{
		// Temporarily
		s_ppu_toc = g_fxo->get<std::unordered_map<u32, u32>>();

		for (const auto& func : info.funcs)
		{
			for (auto& block : func.blocks)
			{
				ppu_register_function_at(block.first, block.second, nullptr);
			}

			if (g_cfg.core.ppu_debug && func.size && func.toc != umax)
			{
				s_ppu_toc->emplace(func.addr, func.toc);
				ppu_ref<u32>(func.addr) = ::narrow<u32>(reinterpret_cast<std::uintptr_t>(&ppu_check_toc));
			}
		}

		return;
	}

	// Link table
	static const std::unordered_map<std::string, u64> s_link_table = []()
	{
		std::unordered_map<std::string, u64> link_table
		{
			{ "__mptr", reinterpret_cast<u64>(&vm::g_base_addr) },
			{ "__cptr", reinterpret_cast<u64>(&vm::g_exec_addr) },
			{ "__trap", reinterpret_cast<u64>(&ppu_trap) },
			{ "__error", reinterpret_cast<u64>(&ppu_error) },
			{ "__check", reinterpret_cast<u64>(&ppu_check) },
			{ "__trace", reinterpret_cast<u64>(&ppu_trace) },
			{ "__syscall", reinterpret_cast<u64>(ppu_execute_syscall) },
			{ "__get_tb", reinterpret_cast<u64>(get_timebased_time) },
			{ "__lwarx", reinterpret_cast<u64>(ppu_lwarx) },
			{ "__ldarx", reinterpret_cast<u64>(ppu_ldarx) },
			{ "__stwcx", reinterpret_cast<u64>(ppu_stwcx) },
			{ "__stdcx", reinterpret_cast<u64>(ppu_stdcx) },
			{ "__vexptefp", reinterpret_cast<u64>(sse_exp2_ps) },
			{ "__vlogefp", reinterpret_cast<u64>(sse_log2_ps) },
			{ "__vperm", s_use_ssse3 ? reinterpret_cast<u64>(sse_altivec_vperm) : reinterpret_cast<u64>(sse_altivec_vperm_v0) }, // Obsolete
			{ "__lvsl", reinterpret_cast<u64>(sse_altivec_lvsl) },
			{ "__lvsr", reinterpret_cast<u64>(sse_altivec_lvsr) },
			{ "__lvlx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_lvlx) : reinterpret_cast<u64>(sse_cellbe_lvlx_v0) },
			{ "__lvrx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_lvrx) : reinterpret_cast<u64>(sse_cellbe_lvrx_v0) },
			{ "__stvlx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_stvlx) : reinterpret_cast<u64>(sse_cellbe_stvlx_v0) },
			{ "__stvrx", s_use_ssse3 ? reinterpret_cast<u64>(sse_cellbe_stvrx) : reinterpret_cast<u64>(sse_cellbe_stvrx_v0) },
			{ "__resupdate", reinterpret_cast<u64>(vm::reservation_update) },
			{ "sys_config_io_event", reinterpret_cast<u64>(ppu_get_syscall(523)) },
		};

		for (u64 index = 0; index < 1024; index++)
		{
			if (auto sc = ppu_get_syscall(index))
			{
				link_table.emplace(fmt::format("%s", ppu_syscall_code(index)), reinterpret_cast<u64>(ppu_execute_syscall));
				link_table.emplace(fmt::format("syscall_%u", index), reinterpret_cast<u64>(ppu_execute_syscall));
			}
		}

		return link_table;
	}();

	// Get cache path for this executable
	std::string cache_path;

	if (info.name.empty())
	{
		cache_path = info.cache;
	}
	else
	{
		// New PPU cache location
		cache_path = fs::get_cache_dir() + "cache/";

		const std::string dev_flash = vfs::get("/dev_flash/");

		if (!info.path.starts_with(dev_flash) && !Emu.GetTitleID().empty() && Emu.GetCat() != "1P")
		{
			// Add prefix for anything except dev_flash files, standalone elfs or PS1 classics
			cache_path += Emu.GetTitleID();
			cache_path += '/';
		}

		// Add PPU hash and filename
		fmt::append(cache_path, "ppu-%s-%s/", fmt::base57(info.sha1), info.path.substr(info.path.find_last_of('/') + 1));

		if (!fs::create_path(cache_path))
		{
			fmt::throw_exception("Failed to create cache directory: %s (%s)", cache_path, fs::g_tls_error);
		}
	}

#ifdef LLVM_AVAILABLE
	// Initialize progress dialog
	g_progr = "Compiling PPU modules...";

	// Compiled PPU module info
	struct jit_module
	{
		std::vector<u64*> vars;
		std::vector<ppu_function_t> funcs;
	};

	struct jit_core_allocator
	{
		const s32 thread_count = g_cfg.core.llvm_threads ? std::min<s32>(g_cfg.core.llvm_threads, limit()) : limit();

		// Initialize global semaphore with the max number of threads
		::semaphore<0x7fffffff> sem{std::max<s32>(thread_count, 1)};

		static s32 limit()
		{
			return static_cast<s32>(std::thread::hardware_concurrency());
		}
	};

	// Permanently loaded compiled PPU modules (name -> data)
	jit_module& jit_mod = g_fxo->get<std::unordered_map<std::string, jit_module>>()->emplace(cache_path + info.name, jit_module{}).first->second;

	// Compiler instance (deferred initialization)
	std::shared_ptr<jit_compiler> jit;

	// Compiler mutex (global)
	static shared_mutex jmutex;

	// Global variables to initialize
	std::vector<std::pair<std::string, u64>> globals;

	// Split module into fragments <= 1 MiB
	std::size_t fpos = 0;

	// Difference between function name and current location
	const u32 reloc = info.name.empty() ? 0 : info.segs.at(0).addr;

	// Info sent to threads
	std::vector<std::pair<std::string, ppu_module>> workload;

	// Info to load to main JIT instance (true - compiled)
	std::vector<std::pair<std::string, bool>> link_workload;

	// Sync variable to acquire workloads
	atomic_t<u32> work_cv = 0;

	while (jit_mod.vars.empty() && fpos < info.funcs.size())
	{
		// Initialize compiler instance
		if (!jit && get_current_cpu_thread())
		{
			jit = std::make_shared<jit_compiler>(s_link_table, g_cfg.core.llvm_cpu);
		}

		// First function in current module part
		const auto fstart = fpos;

		// Copy module information (TODO: optimize)
		ppu_module part;
		part.copy_part(info);
		part.funcs.reserve(16000);

		// Unique suffix for each module part
		const u32 suffix = info.funcs.at(fstart).addr - reloc;

		// Overall block size in bytes
		std::size_t bsize = 0;

		while (fpos < info.funcs.size())
		{
			auto& func = info.funcs[fpos];

			if (bsize + func.size > 100 * 1024 && bsize)
			{
				break;
			}

			for (auto&& block : func.blocks)
			{
				bsize += block.second;

				// Also split functions blocks into functions (TODO)
				ppu_function entry;
				entry.addr = block.first;
				entry.size = block.second;
				entry.toc  = func.toc;
				fmt::append(entry.name, "__0x%x", block.first - reloc);
				part.funcs.emplace_back(std::move(entry));
			}

			fpos++;
		}

		// Compute module hash to generate (hopefully) unique object name
		std::string obj_name;
		{
			sha1_context ctx;
			u8 output[20];
			sha1_starts(&ctx);

			for (const auto& func : part.funcs)
			{
				if (func.size == 0)
				{
					continue;
				}

				const be_t<u32> addr = func.addr - reloc;
				const be_t<u32> size = func.size;
				sha1_update(&ctx, reinterpret_cast<const u8*>(&addr), sizeof(addr));
				sha1_update(&ctx, reinterpret_cast<const u8*>(&size), sizeof(size));

				for (const auto& block : func.blocks)
				{
					if (block.second == 0 || reloc)
					{
						continue;
					}

					// Find relevant relocations
					auto low = std::lower_bound(part.relocs.cbegin(), part.relocs.cend(), block.first);
					auto high = std::lower_bound(low, part.relocs.cend(), block.first + block.second);
					auto addr = block.first;

					for (; low != high; ++low)
					{
						// Aligned relocation address
						const u32 roff = low->addr & ~3;

						if (roff > addr)
						{
							// Hash from addr to the beginning of the relocation
							sha1_update(&ctx, vm::_ptr<const u8>(addr), roff - addr);
						}

						// Hash relocation type instead
						const be_t<u32> type = low->type;
						sha1_update(&ctx, reinterpret_cast<const u8*>(&type), sizeof(type));

						// Set the next addr
						addr = roff + 4;
					}

					// Hash from addr to the end of the block
					sha1_update(&ctx, vm::_ptr<const u8>(addr), block.second - (addr - block.first));
				}

				if (reloc)
				{
					continue;
				}

				sha1_update(&ctx, vm::_ptr<const u8>(func.addr), func.size);
			}

			if (false)
			{
				const be_t<u64> forced_upd = 3;
				sha1_update(&ctx, reinterpret_cast<const u8*>(&forced_upd), sizeof(forced_upd));
			}

			sha1_finish(&ctx, output);

			// Settings: should be populated by settings which affect codegen (TODO)
			enum class ppu_settings : u32
			{
				non_win32,

				__bitset_enum_max
			};

			be_t<bs_t<ppu_settings>> settings{};

#ifndef _WIN32
			settings += ppu_settings::non_win32;
#endif

			// Write version, hash, CPU, settings
			fmt::append(obj_name, "v3-tane-%s-%s-%s.obj", fmt::base57(output, 16), fmt::base57(settings), jit_compiler::cpu(g_cfg.core.llvm_cpu));
		}

		if (Emu.IsStopped())
		{
			break;
		}

		globals.emplace_back(fmt::format("__mptr%x", suffix), reinterpret_cast<u64>(vm::g_base_addr));
		globals.emplace_back(fmt::format("__cptr%x", suffix), reinterpret_cast<u64>(vm::g_exec_addr));

		// Initialize segments for relocations
		for (u32 i = 0; i < info.segs.size(); i++)
		{
			globals.emplace_back(fmt::format("__seg%u_%x", i, suffix), info.segs[i].addr);
		}

		link_workload.emplace_back(obj_name, false);

		// Check object file
		if (fs::is_file(cache_path + obj_name + ".gz") || fs::is_file(cache_path + obj_name))
		{
			if (!jit)
			{
				ppu_log.success("LLVM: Module exists: %s", obj_name);
				continue;
			}

			continue;
		}

		// Adjust information (is_compiled)
		link_workload.back().second = true;

		// Fill workload list for compilation
		workload.emplace_back(std::move(obj_name), std::move(part));

		// Update progress dialog
		g_progr_ptotal++;
	}

	// Create worker threads for compilation (TODO: how many threads)
	{
		u32 thread_count = Emu.GetMaxThreads();

		if (workload.size() < thread_count)
		{
			thread_count = ::size32(workload);
		}

		struct thread_index_allocator
		{
			atomic_t<u64> index = 0;
		};

		named_thread_group threads(fmt::format("PPUW.%u.", ++g_fxo->get<thread_index_allocator>()->index), thread_count, [&]()
		{
			// Set low priority
			thread_ctrl::set_native_priority(-1);

			for (u32 i = work_cv++; i < workload.size(); i = work_cv++)
			{
				// Keep allocating workload
				const auto [obj_name, part] = std::as_const(workload)[i];

				// Allocate "core"
				std::lock_guard jlock(g_fxo->get<jit_core_allocator>()->sem);

				if (!Emu.IsStopped())
				{
					ppu_log.warning("LLVM: Compiling module %s%s", cache_path, obj_name);

					// Use another JIT instance
					jit_compiler jit2({}, g_cfg.core.llvm_cpu, 0x1);
					ppu_initialize2(jit2, part, cache_path, obj_name);

					ppu_log.success("LLVM: Compiled module %s", obj_name);
				}

				g_progr_pdone++;
			}
		});

		threads.join();

		if (Emu.IsStopped() || !get_current_cpu_thread())
		{
			return;
		}

		std::lock_guard lock(jmutex);

		for (auto [obj_name, is_compiled] : link_workload)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			jit->add(cache_path + obj_name);

			if (!is_compiled)
			{
				ppu_log.success("LLVM: Loaded module %s", obj_name);
			}
		}
	}

	if (Emu.IsStopped() || !get_current_cpu_thread())
	{
		return;
	}

	// Jit can be null if the loop doesn't ever enter.
	if (jit && jit_mod.vars.empty())
	{
		std::lock_guard lock(jmutex);
		jit->fin();

		// Get and install function addresses
		for (const auto& func : info.funcs)
		{
			if (!func.size) continue;

			for (const auto& block : func.blocks)
			{
				if (block.second)
				{
					const u64 addr = jit->get(fmt::format("__0x%x", block.first - reloc));
					jit_mod.funcs.emplace_back(reinterpret_cast<ppu_function_t>(addr));
					ppu_ref<u32>(block.first) = ::narrow<u32>(addr);
				}
			}
		}

		// Initialize global variables
		for (auto& var : globals)
		{
			const u64 addr = jit->get(var.first);

			jit_mod.vars.emplace_back(reinterpret_cast<u64*>(addr));

			if (addr)
			{
				*reinterpret_cast<u64*>(addr) = var.second;
			}
		}
	}
	else
	{
		std::size_t index = 0;

		// Locate existing functions
		for (const auto& func : info.funcs)
		{
			if (!func.size) continue;

			for (const auto& block : func.blocks)
			{
				if (block.second)
				{
					ppu_ref<u32>(block.first) = ::narrow<u32>(reinterpret_cast<uptr>(jit_mod.funcs[index++]));
				}
			}
		}

		index = 0;

		// Rewrite global variables
		while (index < jit_mod.vars.size())
		{
			*jit_mod.vars[index++] = reinterpret_cast<u64>(vm::g_base_addr);
			*jit_mod.vars[index++] = reinterpret_cast<u64>(vm::g_exec_addr);

			for (const auto& seg : info.segs)
			{
				*jit_mod.vars[index++] = seg.addr;
			}
		}
	}
#else
	fmt::throw_exception("LLVM is not available in this build.");
#endif
}

static void ppu_initialize2(jit_compiler& jit, const ppu_module& module_part, const std::string& cache_path, const std::string& obj_name)
{
#ifdef LLVM_AVAILABLE
	using namespace llvm;

	// Create LLVM module
	std::unique_ptr<Module> module = std::make_unique<Module>(obj_name, jit.get_context());

	// Initialize target
	module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	module->setDataLayout(jit.get_engine().getTargetMachine()->createDataLayout());

	// Initialize translator
	PPUTranslator translator(jit.get_context(), module.get(), module_part, jit.get_engine());

	// Define some types
	const auto _void = Type::getVoidTy(jit.get_context());
	const auto _func = FunctionType::get(_void, {translator.GetContextType()->getPointerTo()}, false);

	// Initialize function list
	for (const auto& func : module_part.funcs)
	{
		if (func.size)
		{
			const auto f = cast<Function>(module->getOrInsertFunction(func.name, _func).getCallee());
			f->addAttribute(1, Attribute::NoAlias);
		}
	}

	{
		legacy::FunctionPassManager pm(module.get());

		// Basic optimizations
		//pm.add(createCFGSimplificationPass());
		//pm.add(createPromoteMemoryToRegisterPass());
		pm.add(createEarlyCSEPass());
		//pm.add(createTailCallEliminationPass());
		//pm.add(createInstructionCombiningPass());
		//pm.add(createBasicAAWrapperPass());
		//pm.add(new MemoryDependenceAnalysis());
		//pm.add(createLICMPass());
		//pm.add(createLoopInstSimplifyPass());
		//pm.add(createNewGVNPass());
		pm.add(createDeadStoreEliminationPass());
		//pm.add(createSCCPPass());
		//pm.add(createReassociatePass());
		//pm.add(createInstructionCombiningPass());
		//pm.add(createInstructionSimplifierPass());
		//pm.add(createAggressiveDCEPass());
		//pm.add(createCFGSimplificationPass());
		//pm.add(createLintPass()); // Check

		// Translate functions
		for (size_t fi = 0, fmax = module_part.funcs.size(); fi < fmax; fi++)
		{
			if (Emu.IsStopped())
			{
				ppu_log.success("LLVM: Translation cancelled");
				return;
			}

			if (module_part.funcs[fi].size)
			{
				// Translate
				if (const auto func = translator.Translate(module_part.funcs[fi]))
				{
					// Run optimization passes
					pm.run(*func);
				}
				else
				{
					Emu.Pause();
					return;
				}
			}
		}

		legacy::PassManager mpm;

		// Remove unused functions, structs, global variables, etc
		//mpm.add(createStripDeadPrototypesPass());
		//mpm.add(createFunctionInliningPass());
		//mpm.add(createDeadInstEliminationPass());
		//mpm.run(*module);

		std::string result;
		raw_string_ostream out(result);

		if (g_cfg.core.llvm_logs)
		{
			out << *module; // print IR
			fs::file(cache_path + obj_name + ".log", fs::rewrite).write(out.str());
			result.clear();
		}

		if (verifyModule(*module, &out))
		{
			out.flush();
			ppu_log.error("LLVM: Verification failed for %s:\n%s", obj_name, result);
			Emu.CallAfter([]{ Emu.Stop(); });
			return;
		}

		ppu_log.notice("LLVM: %zu functions generated", module->getFunctionList().size());
	}

	// Load or compile module
	jit.add(std::move(module), cache_path);
#endif // LLVM_AVAILABLE
}
