#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceDbg;

enum SceDbgBreakOnErrorState : s32
{
	SCE_DBG_DISABLE_BREAK_ON_ERROR = 0,
	SCE_DBG_ENABLE_BREAK_ON_ERROR
};

s32 sceDbgSetMinimumLogLevel(s32 minimumLogLevel)
{
	throw __FUNCTION__;
}

s32 sceDbgSetBreakOnErrorState(SceDbgBreakOnErrorState state)
{
	throw __FUNCTION__;
}

s32 sceDbgAssertionHandler(vm::psv::ptr<const char> pFile, s32 line, bool stop, vm::psv::ptr<const char> pComponent, vm::psv::ptr<const char> pMessage) // va_args...
{
	throw __FUNCTION__;
}

s32 sceDbgLoggingHandler(vm::psv::ptr<const char> pFile, s32 line, s32 severity, vm::psv::ptr<const char> pComponent, vm::psv::ptr<const char> pMessage) // va_args...
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceDbg, #name, name)

psv_log_base sceDbg("SceDbg", []()
{
	sceDbg.on_load = nullptr;
	sceDbg.on_unload = nullptr;
	sceDbg.on_stop = nullptr;

	REG_FUNC(0x941622FA, sceDbgSetMinimumLogLevel);
	REG_FUNC(0x1AF3678B, sceDbgAssertionHandler);
	REG_FUNC(0x6605AB19, sceDbgLoggingHandler);
	REG_FUNC(0xED4A00BA, sceDbgSetBreakOnErrorState);
});
