#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceCodecEngine.h"

logs::channel sceCodecEngine("sceCodecEngine");

s32 sceCodecEnginePmonStart()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCodecEnginePmonStop()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCodecEnginePmonGetProcessorLoad(vm::ptr<SceCodecEnginePmonProcessorLoad> pProcessorLoad)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceCodecEnginePmonReset()
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceCodecEngine, nid, name)

DECLARE(arm_module_manager::SceCodecEngine)("SceCodecEngine", []()
{
	REG_FUNC(0x3E718890, sceCodecEnginePmonStart);
	REG_FUNC(0x268B1EF5, sceCodecEnginePmonStop);
	REG_FUNC(0x859E4A68, sceCodecEnginePmonGetProcessorLoad);
	REG_FUNC(0xA097E4C8, sceCodecEnginePmonReset);
});
