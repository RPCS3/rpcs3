#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceScreenShot.h"

logs::channel sceScreenShot("sceScreenShot");

s32 sceScreenShotSetParam(vm::cptr<SceScreenShotParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceScreenShotSetOverlayImage(vm::cptr<char> filePath, s32 offsetX, s32 offsetY)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceScreenShotDisable()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceScreenShotEnable()
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(libSceScreenShot, nid, name)

DECLARE(arm_module_manager::SceScreenShot)("libSceScreenShot", []()
{
	REG_FUNC(0x05DB59C7, sceScreenShotSetParam);
	REG_FUNC(0x7061665B, sceScreenShotSetOverlayImage);
	REG_FUNC(0x50AE9FF9, sceScreenShotDisable);
	REG_FUNC(0x76E674D1, sceScreenShotEnable);
});
