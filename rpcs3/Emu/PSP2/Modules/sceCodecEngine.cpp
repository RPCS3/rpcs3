#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceCodecEngine.h"

logs::channel sceCodecEngine("sceCodecEngine", logs::level::notice);

s32 sceCodecEnginePmonStart()
{
	throw EXCEPTION("");
}

s32 sceCodecEnginePmonStop()
{
	throw EXCEPTION("");
}

s32 sceCodecEnginePmonGetProcessorLoad(vm::ptr<SceCodecEnginePmonProcessorLoad> pProcessorLoad)
{
	throw EXCEPTION("");
}

s32 sceCodecEnginePmonReset()
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceCodecEngine, nid, name)

DECLARE(arm_module_manager::SceCodecEngine)("SceCodecEngine", []()
{
	REG_FUNC(0x3E718890, sceCodecEnginePmonStart);
	REG_FUNC(0x268B1EF5, sceCodecEnginePmonStop);
	REG_FUNC(0x859E4A68, sceCodecEnginePmonGetProcessorLoad);
	REG_FUNC(0xA097E4C8, sceCodecEnginePmonReset);
});
