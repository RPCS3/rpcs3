#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysutil.h"



LOG_CHANNEL(cellVideoExport);

struct CellVideoExportSetParam
{
	vm::bptr<char> title;
	vm::bptr<char> game_title;
	vm::bptr<char> game_comment;
	be_t<s32> editable;
	vm::bptr<void> reserved2;
};

using CellVideoExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);

error_code cellVideoExportProgress(vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, 0xFFFF, userdata); // 0-0xFFFF where 0xFFFF = 100%
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportInitialize2(u32 version, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportInitialize(u32 version, u32 container, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFromFileWithCopy(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellVideoExportSetParam> param, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportFromFileWithCopy(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellVideoExportSetParam> param, vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellVideoExportFinalize(vm::ptr<CellVideoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellVideoExport.todo("cellVideoExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellVideoExport)("cellVideoExportUtility", []()
{
	REG_FUNC(cellVideoExportUtility, cellVideoExportProgress);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize2);
	REG_FUNC(cellVideoExportUtility, cellVideoExportInitialize);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFileWithCopy);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFromFile);
	REG_FUNC(cellVideoExportUtility, cellVideoExportFinalize);
});
