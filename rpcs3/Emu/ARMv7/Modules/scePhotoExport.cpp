#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base scePhotoExport;

#define REG_FUNC(nid, name) reg_psv_func(nid, &scePhotoExport, #name, name)

psv_log_base scePhotoExport("ScePhotoExport", []()
{
	scePhotoExport.on_load = nullptr;
	scePhotoExport.on_unload = nullptr;
	scePhotoExport.on_stop = nullptr;

	//REG_FUNC(0x70512321, scePhotoExportFromData);
	//REG_FUNC(0x84FD9FC5, scePhotoExportFromFile);
});
