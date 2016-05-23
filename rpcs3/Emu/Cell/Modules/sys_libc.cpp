#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUOpcodes.h"

logs::channel sys_libc("sys_libc", logs::level::notice);

namespace sys_libc_func
{
	void memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
	{
		sys_libc.trace("memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

		::memcpy(dst.get_ptr(), src.get_ptr(), size);
	}
}

// Define macro for namespace
#define REG_FUNC_(name) REG_FNID(sys_libc, ppu_generate_id(#name), sys_libc_func::name)

DECLARE(ppu_module_manager::sys_libc)("sys_libc", []()
{
	REG_FUNC_(memcpy);
});
