#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"
#include "cellCrossController.h"


LOG_CHANNEL(cellCrossController);

template <>
void fmt_class_string<CellCrossControllerError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCrossControllerError value)
	{
		switch (value)
		{
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_CANCEL);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_NETWORK);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_OUT_OF_MEMORY);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_FATAL);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_SIG_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_ICON_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_PKG_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_SIG_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_ICON_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_STATE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INTERNAL);
		}

		return unknown;
	});
}

error_code cellCrossControllerInitialize(vm::cptr<CellCrossControllerParam> pParam, vm::cptr<CellCrossControllerPackageInfo> pPkgInfo, vm::ptr<CellCrossControllerCallback> cb, vm::ptr<void> userdata) // LittleBigPlanet 2 and 3
{
	cellCrossController.todo("cellCrossControllerInitialize(pParam=*0x%x, pPkgInfo=*0x%x, cb=*0x%x, userdata=*0x%x)", pParam, pPkgInfo, cb, userdata);

	// TODO
	//if (something)
	//{
	//	return CELL_CROSS_CONTROLLER_ERROR_INVALID_STATE;
	//}

	if (!pParam || !pPkgInfo)
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE;
	}

	// Check if the strings exceed the allowed size (not counting null terminators)

	if (!pParam->pPackageFileName || !memchr(pParam->pPackageFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILENAME;
	}

	if (!pParam->pSignatureFileName || !memchr(pParam->pSignatureFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_SIG_FILENAME;
	}

	if (!pParam->pIconFileName || !memchr(pParam->pIconFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_ICON_FILENAME;
	}

	if (!pPkgInfo->pAppVer || !memchr(pPkgInfo->pAppVer.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_APP_VER_LEN + 1) ||
	    !pPkgInfo->pTitleId || !memchr(pPkgInfo->pTitleId.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_TITLE_ID_LEN + 1) ||
	    !pPkgInfo->pTitle || !memchr(pPkgInfo->pTitle.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_TITLE_LEN + 1) ||
	    !cb)
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb(ppu, CELL_CROSS_CONTROLLER_STATUS_INITIALIZED, CELL_OK, vm::null, userdata);
		cb(ppu, CELL_CROSS_CONTROLLER_STATUS_FINALIZED, CELL_OK, vm::null, userdata);

		return CELL_OK;
	});

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellCrossController)("cellCrossController", []()
{
	REG_FUNC(cellCrossController, cellCrossControllerInitialize);
});
