#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceMt19937;

struct SceMt19937Context
{
	u32 count;
	u32 state[624];
};

s32 sceMt19937Init(vm::psv::ptr<SceMt19937Context> pCtx, u32 seed)
{
	throw __FUNCTION__;
}

u32 sceMt19937UInt(vm::psv::ptr<SceMt19937Context> pCtx)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceMt19937, #name, name)

psv_log_base sceMt19937("SceMt19937", []()
{
	sceMt19937.on_load = nullptr;
	sceMt19937.on_unload = nullptr;
	sceMt19937.on_stop = nullptr;

	REG_FUNC(0xEE5BA27C, sceMt19937Init);
	REG_FUNC(0x29E43BB5, sceMt19937UInt);
});
