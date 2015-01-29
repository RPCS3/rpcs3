#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceTouch;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceTouch, #name, name)

psv_log_base sceTouch("SceTouch", []()
{
	sceTouch.on_load = nullptr;
	sceTouch.on_unload = nullptr;
	sceTouch.on_stop = nullptr;

	//REG_FUNC(0x169A1D58, sceTouchRead);
	//REG_FUNC(0xFF082DF0, sceTouchPeek);
	//REG_FUNC(0x1B9C5D14, sceTouchSetSamplingState);
	//REG_FUNC(0x26531526, sceTouchGetSamplingState);
	//REG_FUNC(0x10A2CA25, sceTouchGetPanelInfo);
});
