#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUOpcodes.h"

namespace vm { using namespace ps3; }

logs::channel sys_libc("sys_libc");

void sys_libc_memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
{
	sys_libc.trace("memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

	::memcpy(dst.get_ptr(), src.get_ptr(), size);
}

DECLARE(ppu_module_manager::sys_libc)("sys_libc", []()
{
	REG_FNID(sys_libc, "memcpy", sys_libc_memcpy).flags = MFF_FORCED_HLE;
});
