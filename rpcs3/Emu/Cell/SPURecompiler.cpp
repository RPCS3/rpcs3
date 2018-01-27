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
	auto func_ptr = spu.compiled_cache[spu.pc / 4];

	// func.contents is there only as a temporary test, to see if SPU code not getting invalidated is the reason for the crashes
	if (func_ptr && func_ptr->dirty_bit)
	{
		auto & func = *func_ptr;
		func.dirty_bit = false;
		u32 index = (reinterpret_cast<size_t>(func_ptr) - reinterpret_cast<size_t>(&spu.compiled_functions[0])) / sizeof(func);
		spu.first_clean_func_index = std::min<u32>(index, spu.first_clean_func_index);
		spu.last_clean_func_index = std::max<u32>(index + 1, spu.last_clean_func_index);

		if (!spu.same_function(func.contents, _ls + (spu.pc / 4)))
		{
			func.contents = spu.spu_db->analyse(_ls, spu.pc, func.contents);
		}
	}
	else if (!func_ptr)
	{
		auto & func = spu.compiled_functions[++spu.next_compiled_func_index];
		func.contents = spu.spu_db->analyse(_ls, spu.pc);
		func_ptr = &func;
		spu.compiled_cache[spu.pc / 4] = func_ptr;
		spu.last_clean_func_index = spu.next_compiled_func_index + 1;
		spu.first_clean_func_index = std::min<u32>(spu.first_clean_func_index, spu.next_compiled_func_index);
	}

	// Reset callstack if necessary
	if ((func_ptr->contents->does_reset_stack && spu.recursion_level) || spu.recursion_level >= 128)
	{
		spu.state += cpu_flag::ret;
		return;
	}

	// Compile if needed
	if (!func_ptr->contents->compiled)
	{
		if (!spu.spu_rec)
		{
			spu.spu_rec = fxm::get_always<spu_recompiler>();
		}

		spu.spu_rec->compile(func_ptr->contents);

		if (!func_ptr->contents->compiled) fmt::throw_exception("Compilation failed" HERE);
	}

	const u32 res = func_ptr->contents->compiled(&spu, _ls);

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
