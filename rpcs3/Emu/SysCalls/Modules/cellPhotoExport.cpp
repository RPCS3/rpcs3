#include "stdafx.h"
#if 0

void cellPhotoExport_init();
Module cellPhotoExport(0xf029, cellPhotoExport_init);

// Return Codes
enum
{
	CELL_PHOTO_EXPORT_UTIL_RET_OK             = 0,
	CELL_PHOTO_EXPORT_UTIL_RET_CANCEL         = 1,
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

int cellPhotoExportInitialize()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

int cellPhotoExportInitialize2()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

int cellPhotoExportFinalize()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

int cellPhotoExportFromFile()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

int cellPhotoExportProgress()
{
	UNIMPLEMENTED_FUNC(cellPhotoExport);
	return CELL_OK;
}

void cellPhotoExport_init()
{
	REG_FUNC(cellPhotoExport, cellPhotoExportInitialize);
	REG_FUNC(cellPhotoExport, cellPhotoExportInitialize2);
	REG_FUNC(cellPhotoExport, cellPhotoExportFinalize);
	REG_FUNC(cellPhotoExport, cellPhotoExportFromFile);
	//cellPhotoExport.AddFunc(, cellPhotoExportFromFileWithCopy);
	REG_FUNC(cellPhotoExport, cellPhotoExportProgress);
}
#endif
