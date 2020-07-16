#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"



LOG_CHANNEL(cellPhotoExport);

// Return Codes
enum CellPhotoExportError : u32
{
	CELL_PHOTO_EXPORT_UTIL_ERROR_BUSY         = 0x8002c201,
	CELL_PHOTO_EXPORT_UTIL_ERROR_INTERNAL     = 0x8002c202,
	CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM        = 0x8002c203,
	CELL_PHOTO_EXPORT_UTIL_ERROR_ACCESS_ERROR = 0x8002c204,
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_INTERNAL  = 0x8002c205,
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_REGIST    = 0x8002c206,
	CELL_PHOTO_EXPORT_UTIL_ERROR_SET_META     = 0x8002c207,
	CELL_PHOTO_EXPORT_UTIL_ERROR_FLUSH_META   = 0x8002c208,
	CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE         = 0x8002c209,
	CELL_PHOTO_EXPORT_UTIL_ERROR_INITIALIZE   = 0x8002c20a,
};

template<>
void fmt_class_string<CellPhotoExportError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_BUSY);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_INTERNAL);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_DB_INTERNAL);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_DB_REGIST);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_SET_META);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_FLUSH_META);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE);
			STR_CASE(CELL_PHOTO_EXPORT_UTIL_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

struct CellPhotoExportSetParam
{
	vm::bptr<char> photo_title;
	vm::bptr<char> game_title;
	vm::bptr<char> game_comment;
	vm::bptr<void> reserved;
};

using CellPhotoExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);

error_code cellPhotoInitialize()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

error_code cellPhotoFinalize()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

error_code cellPhotoRegistFromFile()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

error_code cellPhotoExportInitialize(u32 version, u32 container, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportInitialize2(u32 version, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFinalize(vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoExportSetParam> param, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportFromFileWithCopy(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoExportSetParam> param, vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportFromFileWithCopy(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoExportProgress(vm::ptr<CellPhotoExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoExport.todo("cellPhotoExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, 0xFFFF, userdata); // 0-0xFFFF where 0xFFFF = 100%
		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPhotoExport)("cellPhotoUtility", []()
{
	REG_FUNC(cellPhotoUtility, cellPhotoInitialize);
	REG_FUNC(cellPhotoUtility, cellPhotoFinalize);
	REG_FUNC(cellPhotoUtility, cellPhotoRegistFromFile);
	REG_FUNC(cellPhotoUtility, cellPhotoExportInitialize);
	REG_FUNC(cellPhotoUtility, cellPhotoExportInitialize2);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFinalize);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFromFile);
	REG_FUNC(cellPhotoUtility, cellPhotoExportFromFileWithCopy);
	REG_FUNC(cellPhotoUtility, cellPhotoExportProgress);
});
