#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceDbg.h"

s32 sceDbgSetMinimumLogLevel(s32 minimumLogLevel)
{
	throw __FUNCTION__;
}

s32 sceDbgSetBreakOnErrorState(SceDbgBreakOnErrorState state)
{
	throw __FUNCTION__;
}

s32 sceDbgAssertionHandler(vm::cptr<char> pFile, s32 line, bool stop, vm::cptr<char> pComponent, vm::cptr<char> pMessage) // va_args...
{
	throw __FUNCTION__;
}

s32 sceDbgLoggingHandler(vm::cptr<char> pFile, s32 line, s32 severity, vm::cptr<char> pComponent, vm::cptr<char> pMessage) // va_args...
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceDbg, #name, name)

psv_log_base sceDbg("SceDbg", []()
{
	sceDbg.on_load = nullptr;
	sceDbg.on_unload = nullptr;
	sceDbg.on_stop = nullptr;
	sceDbg.on_error = nullptr;

	REG_FUNC(0x941622FA, sceDbgSetMinimumLogLevel);
	REG_FUNC(0x1AF3678B, sceDbgAssertionHandler);
	REG_FUNC(0x6605AB19, sceDbgLoggingHandler);
	REG_FUNC(0xED4A00BA, sceDbgSetBreakOnErrorState);
});
