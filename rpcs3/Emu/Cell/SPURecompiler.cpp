#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPURecompiler.h"
#include "SPUASMJITRecompiler.h"

extern u64 get_system_time();

void spu_recompiler_base::enter(SPUThread& spu)
{
	if (spu.pc >= 0x40000 || spu.pc % 4)
	{
		throw fmt::exception("Invalid PC: 0x%05x", spu.pc);
	}

	// Get SPU LS pointer
	const auto _ls = vm::ps3::_ptr<u32>(spu.offset);

	// Always validate (TODO)
	const auto func = spu.spu_db->analyse(_ls, spu.pc);

	// Reset callstack if necessary
	if (func->does_reset_stack && spu.recursion_level)
	{
		spu.state += cpu_state::ret;
		return;
	}

	if (!func->compiled)
	{
		if (!spu.spu_rec)
		{
			spu.spu_rec = fxm::get_always<spu_recompiler>();
		}

		spu.spu_rec->compile(*func);

		if (!func->compiled) throw std::runtime_error("Compilation failed" HERE);
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
			throw std::logic_error("Invalid interrupt status set" HERE);
		}

		spu.set_interrupt_status(true);
	}
	else if (res & 0x8000000)
	{
		spu.set_interrupt_status(false);
	}

	spu.pc = res & 0x3fffc;
}
