#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"



LOG_CHANNEL(cellMusicExport);

// Return Codes
enum CellMusicExportError : u32
{
	CELL_MUSIC_EXPORT_UTIL_ERROR_BUSY         = 0x8002c601,
	CELL_MUSIC_EXPORT_UTIL_ERROR_INTERNAL     = 0x8002c602,
	CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM        = 0x8002c603,
	CELL_MUSIC_EXPORT_UTIL_ERROR_ACCESS_ERROR = 0x8002c604,
	CELL_MUSIC_EXPORT_UTIL_ERROR_DB_INTERNAL  = 0x8002c605,
	CELL_MUSIC_EXPORT_UTIL_ERROR_DB_REGIST    = 0x8002c606,
	CELL_MUSIC_EXPORT_UTIL_ERROR_SET_META     = 0x8002c607,
	CELL_MUSIC_EXPORT_UTIL_ERROR_FLUSH_META   = 0x8002c608,
	CELL_MUSIC_EXPORT_UTIL_ERROR_MOVE         = 0x8002c609,
	CELL_MUSIC_EXPORT_UTIL_ERROR_INITIALIZE   = 0x8002c60a,
};

template<>
void fmt_class_string<CellMusicExportError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_INTERNAL);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_DB_INTERNAL);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_DB_REGIST);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_SET_META);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_FLUSH_META);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_MOVE);
			STR_CASE(CELL_MUSIC_EXPORT_UTIL_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

struct CellMusicExportSetParam
{
	vm::bptr<char> title;
	vm::bptr<char> game_title;
	vm::bptr<char> artist;
	vm::bptr<char> genre;
	vm::bptr<char> game_comment;
	vm::bptr<void> reserved1;
	vm::bptr<void> reserved2;
};

using CellMusicExportUtilFinishCallback = void(s32 result, vm::ptr<void> userdata);

error_code cellMusicExportInitialize(u32 version, u32 container, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportInitialize(version=0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportInitialize2(u32 version, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportInitialize2(version=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportFinalize(vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellMusicExportSetParam> param, vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportFromFile(srcHddDir=%s, srcHddFile=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", srcHddDir, srcHddFile, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicExportProgress(vm::ptr<CellMusicExportUtilFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellMusicExport.todo("cellMusicExportProgress(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, 0xFFFF, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellMusicExport)("cellMusicExportUtility", []()
{
	REG_FUNC(cellMusicExportUtility, cellMusicExportInitialize);
	REG_FUNC(cellMusicExportUtility, cellMusicExportInitialize2);
	REG_FUNC(cellMusicExportUtility, cellMusicExportFinalize);
	REG_FUNC(cellMusicExportUtility, cellMusicExportFromFile);
	REG_FUNC(cellMusicExportUtility, cellMusicExportProgress);
});
