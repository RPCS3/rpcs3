#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "scePhotoExport.h"

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
	throw __FUNCTION__;
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
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &scePhotoExport, #name, name)

psv_log_base scePhotoExport("ScePhotoExport", []()
{
	scePhotoExport.on_load = nullptr;
	scePhotoExport.on_unload = nullptr;
	scePhotoExport.on_stop = nullptr;
	scePhotoExport.on_error = nullptr;

	REG_FUNC(0x70512321, scePhotoExportFromData);
	REG_FUNC(0x84FD9FC5, scePhotoExportFromFile);
});
