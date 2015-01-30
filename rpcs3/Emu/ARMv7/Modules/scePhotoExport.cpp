#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base scePhotoExport;

struct ScePhotoExportParam
{
	u32 version;
	vm::psv::ptr<const char> photoTitle;
	vm::psv::ptr<const char> gameTitle;
	vm::psv::ptr<const char> gameComment;
	char reserved[32];
};

typedef vm::psv::ptr<s32(vm::psv::ptr<void>)> ScePhotoExportCancelFunc;

s32 scePhotoExportFromData(
	vm::psv::ptr<const void> photodata,
	s32 photodataSize,
	vm::psv::ptr<const ScePhotoExportParam> param,
	vm::psv::ptr<void> workMemory,
	ScePhotoExportCancelFunc cancelFunc,
	vm::psv::ptr<void> userdata,
	vm::psv::ptr<char> exportedPath,
	s32 exportedPathLength)
{
	throw __FUNCTION__;
}

s32 scePhotoExportFromFile(
	vm::psv::ptr<const char> photodataPath,
	vm::psv::ptr<const ScePhotoExportParam> param,
	vm::psv::ptr<void> workMemory,
	ScePhotoExportCancelFunc cancelFunc,
	vm::psv::ptr<void> userdata,
	vm::psv::ptr<char> exportedPath,
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

	REG_FUNC(0x70512321, scePhotoExportFromData);
	REG_FUNC(0x84FD9FC5, scePhotoExportFromFile);
});
