#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUInstrTable.h"

extern Module sys_libc;

namespace sys_libc_func
{
	void memcpy(vm::ptr<void> dst, vm::ptr<const void> src, u32 size)
	{
		sys_libc.Warning("memcpy(dst=0x%x, src=0x%x, size=0x%x)", dst, src, size);

		::memcpy(dst.get_ptr(), src.get_ptr(), size);
	}
}

Module sys_libc("sys_libc", []()
{
	using namespace PPU_instr;

	REG_SUB(sys_libc, "", sys_libc_func::, memcpy,
		{ SPET_MASKED_OPCODE, 0x2ba50007, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78630020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78840020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7c6b1b78, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x419d0070, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2c250000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d820020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x28a5000f, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x40850024, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x78ace8c2, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7d8903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xe8e40000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38a5fff8, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38840008, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0xf8eb0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x396b0008, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4200ffec, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x2c250000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4d820020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7ca903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x88040000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38840001, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x980b0000, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x396b0001, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4200fff0, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4e800020, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x7ce903a6, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x8d04ffff, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x38a5ffff, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x9d0bffff, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x4200fff4, 0xffffffff },
		{ SPET_MASKED_OPCODE, 0x48000034, 0xffffffff },
	);
});
