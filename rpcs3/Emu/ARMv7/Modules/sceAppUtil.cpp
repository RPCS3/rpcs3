#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceAppUtil.h"

s32 sceAppUtilInit(vm::ptr<const SceAppUtilInitParam> initParam, vm::ptr<SceAppUtilBootParam> bootParam)
{
	throw __FUNCTION__;
}

s32 sceAppUtilShutdown()
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveDataSlotCreate(u32 slotId, vm::ptr<const SceAppUtilSaveDataSlotParam> param, vm::ptr<const SceAppUtilSaveDataMountPoint> mountPoint)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveDataSlotDelete(u32 slotId, vm::ptr<const SceAppUtilSaveDataMountPoint> mountPoint)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveDataSlotSetParam(u32 slotId, vm::ptr<const SceAppUtilSaveDataSlotParam> param, vm::ptr<const SceAppUtilSaveDataMountPoint> mountPoint)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveDataSlotGetParam(u32 slotId, vm::ptr<SceAppUtilSaveDataSlotParam> param, vm::ptr<const SceAppUtilSaveDataMountPoint> mountPoint)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveDataFileSave(vm::ptr<const SceAppUtilSaveDataFileSlot> slot, vm::ptr<const SceAppUtilSaveDataFile> files, u32 fileNum, vm::ptr<const SceAppUtilSaveDataMountPoint> mountPoint, vm::ptr<u32> requiredSizeKB)
{
	throw __FUNCTION__;
}

s32 sceAppUtilPhotoMount()
{
	throw __FUNCTION__;
}

s32 sceAppUtilPhotoUmount()
{
	throw __FUNCTION__;
}

s32 sceAppUtilSystemParamGetInt(u32 paramId, vm::ptr<s32> value)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSystemParamGetString(u32 paramId, vm::ptr<char> buf, u32 bufSize)
{
	throw __FUNCTION__;
}

s32 sceAppUtilSaveSafeMemory(vm::ptr<const void> buf, u32 bufSize, s64 offset)
{
	throw __FUNCTION__;
}

s32 sceAppUtilLoadSafeMemory(vm::ptr<void> buf, u32 bufSize, s64 offset)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAppUtil, #name, name)

psv_log_base sceAppUtil("SceAppUtil", []()
{
	sceAppUtil.on_load = nullptr;
	sceAppUtil.on_unload = nullptr;
	sceAppUtil.on_stop = nullptr;
	sceAppUtil.on_error = nullptr;

	REG_FUNC(0xDAFFE671, sceAppUtilInit);
	REG_FUNC(0xB220B00B, sceAppUtilShutdown);
	REG_FUNC(0x7E8FE96A, sceAppUtilSaveDataSlotCreate);
	REG_FUNC(0x266A7646, sceAppUtilSaveDataSlotDelete);
	REG_FUNC(0x98630136, sceAppUtilSaveDataSlotSetParam);
	REG_FUNC(0x93F0D89F, sceAppUtilSaveDataSlotGetParam);
	REG_FUNC(0x1E2A6158, sceAppUtilSaveDataFileSave);
	REG_FUNC(0xEE85804D, sceAppUtilPhotoMount);
	REG_FUNC(0x9651B941, sceAppUtilPhotoUmount);
	REG_FUNC(0x5DFB9CA0, sceAppUtilSystemParamGetInt);
	REG_FUNC(0x6E6AA267, sceAppUtilSystemParamGetString);
	REG_FUNC(0x9D8AC677, sceAppUtilSaveSafeMemory);
	REG_FUNC(0x3424D772, sceAppUtilLoadSafeMemory);
});
