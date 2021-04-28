#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellVideoUpload.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellVideoUpload);

template<>
void fmt_class_string<CellVideoUploadError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_CANCEL);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_NETWORK);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_SERVICE_STOP);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_SERVICE_BUSY);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_SERVICE_UNAVAILABLE);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_SERVICE_QUOTA);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_ACCOUNT_STOP);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_FATAL);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_INVALID_VALUE);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_FILE_OPEN);
			STR_CASE(CELL_VIDEO_UPLOAD_ERROR_INVALID_STATE);
		}

		return unknown;
	});
}

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
