#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"



logs::channel cellCrossController("cellCrossController");

enum
{
	CELL_CROSS_CONTROLLER_STATUS_INITIALIZED = 1,
	CELL_CROSS_CONTROLLER_STATUS_FINALIZED = 2
};

struct CellCrossControllerParam
{
	vm::bcptr<char> pPackageFileName;
	vm::bcptr<char> pSignatureFileName;
	vm::bcptr<char> pIconFileName;
	vm::bptr<void> option;
};

struct CellCrossControllerPackageInfo
{
	vm::bcptr<char> pTitle;
	vm::bcptr<char> pTitleId;
	vm::bcptr<char> pAppVer;
};

using CellCrossControllerCallback = void(s32 status, s32 errorCode, vm::ptr<void> option, vm::ptr<void> userdata);

s32 cellCrossControllerInitialize(vm::cptr<CellCrossControllerParam> pParam, vm::cptr<CellCrossControllerPackageInfo> pPkgInfo, vm::ptr<CellCrossControllerCallback> cb, vm::ptr<void> userdata) // LittleBigPlanet 2 and 3
{
	cellCrossController.todo("cellCrossControllerInitialize(pParam=*0x%x, pPkgInfo=*0x%x, cb=*0x%x, userdata=*0x%x)", pParam, pPkgInfo, cb, userdata);

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
