#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceMd5;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceMd5, #name, name)

psv_log_base sceMd5("SceMd5", []()
{
	sceMd5.on_load = nullptr;
	sceMd5.on_unload = nullptr;
	sceMd5.on_stop = nullptr;

	//REG_FUNC(0xB845BCCB, sceMd5Digest);
	//REG_FUNC(0x4D6436F9, sceMd5BlockInit);
	//REG_FUNC(0x094A4902, sceMd5BlockUpdate);
	//REG_FUNC(0xB94ABF83, sceMd5BlockResult);
});
