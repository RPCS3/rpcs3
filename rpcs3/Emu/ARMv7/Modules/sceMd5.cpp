#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceMd5;

struct SceMd5Context
{
	u32 h[4];
	u32 pad;
	u16 usRemains;
	u16 usComputed;
	u64 ullTotalLen;
	u8 buf[64];
	u8 result[64];
};

s32 sceMd5Digest(vm::psv::ptr<const void> plain, u32 len, vm::psv::ptr<u8> digest)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockInit(vm::psv::ptr<SceMd5Context> pContext)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockUpdate(vm::psv::ptr<SceMd5Context> pContext, vm::psv::ptr<const void> plain, u32 len)
{
	throw __FUNCTION__;
}

s32 sceMd5BlockResult(vm::psv::ptr<SceMd5Context> pContext, vm::psv::ptr<u8> digest)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceMd5, #name, name)

psv_log_base sceMd5("SceMd5", []()
{
	sceMd5.on_load = nullptr;
	sceMd5.on_unload = nullptr;
	sceMd5.on_stop = nullptr;

	REG_FUNC(0xB845BCCB, sceMd5Digest);
	REG_FUNC(0x4D6436F9, sceMd5BlockInit);
	REG_FUNC(0x094A4902, sceMd5BlockUpdate);
	REG_FUNC(0xB94ABF83, sceMd5BlockResult);
});
