#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPURecompiler.h"
#include "SPUASMJITRecompiler.h"

extern u64 get_system_time();

SPURecompilerDecoder::SPURecompilerDecoder(SPUThread& spu)
	: db(fxm::get_always<SPUDatabase>())
	, rec(fxm::get_always<spu_recompiler>())
	, spu(spu)
{
}

u32 SPURecompilerDecoder::DecodeMemory(const u32 address)
{
	if (spu.offset != address - spu.pc || spu.pc >= 0x40000 || spu.pc % 4)
	{
		throw EXCEPTION("Invalid address or PC (address=0x%x, PC=0x%05x)", address, spu.pc);
	}

	const auto _ls = vm::get_ptr<be_t<u32>>(spu.offset);

	const u32 index = spu.pc / 4;

	if (!m_entries.at(index) || true) // always validate (TODO)
	{
		const auto func = db->analyse(_ls, spu.pc);

		if (!func->compiled) rec->compile(*func);

		if (!func->compiled) throw EXCEPTION("Compilation failed");

		m_entries[index] = func->compiled;
	}

	const u32 res = m_entries[index](&spu, _ls);

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
			throw EXCEPTION("Undefined behaviour");
		}

		spu.set_interrupt_status(true);
	}
	else if (res & 0x8000000)
	{
		spu.set_interrupt_status(false);
	}

	spu.pc = res & 0x3fffc;

	return 0;
}
