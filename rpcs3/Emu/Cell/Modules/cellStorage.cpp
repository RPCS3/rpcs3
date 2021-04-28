#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"
#include "cellStorage.h"

LOG_CHANNEL(cellSysutil);

template <>
void fmt_class_string<CellStorageError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_STORAGEDATA_ERROR_ACCESS_ERROR);
		STR_CASE(CELL_STORAGEDATA_ERROR_INTERNAL);
		STR_CASE(CELL_STORAGEDATA_ERROR_PARAM);
		STR_CASE(CELL_STORAGEDATA_ERROR_FAILURE);
		STR_CASE(CELL_STORAGEDATA_ERROR_BUSY);
		}

		return unknown;
	});
}

error_code cellStorageDataImportMove(u32 version, vm::ptr<char> srcMediaFile, vm::ptr<char> dstHddDir, vm::ptr<CellStorageDataSetParam> param, vm::ptr<CellStorageDataFinishCallback> funcFinish, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellStorageDataImportMove(version=0x%x, srcMediaFile=%s, dstHddDir=%s, param=*0x%x, funcFinish=*0x%x, container=0x%x, userdata=*0x%x)", version, srcMediaFile, dstHddDir, param, funcFinish, container, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellStorageDataImport(u32 version, vm::ptr<char> srcMediaFile, vm::ptr<char> dstHddDir, vm::ptr<CellStorageDataSetParam> param, vm::ptr<CellStorageDataFinishCallback> funcFinish, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellStorageDataImport(version=0x%x, srcMediaFile=%s, dstHddDir=%s, param=*0x%x, funcFinish=*0x%x, container=0x%x, userdata=*0x%x)", version, srcMediaFile, dstHddDir, param, funcFinish, container, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellStorageDataExport(u32 version, vm::ptr<char> srcHddFile, vm::ptr<char> dstMediaDir, vm::ptr<CellStorageDataSetParam> param, vm::ptr<CellStorageDataFinishCallback> funcFinish, u32 container, vm::ptr<void> userdata)
{
	cellSysutil.todo("cellStorageDataExport(version=0x%x, srcHddFile=%s, dstMediaDir=%s, param=*0x%x, funcFinish=*0x%x, container=0x%x, userdata=*0x%x)", version, srcHddFile, dstMediaDir, param, funcFinish, container, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

void cellSysutil_Storage_init()
{
	REG_FUNC(cellSysutil, cellStorageDataImportMove);
	REG_FUNC(cellSysutil, cellStorageDataImport);
	REG_FUNC(cellSysutil, cellStorageDataExport);
}
