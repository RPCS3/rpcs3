#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "scePhotoExport.h"

logs::channel scePhotoExport("scePhotoExport");

s32 scePhotoExportFromData(
	vm::cptr<void> photodata,
	s32 photodataSize,
	vm::cptr<ScePhotoExportParam> param,
	vm::ptr<void> workMemory,
	vm::ptr<ScePhotoExportCancelFunc> cancelFunc,
	vm::ptr<void> userdata,
	vm::ptr<char> exportedPath,
	s32 exportedPathLength)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoExportFromFile(
	vm::cptr<char> photodataPath,
	vm::cptr<ScePhotoExportParam> param,
	vm::ptr<void> workMemory,
	vm::ptr<ScePhotoExportCancelFunc> cancelFunc,
	vm::ptr<void> userdata,
	vm::ptr<char> exportedPath,
	s32 exportedPathLength)
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(libScePhotoExport, nid, name)

DECLARE(arm_module_manager::ScePhotoExport)("libScePhotoExport", []()
{
	REG_FUNC(0x70512321, scePhotoExportFromData);
	REG_FUNC(0x84FD9FC5, scePhotoExportFromFile);
});
