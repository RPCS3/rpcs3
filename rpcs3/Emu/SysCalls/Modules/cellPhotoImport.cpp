#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellPhotoImport_init();
Module cellPhotoImport(0xf02b, cellPhotoImport_init);

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
int _cellPhotoImport()
{
	UNIMPLEMENTED_FUNC(cellPhotoImport);
	return CELL_OK;
}

int _cellPhotoImport2()
{
	UNIMPLEMENTED_FUNC(cellPhotoImport);
	return CELL_OK;
}

void cellPhotoImport_init()
{
	cellPhotoImport.AddFunc(0x0783bce0, _cellPhotoImport);
	cellPhotoImport.AddFunc(0x1ab8df55, _cellPhotoImport2);
}