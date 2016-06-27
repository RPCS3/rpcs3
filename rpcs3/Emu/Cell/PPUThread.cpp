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

const auto s_ppu_compiled = static_cast<ppu_function_t*>(memory_helper::reserve_memory(0x200000000));

extern void ppu_register_function_at(u32 addr, ppu_function_t ptr)
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		memory_helper::commit_page_memory(s_ppu_compiled + addr / 4, sizeof(ppu_function_t));
		s_ppu_compiled[addr / 4] = ptr;
	}
}

std::string PPUThread::get_name() const
{
	return fmt::format("PPU[0x%x] Thread (%s)", id, m_name);
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

	return custom_task ? custom_task(*this) : fast_call(pc, static_cast<u32>(GPR[2]));
}

void PPUThread::cpu_task_main()
{
	if (g_cfg_ppu_decoder.get() == ppu_decoder_type::llvm)
	{
		return s_ppu_compiled[pc / 4](*this);
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
	: cpu_thread(cpu_type::ppu)
	, m_name(name)
{
}

be_t<u64>* PPUThread::get_stack_arg(s32 i, u64 align)
{
	if (align != 1 && align != 2 && align != 4 && align != 8 && align != 16) throw fmt::exception("Unsupported alignment: 0x%llx" HERE, align);
	return vm::_ptr<u64>(vm::cast((GPR[1] + 0x30 + 0x8 * (i - 1)) & (0 - align), HERE));
}

void PPUThread::fast_call(u32 addr, u32 rtoc)
{
	const auto old_PC = pc;
	const auto old_stack = GPR[1];
	const auto old_rtoc = GPR[2];
	const auto old_LR = LR;
	const auto old_task = std::move(custom_task);
	const auto old_func = last_function;

	pc = addr;
	GPR[2] = rtoc;
	LR = Emu.GetCPUThreadStop();
	custom_task = nullptr;
	last_function = nullptr;

	try
	{
		cpu_task_main();

		if (GPR[1] != old_stack && !state.test(cpu_state::ret) && !state.test(cpu_state::exit)) // GPR[1] shouldn't change
		{
			throw fmt::exception("Stack inconsistency (addr=0x%x, rtoc=0x%x, SP=0x%llx, old=0x%llx)", addr, rtoc, GPR[1], old_stack);
		}
	}
	catch (cpu_state _s)
	{
		state += _s;
		if (_s != cpu_state::ret) throw;
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

	state -= cpu_state::ret;

	pc = old_PC;
	GPR[1] = old_stack;
	GPR[2] = old_rtoc;
	LR = old_LR;
	custom_task = std::move(old_task);
	last_function = old_func;

	//if (custom_task)
	//{
	//	state += cpu_state::interrupt;
	//	handle_interrupt();
	//}
}

const ppu_decoder<ppu_itype> s_ppu_itype;

extern u64 get_timebased_time();
extern void ppu_execute_syscall(PPUThread& ppu, u64 code);
extern void ppu_execute_function(PPUThread& ppu, u32 index);
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
	throw fmt::exception("Trap! (0x%llx)", addr);
}

static void ppu_trace(u64 addr)
{
	LOG_NOTICE(PPU, "Trace: 0x%llx", addr);
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

// Interpreter call for simple vector instructions
static __m128i ppu_vec3op(decltype(&ppu_interpreter::UNK) func, __m128i _a, __m128i _b, __m128i _c)
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

extern void ppu_initialize(const std::string& name, const std::vector<std::pair<u32, u32>>& funcs, u32 entry)
{
	if (g_cfg_ppu_decoder.get() != ppu_decoder_type::llvm || funcs.empty())
	{
		return;
	}

	std::unordered_map<std::string, std::uintptr_t> link_table
	{
		{ "__memory", (u64)vm::g_base_addr },
		{ "__memptr", (u64)&vm::g_base_addr },
		{ "__trap", (u64)&ppu_trap },
		{ "__trace", (u64)&ppu_trace },
		{ "__hlecall", (u64)&ppu_execute_function },
		{ "__syscall", (u64)&ppu_execute_syscall },
		{ "__get_tbl", (u64)&get_timebased_time },
		{ "__call", (u64)s_ppu_compiled },
		{ "__lwarx", (u64)&ppu_lwarx },
		{ "__ldarx", (u64)&ppu_ldarx },
		{ "__stwcx", (u64)&ppu_stwcx },
		{ "__stdcx", (u64)&ppu_stdcx },
		{ "__vec3op", (u64)&ppu_vec3op },
		{ "__adde_get_ca", (u64)&adde_carry },
		{ "__vexptefp", (u64)&sse_exp2_ps },
		{ "__vlogefp", (u64)&sse_log2_ps },
		{ "__vperm", (u64)&sse_altivec_vperm },
		{ "__vrefp", (u64)&sse_rcp_ps },
		{ "__vrsqrtefp", (u64)&sse_rsqrt_ps },
		{ "__lvsl", (u64)&sse_altivec_lvsl },
		{ "__lvsr", (u64)&sse_altivec_lvsr },
		{ "__lvlx", (u64)&sse_cellbe_lvlx },
		{ "__lvrx", (u64)&sse_cellbe_lvrx },
		{ "__stvlx", (u64)&sse_cellbe_stvlx },
		{ "__stvrx", (u64)&sse_cellbe_stvrx },
		{ "__fre", (u64)&sse_rcp_ss },
		{ "__frsqrte", (u64)&sse_rsqrt_ss },
	};

#ifdef LLVM_AVAILABLE
	using namespace llvm;

	// Create LLVM module
	std::unique_ptr<Module> module = std::make_unique<Module>(name, g_llvm_ctx);

	// Initialize target
	module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
	
	// Initialize translator
	std::unique_ptr<PPUTranslator> translator = std::make_unique<PPUTranslator>(g_llvm_ctx, module.get(), 0, entry);

	// Define some types
	const auto _void = Type::getVoidTy(g_llvm_ctx);
	const auto _func = FunctionType::get(_void, { translator->GetContextType()->getPointerTo() }, false);

	// Initialize function list
	for (const auto& info : funcs)
	{
		if (info.second)
		{
			const auto f = cast<Function>(module->getOrInsertFunction(fmt::format("__sub_%x", info.first), _func));
			f->addAttribute(1, Attribute::NoAlias);
			translator->AddFunction(info.first, f);
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
	//pm.add(createBasicAAWrapperPass());
	pm.add(new MemoryDependenceAnalysis());
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
	
	// Translate functions
	for (const auto& info : funcs)
	{
		if (info.second)
		{
			const auto func = translator->TranslateToIR(info.first, info.first + info.second, vm::_ptr<u32>(info.first));

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
				}
			}
		}
	}

	legacy::PassManager mpm;

	// Remove unused functions, structs, global variables, etc
	mpm.add(createStripDeadPrototypesPass());
	//mpm.add(createFunctionInliningPass());
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

	const auto jit = fxm::make<jit_compiler>(std::move(module), std::move(link_table));

	if (!jit)
	{
		LOG_FATAL(PPU, "LLVM: Multiple modules are not yet supported");
		return;
	}

	memory_helper::free_reserved_memory(s_ppu_compiled, 0x200000000); // TODO

	// Get and install function addresses
	for (const auto& info : funcs)
	{
		const u32 addr = info.first;
		if (info.second)
		{
			const std::uintptr_t link = jit->get(fmt::format("__sub_%x", addr));
			memory_helper::commit_page_memory(s_ppu_compiled + addr / 4, sizeof(ppu_function_t));
			s_ppu_compiled[addr / 4] = (ppu_function_t)link;

			LOG_NOTICE(PPU, "** Function __sub_%x -> 0x%llx (addr=0x%x, size=0x%x)", addr, link, addr, info.second);
		}
	}

	LOG_SUCCESS(PPU, "LLVM: Compilation finished (%s)", sys::getHostCPUName().data());
#endif
}
