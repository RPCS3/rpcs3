#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceDbg.h"

logs::channel sceDbg("sceDbg");

s32 sceDbgSetMinimumLogLevel(s32 minimumLogLevel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDbgSetBreakOnErrorState(SceDbgBreakOnErrorState state)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDbgAssertionHandler(vm::cptr<char> pFile, s32 line, b8 stop, vm::cptr<char> pComponent, vm::cptr<char> pMessage, arm_va_args_t va_args)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceDbgLoggingHandler(vm::cptr<char> pFile, s32 line, s32 severity, vm::cptr<char> pComponent, vm::cptr<char> pMessage, arm_va_args_t va_args)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceDbg, nid, name)

DECLARE(arm_module_manager::SceDbg)("SceDbg", []()
{
	REG_FUNC(0x941622FA, sceDbgSetMinimumLogLevel);
	REG_FUNC(0x1AF3678B, sceDbgAssertionHandler);
	REG_FUNC(0x6605AB19, sceDbgLoggingHandler);
	REG_FUNC(0xED4A00BA, sceDbgSetBreakOnErrorState);
});
