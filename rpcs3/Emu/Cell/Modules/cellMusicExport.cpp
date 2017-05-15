#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

namespace vm { using namespace ps3; }

logs::channel cellMusicExport("cellMusicExport");

// Return Codes
enum
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

s32 cellMusicExportInitialize()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

s32 cellMusicExportInitialize2()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

s32 cellMusicExportFinalize()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

s32 cellMusicExportFromFile()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

s32 cellMusicExportProgress()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
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
