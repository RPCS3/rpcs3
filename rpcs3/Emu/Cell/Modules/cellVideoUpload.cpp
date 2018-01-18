#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellVideoUpload.h"
#include "cellSysutil.h"

logs::channel cellVideoUpload("cellVideoUpload");

error_code cellVideoUploadInitialize(vm::cptr<CellVideoUploadParam> pParam, vm::ptr<CellVideoUploadCallback> cb, vm::ptr<void> userdata)
{
	cellVideoUpload.todo("cellVideoUploadInitialize(pParam=*0x%x, cb=*0x%x, userdata=*0x%x)", pParam, cb, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<char[]> pResultURL(128);

		cb(ppu, CELL_VIDEO_UPLOAD_STATUS_INITIALIZED, CELL_OK, pResultURL, userdata);
		cb(ppu, CELL_VIDEO_UPLOAD_STATUS_FINALIZED, CELL_OK, pResultURL, userdata);

		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVideoUpload)("cellVideoUpload", []()
{
	REG_FUNC(cellVideoUpload, cellVideoUploadInitialize);
});
