#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceAppUtil;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAppUtil, #name, name)

psv_log_base sceAppUtil("SceAppUtil", []()
{
	sceAppUtil.on_load = nullptr;
	sceAppUtil.on_unload = nullptr;
	sceAppUtil.on_stop = nullptr;

	//REG_FUNC(0xDAFFE671, sceAppUtilInit);
	//REG_FUNC(0xB220B00B, sceAppUtilShutdown);
	//REG_FUNC(0x7E8FE96A, sceAppUtilSaveDataSlotCreate);
	//REG_FUNC(0x266A7646, sceAppUtilSaveDataSlotDelete);
	//REG_FUNC(0x98630136, sceAppUtilSaveDataSlotSetParam);
	//REG_FUNC(0x93F0D89F, sceAppUtilSaveDataSlotGetParam);
	//REG_FUNC(0x1E2A6158, sceAppUtilSaveDataFileSave);
	//REG_FUNC(0xEE85804D, sceAppUtilPhotoMount);
	//REG_FUNC(0x9651B941, sceAppUtilPhotoUmount);
	//REG_FUNC(0x5DFB9CA0, sceAppUtilSystemParamGetInt);
	//REG_FUNC(0x6E6AA267, sceAppUtilSystemParamGetString);
	//REG_FUNC(0x9D8AC677, sceAppUtilSaveSafeMemory);
	//REG_FUNC(0x3424D772, sceAppUtilLoadSafeMemory);
});
