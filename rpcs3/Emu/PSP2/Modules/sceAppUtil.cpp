#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAppUtil.h"

logs::channel sceAppUtil("sceAppUtil");

s32 sceAppUtilInit(vm::cptr<SceAppUtilInitParam> initParam, vm::ptr<SceAppUtilBootParam> bootParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilShutdown()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveDataSlotCreate(u32 slotId, vm::cptr<SceAppUtilSaveDataSlotParam> param, vm::cptr<SceAppUtilSaveDataMountPoint> mountPoint)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveDataSlotDelete(u32 slotId, vm::cptr<SceAppUtilSaveDataMountPoint> mountPoint)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveDataSlotSetParam(u32 slotId, vm::cptr<SceAppUtilSaveDataSlotParam> param, vm::cptr<SceAppUtilSaveDataMountPoint> mountPoint)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveDataSlotGetParam(u32 slotId, vm::ptr<SceAppUtilSaveDataSlotParam> param, vm::cptr<SceAppUtilSaveDataMountPoint> mountPoint)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveDataFileSave(vm::cptr<SceAppUtilSaveDataFileSlot> slot, vm::cptr<SceAppUtilSaveDataFile> files, u32 fileNum, vm::cptr<SceAppUtilSaveDataMountPoint> mountPoint, vm::ptr<u32> requiredSizeKB)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilPhotoMount()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilPhotoUmount()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSystemParamGetInt(u32 paramId, vm::ptr<s32> value)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSystemParamGetString(u32 paramId, vm::ptr<char> buf, u32 bufSize)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilSaveSafeMemory(vm::cptr<void> buf, u32 bufSize, s64 offset)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceAppUtilLoadSafeMemory(vm::ptr<void> buf, u32 bufSize, s64 offset)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceAppUtil, nid, name)

DECLARE(arm_module_manager::SceAppUtil)("SceAppUtil", []()
{
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
