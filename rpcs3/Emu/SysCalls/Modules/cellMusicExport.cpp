#include "stdafx.h"
#if 0

void cellMusicExport_init();
Module cellMusicExport(0xf02c, cellMusicExport_init);

// Return Codes
enum
{
	CELL_MUSIC_EXPORT_UTIL_RET_OK             = 0,
	CELL_MUSIC_EXPORT_UTIL_RET_CANCEL         = 1,
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

int cellMusicExportInitialize()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

int cellMusicExportInitialize2()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

int cellMusicExportFinalize()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

int cellMusicExportFromFile()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

int cellMusicExportProgress()
{
	UNIMPLEMENTED_FUNC(cellMusicExport);
	return CELL_OK;
}

void cellMusicExport_init()
{
	REG_FUNC(cellMusicExport, cellMusicExportInitialize);
	REG_FUNC(cellMusicExport, cellMusicExportInitialize2);
	REG_FUNC(cellMusicExport, cellMusicExportFinalize);
	REG_FUNC(cellMusicExport, cellMusicExportFromFile);
	REG_FUNC(cellMusicExport, cellMusicExportProgress);
}
#endif
