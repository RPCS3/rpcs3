#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceDbg;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDbg, #name, name)

psv_log_base sceDbg("SceDbg", []()
{
	sceDbg.on_load = nullptr;
	sceDbg.on_unload = nullptr;
	sceDbg.on_stop = nullptr;

	//REG_FUNC(0x941622FA, sceDbgSetMinimumLogLevel);
	//REG_FUNC(0x1AF3678B, sceDbgAssertionHandler);
	//REG_FUNC(0x6605AB19, sceDbgLoggingHandler);
	//REG_FUNC(0xED4A00BA, sceDbgSetBreakOnErrorState);
});
