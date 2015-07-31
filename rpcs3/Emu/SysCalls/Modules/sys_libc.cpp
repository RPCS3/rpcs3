#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUInstrTable.h"

namespace vm { using namespace ps3; }

extern Module sys_libc;

namespace sys_libc_func
{
	void memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
	{
		sys_libc.Log("memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

		::memcpy(dst.get_ptr(), src.get_ptr(), size);
	}
}

Module sys_libc("sys_libc", []()
{
	using namespace PPU_instr;

	REG_SUB(sys_libc, sys_libc_func, memcpy,
		SP_I(CMPLDI(cr7, r5, 7)),
		SP_I(CLRLDI(r3, r3, 32)),
		SP_I(CLRLDI(r4, r4, 32)),
		SP_I(MR(r11, r3)),
		SP_I(BGT(cr7, XXX & 0xff)),
		SP_I(CMPDI(r5, 0)),
		OPT_SP_I(MR(r9, r3)),
		{ SPET_MASKED_OPCODE, 0x4d820020, 0xffffffff },
	);
});
