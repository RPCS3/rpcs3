#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceCodecEngine;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceCodecEngine, #name, name)

psv_log_base sceCodecEngine("SceCodecEngine", []()
{
	sceCodecEngine.on_load = nullptr;
	sceCodecEngine.on_unload = nullptr;
	sceCodecEngine.on_stop = nullptr;

	//REG_FUNC(0x3E718890, sceCodecEnginePmonStart);
	//REG_FUNC(0x268B1EF5, sceCodecEnginePmonStop);
	//REG_FUNC(0x859E4A68, sceCodecEnginePmonGetProcessorLoad);
	//REG_FUNC(0xA097E4C8, sceCodecEnginePmonReset);
});
