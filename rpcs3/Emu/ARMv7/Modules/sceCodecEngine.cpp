#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceCodecEngine;

struct SceCodecEnginePmonProcessorLoad
{
	u32 size;
	u32 average;
};

s32 sceCodecEnginePmonStart()
{
	throw __FUNCTION__;
}

s32 sceCodecEnginePmonStop()
{
	throw __FUNCTION__;
}

s32 sceCodecEnginePmonGetProcessorLoad(vm::psv::ptr<SceCodecEnginePmonProcessorLoad> pProcessorLoad)
{
	throw __FUNCTION__;
}

s32 sceCodecEnginePmonReset()
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceCodecEngine, #name, name)

psv_log_base sceCodecEngine("SceCodecEngine", []()
{
	sceCodecEngine.on_load = nullptr;
	sceCodecEngine.on_unload = nullptr;
	sceCodecEngine.on_stop = nullptr;

	REG_FUNC(0x3E718890, sceCodecEnginePmonStart);
	REG_FUNC(0x268B1EF5, sceCodecEnginePmonStop);
	REG_FUNC(0x859E4A68, sceCodecEnginePmonGetProcessorLoad);
	REG_FUNC(0xA097E4C8, sceCodecEnginePmonReset);
});
