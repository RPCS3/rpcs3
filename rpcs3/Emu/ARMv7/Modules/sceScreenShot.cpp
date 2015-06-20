#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceScreenShot.h"

s32 sceScreenShotSetParam(vm::ptr<const SceScreenShotParam> param)
{
	throw __FUNCTION__;
}

s32 sceScreenShotSetOverlayImage(vm::ptr<const char> filePath, s32 offsetX, s32 offsetY)
{
	throw __FUNCTION__;
}

s32 sceScreenShotDisable()
{
	throw __FUNCTION__;
}

s32 sceScreenShotEnable()
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceScreenShot, #name, name)

psv_log_base sceScreenShot("SceScreenShot", []()
{
	sceScreenShot.on_load = nullptr;
	sceScreenShot.on_unload = nullptr;
	sceScreenShot.on_stop = nullptr;
	sceScreenShot.on_error = nullptr;

	REG_FUNC(0x05DB59C7, sceScreenShotSetParam);
	REG_FUNC(0x7061665B, sceScreenShotSetOverlayImage);
	REG_FUNC(0x50AE9FF9, sceScreenShotDisable);
	REG_FUNC(0x76E674D1, sceScreenShotEnable);
});
