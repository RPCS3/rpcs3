#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPURecompiler.h"
#include "SPUASMJITRecompiler.h"
#include "SPULLVMRecompiler.h"
#include <algorithm>

extern u64 get_system_time();

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::enter(SPUThread& spu)
{
	if (spu.pc >= 0x40000 || spu.pc % 4)
	{
		fmt::throw_exception("Invalid PC: 0x%05x", spu.pc);
	}

	// Get SPU LS pointer
	const auto _ls = vm::ps3::_ptr<u32>(spu.offset);

	// Search if cached data matches
	auto func = spu.compiled_cache[spu.pc / 4];

	// Check shared db if we dont have a match
	if (!func || !std::equal(func->data.begin(), func->data.end(), _ls + spu.pc / 4, [](const be_t<u32>& l, const be_t<u32>& r) { return *(u32*)(u8*)&l == *(u32*)(u8*)&r; }))
	{
		func = spu.spu_db->analyse(_ls, spu.pc);
		spu.compiled_cache[spu.pc / 4] = func;
	}

	// Reset callstack if necessary
	if ((func->does_reset_stack && spu.recursion_level) || spu.recursion_level >= 128)
	{
		spu.state += cpu_flag::ret;
		return;
	}

	// Compile if needed
	if (!func->compiled)
	{
		if (!spu.spu_rec)
		{
			if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
			{
				spu.spu_rec = fxm::get_always<spu_recompiler>(spu);
			}
			else if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
			{
				spu.spu_rec = fxm::get_always<spu_llvm_recompiler>(spu);
			}
		}

		spu.spu_rec->compile(*func);

		if (!func->compiled) fmt::throw_exception("Compilation failed" HERE);
	}

	const u32 res = func->compiled(&spu, _ls);

	if (const auto exception = spu.pending_exception)
	{
		spu.pending_exception = nullptr;
		std::rethrow_exception(exception);
	}

	if (res & 0x1000000)
	{
		spu.halt();
	}

	if (res & 0x2000000)
	{
	}

	if (res & 0x4000000)
	{
		if (res & 0x8000000)
		{
			fmt::throw_exception("Invalid interrupt status set (0x%x)" HERE, res);
		}

		spu.set_interrupt_status(true);
	}
	else if (res & 0x8000000)
	{
		spu.set_interrupt_status(false);
	}

	spu.pc = res & 0x3fffc;

	if (spu.interrupts_enabled && (spu.ch_event_mask & spu.ch_event_stat & SPU_EVENT_INTR_IMPLEMENTED) > 0)
	{
        spu.interrupts_enabled = false;
		spu.srr0 = std::exchange(spu.pc, 0);
	}
}

u32 spu_recompiler_base::SPUFunctionCall(SPUThread *spu, u32 link) noexcept
{
	spu->recursion_level++;

	try
	{
		// TODO: check correctness

		if (spu->pc & 0x4000000)
		{
			if (spu->pc & 0x8000000)
			{
				fmt::throw_exception("Undefined behaviour" HERE);
			}

			spu->set_interrupt_status(true);
			spu->pc &= ~0x4000000;
		}
		else if (spu->pc & 0x8000000)
		{
			spu->set_interrupt_status(false);
			spu->pc &= ~0x8000000;
		}

		if (spu->pc == link)
		{
			LOG_ERROR(SPU, "Branch-to-next");
		}
		else if (spu->pc == link - 4)
		{
			LOG_ERROR(SPU, "Branch-to-self");
		}

		while (!test(spu->state) || !spu->check_state())
		{
			// Proceed recursively
			spu_recompiler_base::enter(*spu);

			if (test(spu->state & cpu_flag::ret))
			{
				break;
			}

			if (spu->pc == link)
			{
				spu->recursion_level--;
				return 0; // Successfully returned 
			}
		}

		spu->recursion_level--;
		return 0x2000000 | spu->pc;
	}
	catch (...)
	{
		spu->pending_exception = std::current_exception();

		spu->recursion_level--;
		return 0x1000000 | spu->pc;
	}
}

u32 spu_recompiler_base::SPUInterpreterCall(SPUThread* spu, u32 opcode, spu_inter_func_t func) noexcept
{
	try
	{
		// TODO: check correctness

		const u32 old_pc = spu->pc;

		if (test(spu->state) && spu->check_state())
		{
			return 0x2000000 | spu->pc;
		}

		func(*spu, { opcode });

		if (old_pc != spu->pc)
		{
			spu->pc += 4;
			return 0x2000000 | spu->pc;
		}

		spu->pc += 4;
		return 0;
	}
	catch (...)
	{
		spu->pending_exception = std::current_exception();
		return 0x1000000 | spu->pc;
	}
}