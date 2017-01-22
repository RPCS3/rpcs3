#include "stdafx.h"
#include "Utilities/Config.h"
#include "Utilities/VirtualMemory.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
#include "PPUAnalyser.h"
#include "PPUModule.h"

#ifdef LLVM_AVAILABLE
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

#include "Utilities/JIT.h"
#include "PPUTranslator.h"
#include "Modules/cellMsgDialog.h"
#endif

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

static void ppu_initialize();
extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);
extern void ppu_execute_function(ppu_thread& ppu, u32 index);

const auto s_ppu_compiled = static_cast<u32*>(memory_helper::reserve_memory(0x100000000));

extern void ppu_register_function_at(u32 addr, ppu_function_t ptr)
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		memory_helper::commit_page_memory(s_ppu_compiled + addr / 4, sizeof(s_ppu_compiled[0]));
		s_ppu_compiled[addr / 4] = (u32)(std::uintptr_t)ptr;
	}
}

std::string ppu_thread::get_name() const
{
	return fmt::format("PPU[0x%x] Thread (%s)", id, m_name);
}

std::string ppu_thread::dump() const
{
	std::string ret = cpu_thread::dump();
	ret += fmt::format("Priority: %d\n", prio);
	
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
	//SetHostRoundingMode(FPSCR_RN_NEAR);

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

	// Select opcode table
	const auto& table = *(
		g_cfg_ppu_decoder.get() == ppu_decoder_type::precise ? &s_ppu_interpreter_precise.get_table() :
		g_cfg_ppu_decoder.get() == ppu_decoder_type::fast ? &s_ppu_interpreter_fast.get_table() :
		(fmt::throw_exception<std::logic_error>("Invalid PPU decoder"), nullptr));

	v128 _op;
	decltype(&ppu_interpreter::UNK) func0, func1, func2, func3;

	while (true)
	{
		if (UNLIKELY(test(state)))
		{
			if (check_state()) return;
		}

		// Reinitialize
		{
			const auto _ops = _mm_shuffle_epi8(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(base + cia)), _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3));
			_op.vi = _ops;
			const v128 _i = v128::fromV(_mm_and_si128(_mm_or_si128(_mm_slli_epi32(_op.vi, 6), _mm_srli_epi32(_op.vi, 26)), _mm_set1_epi32(0x1ffff)));
			func0 = table[_i._u32[0]];
			func1 = table[_i._u32[1]];
			func2 = table[_i._u32[2]];
			func3 = table[_i._u32[3]];
		}

		while (LIKELY(func0(*this, { _op._u32[0] })))
		{
			if (cia += 4, LIKELY(func1(*this, { _op._u32[1] })))
			{
				if (cia += 4, LIKELY(func2(*this, { _op._u32[2] })))
				{
					cia += 4;
					func0 = func3;

					const auto _ops = _mm_shuffle_epi8(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(base + cia + 4)), _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3));
					_op.vi = _mm_alignr_epi8(_ops, _op.vi, 12);
					const v128 _i = v128::fromV(_mm_and_si128(_mm_or_si128(_mm_slli_epi32(_op.vi, 6), _mm_srli_epi32(_op.vi, 26)), _mm_set1_epi32(0x1ffff)));
					func1 = table[_i._u32[1]];
					func2 = table[_i._u32[2]];
					func3 = table[_i._u32[3]];

					if (UNLIKELY(test(state)))
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

ppu_thread::~ppu_thread()
{
	if (stack_addr)
	{
		vm::dealloc_verbose_nothrow(stack_addr, vm::stack);
	}
}

ppu_thread::ppu_thread(const std::string& name, u32 prio, u32 stack)
	: cpu_thread()
	, prio(prio)
	, stack_size(std::max<u32>(stack, 0x4000))
	, stack_addr(vm::alloc(stack_size, vm::stack))
	, m_name(name)
{
	if (!stack_addr)
	{
		fmt::throw_exception("Out of stack memory (size=0x%x)" HERE, stack_size);
	}

	gpr[1] = ::align(stack_addr + stack_size, 0x200) - 0x200;
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
	std::unique_lock<named_thread> lock(*this, std::defer_lock);

	while (true)
	{
		if (UNLIKELY(test(state)))
		{
			if (lock) lock.unlock();

			if (check_state()) // check_status() requires unlocked mutex
			{
				return cmd64{};
			}
		}

		// Lightweight queue doesn't care about mutex state
		if (cmd64 result = cmd_queue[cmd_queue.peek()].exchange(cmd64{}))
		{
			return result;
		}

		if (!lock)
		{
			lock.lock();
			continue;
		}

		thread_ctrl::wait(); // Waiting requires locked mutex
	}
}

be_t<u64>* ppu_thread::get_stack_arg(s32 i, u64 align)
{
	if (align != 1 && align != 2 && align != 4 && align != 8 && align != 16) fmt::throw_exception("Unsupported alignment: 0x%llx" HERE, align);
	return vm::_ptr<u64>(vm::cast((gpr[1] + 0x30 + 0x8 * (i - 1)) & (0 - align), HERE));
}

void ppu_thread::fast_call(u32 addr, u32 rtoc)
{
	const auto old_pc = cia;
	const auto old_stack = gpr[1];
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
		const auto ppu = static_cast<ppu_thread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%08x]", ppu->get_name(), ppu->cia);
	};

	try
	{
		exec_task();

		if (gpr[1] != old_stack && !test(state, cpu_flag::ret + cpu_flag::exit)) // gpr[1] shouldn't change
		{
			fmt::throw_exception("Stack inconsistency (addr=0x%x, rtoc=0x%x, SP=0x%llx, old=0x%llx)", addr, rtoc, gpr[1], old_stack);
		}
	}
	catch (cpu_flag _s)
	{
		state += _s;
		if (_s != cpu_flag::ret) throw;
	}
	catch (EmulationStopped)
	{
		if (last_function) LOG_WARNING(PPU, "'%s' aborted", last_function);
		last_function = old_func;
		throw;
	}
	catch (...)
	{
		if (last_function) LOG_ERROR(PPU, "'%s' aborted", last_function);
		last_function = old_func;
		throw;
	}

	state -= cpu_flag::ret;

	cia = old_pc;
	gpr[1] = old_stack;
	gpr[2] = old_rtoc;
	lr = old_lr;
	last_function = old_func;
	g_tls_log_prefix = old_fmt;
}

u32 ppu_thread::stack_push(u32 size, u32 align_v)
{
	if (auto cpu = get_current_cpu_thread()) if (cpu->id >= id_min)
	{
		ppu_thread& context = static_cast<ppu_thread&>(*cpu);

		const u32 old_pos = vm::cast(context.gpr[1], HERE);
		context.gpr[1] -= align(size + 4, 8); // room minimal possible size
		context.gpr[1] &= ~(align_v - 1); // fix stack alignment

		if (context.gpr[1] < context.stack_addr)
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
	if (auto cpu = get_current_cpu_thread()) if (cpu->id >= id_min)
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

static void ppu_trace(u64 addr)
{
	LOG_NOTICE(PPU, "Trace: 0x%llx", addr);
}

static u32 ppu_lwarx(u32 addr)
{
	be_t<u32> reg_value;
	vm::reservation_acquire(&reg_value, addr, sizeof(reg_value));
	return reg_value;
}

static u64 ppu_ldarx(u32 addr)
{
	be_t<u64> reg_value;
	vm::reservation_acquire(&reg_value, addr, sizeof(reg_value));
	return reg_value;
}

static bool ppu_stwcx(u32 addr, u32 reg_value)
{
	const be_t<u32> data = reg_value;
	return vm::reservation_update(addr, &data, sizeof(data));
}

static bool ppu_stdcx(u32 addr, u64 reg_value)
{
	const be_t<u64> data = reg_value;
	return vm::reservation_update(addr, &data, sizeof(data));
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

static void ppu_initialize()
{
	const auto _funcs = fxm::get_always<std::vector<ppu_function>>();

	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm || _funcs->empty())
	{
		if (!Emu.GetCPUThreadStop())
		{
			auto ppu_thr_stop_data = vm::ptr<u32>::make(vm::alloc(2 * 4, vm::main));
			Emu.SetCPUThreadStop(ppu_thr_stop_data.addr());
			ppu_thr_stop_data[0] = ppu_instructions::HACK(1);
			ppu_thr_stop_data[1] = ppu_instructions::BLR();
		}
		
		return;
	}

	std::unordered_map<std::string, std::uintptr_t> link_table
	{
		{ "__mptr", (u64)&vm::g_base_addr },
		{ "__cptr", (u64)&s_ppu_compiled },
		{ "__trap", (u64)&ppu_trap },
		{ "__end", (u64)&ppu_unreachable },
		{ "__trace", (u64)&ppu_trace },
		{ "__hlecall", (u64)&ppu_execute_function },
		{ "__syscall", (u64)&ppu_execute_syscall },
		{ "__get_tbl", (u64)&get_timebased_time },
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

#ifdef LLVM_AVAILABLE
	using namespace llvm;

	// Create LLVM module
	std::unique_ptr<Module> module = std::make_unique<Module>("", g_llvm_ctx);

	// Initialize target
	module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	
	// Initialize translator
	std::unique_ptr<PPUTranslator> translator = std::make_unique<PPUTranslator>(g_llvm_ctx, module.get(), 0);

	// Define some types
	const auto _void = Type::getVoidTy(g_llvm_ctx);
	const auto _func = FunctionType::get(_void, { translator->GetContextType()->getPointerTo() }, false);

	// Initialize function list
	for (const auto& info : *_funcs)
	{
		if (info.size)
		{
			const auto f = cast<Function>(module->getOrInsertFunction(fmt::format("__0x%x", info.addr), _func));
			f->addAttribute(1, Attribute::NoAlias);
			translator->AddFunction(info.addr, f);
		}
		
		for (const auto& b : info.blocks)
		{
			if (b.second)
			{
				translator->AddBlockInfo(b.first);
			}
		}
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
		dlg->Create("Recompiling PPU executable.\nPlease wait...");
	});

	// Translate functions
	for (size_t fi = 0; fi < _funcs->size(); fi++)
	{
		if (Emu.IsStopped())
		{
			LOG_SUCCESS(PPU, "LLVM: Translation cancelled");
			return;
		}

		auto& info = _funcs->at(fi);

		if (info.size)
		{
			// Update dialog
			Emu.CallAfter([=, max = _funcs->size()]()
			{
				dlg->ProgressBarSetMsg(0, fmt::format("Compiling %u of %u", fi + 1, max));

				if (fi * 100 / max != (fi + 1) * 100 / max)
					dlg->ProgressBarInc(0, 1);
			});

			// Translate
			const auto func = translator->TranslateToIR(info, vm::_ptr<u32>(info.addr));

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
							link_table.emplace(n, reinterpret_cast<std::uintptr_t>(ptr));

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
							link_table.emplace(n, reinterpret_cast<std::uintptr_t>(ptr));

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

	LOG_SUCCESS(PPU, "LLVM: %zu functions generated", module->getFunctionList().size());

	Module* module_ptr = module.get();

	const auto jit = fxm::make<jit_compiler>(std::move(module), std::move(link_table));

	if (!jit)
	{
		LOG_FATAL(PPU, "LLVM: Multiple modules are not yet supported");
		return;
	}

	memory_helper::free_reserved_memory(s_ppu_compiled, 0x100000000); // TODO

	// Get and install function addresses
	for (const auto& info : *_funcs)
	{
		if (info.size)
		{
			const std::uintptr_t link = jit->get(fmt::format("__0x%x", info.addr));
			ppu_register_function_at(info.addr, (ppu_function_t)link);

			LOG_NOTICE(PPU, "** Function __0x%x -> 0x%llx (size=0x%x, toc=0x%x, attr %#x)", info.addr, link, info.size, info.toc, info.attr);
		}
	}

	LOG_SUCCESS(PPU, "LLVM: Compilation finished (%s)", sys::getHostCPUName().data());
#endif
}
