#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPURecompiler.h"
#include "SPUASMJITRecompiler.h"
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
	const auto _ls = vm::_ptr<u32>(spu.offset);

	// Search if cached data matches
	auto & func = spu.compiled_cache[spu.pc / 4];
	if (func.dirty_bit)
	{
		func.dirty_bit = false;
		
		// This memcmp acts as a fast path instead of finding it again in analyse.
		if (memcmp(func.contents->data.data(), _ls + (spu.pc / 4), func.contents->size) != 0)
		{
			func.contents = spu.spu_db->analyse(_ls, spu.pc, func.contents.get());
		}
	}
	else if (!func)
	{
		func.contents = spu.spu_db->analyse(_ls, spu.pc);
		spu.compiled_functions.push_back(&func);
	}

	// Reset callstack if necessary
	if ((func.contents->does_reset_stack && spu.recursion_level) || spu.recursion_level >= 128)
	{
		spu.state += cpu_flag::ret;
		return;
	}

	// Compile if needed
	if (!func.contents->compiled)
	{
		if (!spu.spu_rec)
		{
			spu.spu_rec = fxm::get_always<spu_recompiler>();
		}

		spu.spu_rec->compile(func.contents);

		if (!func.contents->compiled) fmt::throw_exception("Compilation failed" HERE);
	}

	const u32 res = func.contents->compiled(&spu, _ls);

	if (spu.pending_exception)
	{
		const auto exception = spu.pending_exception;
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
