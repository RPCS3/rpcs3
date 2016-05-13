#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"
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
