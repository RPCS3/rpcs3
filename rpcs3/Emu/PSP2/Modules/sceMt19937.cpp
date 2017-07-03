#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceMt19937.h"

logs::channel sceMt19937("sceMt19937");

s32 sceMt19937Init(vm::ptr<SceMt19937Context> pCtx, u32 seed)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u32 sceMt19937UInt(vm::ptr<SceMt19937Context> pCtx)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceMt19937, nid, name)

DECLARE(arm_module_manager::SceMt19937)("SceMt19937", []()
{
	REG_FUNC(0xEE5BA27C, sceMt19937Init);
	REG_FUNC(0x29E43BB5, sceMt19937UInt);
});
