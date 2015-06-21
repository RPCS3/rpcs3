#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceMd5.h"

s32 sceMd5Digest(vm::cptr<void> plain, u32 len, vm::ptr<u8> digest)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockInit(vm::ptr<SceMd5Context> pContext)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockUpdate(vm::ptr<SceMd5Context> pContext, vm::cptr<void> plain, u32 len)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockResult(vm::ptr<SceMd5Context> pContext, vm::ptr<u8> digest)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceMd5, #name, name)

psv_log_base sceMd5("SceMd5", []()
{
	sceMd5.on_load = nullptr;
	sceMd5.on_unload = nullptr;
	sceMd5.on_stop = nullptr;
	sceMd5.on_error = nullptr;

	REG_FUNC(0xB845BCCB, sceMd5Digest);
	REG_FUNC(0x4D6436F9, sceMd5BlockInit);
	REG_FUNC(0x094A4902, sceMd5BlockUpdate);
	REG_FUNC(0xB94ABF83, sceMd5BlockResult);
});
