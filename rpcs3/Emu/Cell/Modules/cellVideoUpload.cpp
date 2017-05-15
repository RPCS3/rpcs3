#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellVideoUpload.h"

logs::channel cellVideoUpload("cellVideoUpload");

s32 cellVideoUploadInitialize(vm::cptr<CellVideoUploadParam> pParam, vm::ptr<CellVideoUploadCallback> cb, vm::ptr<void> userdata)
{
	cellVideoUpload.todo("cellVideoUploadInitialize(pParam=*0x%x, cb=*0x%x, userdata=*0x%x)", pParam, cb, userdata);

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVideoUpload)("cellVideoUpload", []()
{
	REG_FUNC(cellVideoUpload, cellVideoUploadInitialize);
});
