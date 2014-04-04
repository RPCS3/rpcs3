#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellMusicExport.AddFunc(0xb4c9b4f9, cellMusicExportInitialize);
	cellMusicExport.AddFunc(0xe0443a44, cellMusicExportInitialize2);
	cellMusicExport.AddFunc(0xe90effea, cellMusicExportFinalize);
	cellMusicExport.AddFunc(0xb202f0e8, cellMusicExportFromFile);
	cellMusicExport.AddFunc(0x92b50ebc, cellMusicExportProgress);
}