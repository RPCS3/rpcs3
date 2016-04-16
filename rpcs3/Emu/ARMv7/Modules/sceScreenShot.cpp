#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/ARMv7Module.h"

#include "sceScreenShot.h"

LOG_CHANNEL(sceScreenShot);

s32 sceScreenShotSetParam(vm::cptr<SceScreenShotParam> param)
{
	throw EXCEPTION("");
}

s32 sceScreenShotSetOverlayImage(vm::cptr<char> filePath, s32 offsetX, s32 offsetY)
{
	throw EXCEPTION("");
}

s32 sceScreenShotDisable()
{
	throw EXCEPTION("");
}

s32 sceScreenShotEnable()
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(libSceScreenShot, nid, name)

DECLARE(arm_module_manager::SceScreenShot)("libSceScreenShot", []()
{
	REG_FUNC(0x05DB59C7, sceScreenShotSetParam);
	REG_FUNC(0x7061665B, sceScreenShotSetOverlayImage);
	REG_FUNC(0x50AE9FF9, sceScreenShotDisable);
	REG_FUNC(0x76E674D1, sceScreenShotEnable);
});
