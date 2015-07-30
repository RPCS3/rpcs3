#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellPhotoImportUtil;

// Return Codes
enum
{
	CELL_PHOTO_IMPORT_RET_OK             = 0,
	CELL_PHOTO_IMPORT_RET_CANCEL         = 1,
	CELL_PHOTO_IMPORT_ERROR_BUSY         = 0x8002c701,
	CELL_PHOTO_IMPORT_ERROR_INTERNAL     = 0x8002c702,
	CELL_PHOTO_IMPORT_ERROR_PARAM        = 0x8002c703,
	CELL_PHOTO_IMPORT_ERROR_ACCESS_ERROR = 0x8002c704,
	CELL_PHOTO_IMPORT_ERROR_COPY         = 0x8002c705,
	CELL_PHOTO_IMPORT_ERROR_INITIALIZE   = 0x8002c706,
};

// Datatypes
struct CellPhotoImportFileDataSub
{ 
	int width;
	int height;
	//CellPhotoImportFormatType format; 
	//CellPhotoImportTexRot rotate; 
}; 

struct CellPhotoImportFileData
{ 
	char dstFileName;   //[CELL_FS_MAX_FS_FILE_NAME_LENGTH]; 
	char photo_title;   //[CELL_PHOTO_IMPORT_PHOTO_TITLE_MAX_LENGTH*3]; 
	char game_title;    //[CELL_PHOTO_IMPORT_GAME_TITLE_MAX_SIZE]; 
	char game_comment;  //[CELL_PHOTO_IMPORT_GAME_COMMENT_MAX_SIZE]; 
	CellPhotoImportFileDataSub* data_sub; 
};

struct CellPhotoImportSetParam
{ 
	unsigned int fileSizeMax;
};

// Functions
s32 cellPhotoImport()
{
	UNIMPLEMENTED_FUNC(cellPhotoImportUtil);
	return CELL_OK;
}

s32 cellPhotoImport2()
{
	UNIMPLEMENTED_FUNC(cellPhotoImportUtil);
	return CELL_OK;
}

Module cellPhotoImportUtil("cellPhotoImport", []()
{
	REG_FUNC(cellPhotoImportUtil, cellPhotoImport);
	REG_FUNC(cellPhotoImportUtil, cellPhotoImport2);
});
