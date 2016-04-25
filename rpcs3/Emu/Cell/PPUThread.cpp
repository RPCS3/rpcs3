#include "stdafx.h"
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

		return fmt::format("%s [0x%08x]", cpu->get_name(), cpu->PC);
	};

	const auto base = vm::_ptr<const u8>(0);

	// Select opcode table
	const auto& table = *(
		g_cfg_ppu_decoder.get() == ppu_decoder_type::precise ? &s_ppu_interpreter_precise.get_table() :
		g_cfg_ppu_decoder.get() == ppu_decoder_type::fast ? &s_ppu_interpreter_fast.get_table() :
		throw std::logic_error("Invalid PPU decoder"));

	u32 _pc{};
	u32 op0, op1, op2;
	ppu_inter_func_t func0, func1, func2;

	while (true)
	{
		if (LIKELY(_pc == PC && !state.load()))
		{
			func0(*this, { op0 });

			if (LIKELY((_pc += 4) == (PC += 4) && !state.load()))
			{
				func1(*this, { op1 });

				if (LIKELY((_pc += 4) == (PC += 4)))
				{
					op0 = op2;
					func0 = func2;
					const auto ops = reinterpret_cast<const be_t<u32>*>(base + _pc);
					func1 = table[ppu_decode(op1 = ops[1])];
					func2 = table[ppu_decode(op2 = ops[2])];
					continue;
				}
			}
		}

		// Reinitialize
		_pc = PC;
		const auto ops = reinterpret_cast<const be_t<u32>*>(base + _pc);
		func0 = table[ppu_decode(op0 = ops[0])];
		func1 = table[ppu_decode(op1 = ops[1])];
		func2 = table[ppu_decode(op2 = ops[2])];

		if (UNLIKELY(check_status())) return;
	}
}

bool PPUThread::handle_interrupt()
{
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
	auto old_PC = PC;
	auto old_stack = GPR[1];
	auto old_rtoc = GPR[2];
	auto old_LR = LR;
	auto old_task = std::move(custom_task);

	PC = addr;
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

	PC = old_PC;

	if (GPR[1] != old_stack) // GPR[1] shouldn't change
	{
		throw EXCEPTION("Stack inconsistency (addr=0x%x, rtoc=0x%x, SP=0x%llx, old=0x%llx)", addr, rtoc, GPR[1], old_stack);
	}

	GPR[2] = old_rtoc;
	LR = old_LR;
	custom_task = std::move(old_task);
}
