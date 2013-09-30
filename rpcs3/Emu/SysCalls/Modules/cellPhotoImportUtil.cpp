#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
void cellPhotoImportUtil_init ();
Module cellPhotoImportUtil ("cellPhotoImportUtil", cellPhotoImportUtil_init);

//Error codes
enum
{
	CELL_PHOTO_IMPORT_RET_OK					= 0,
	CELL_PHOTO_IMPORT_RET_CANCEL				= 1,
	CELL_PHOTO_IMPORT_ERROR_BUSY				= 0x8002c701,
	CELL_PHOTO_IMPORT_ERROR_INTERNAL			= 0x8002c702,
	CELL_PHOTO_IMPORT_ERROR_PARAM				= 0x8002c703,
	CELL_PHOTO_IMPORT_ERROR_ACCESS_ERROR		= 0x8002c704,
	CELL_PHOTO_IMPORT_ERROR_COPY				= 0x8002c705,
	CELL_PHOTO_IMPORT_ERROR_INITIALIZE			= 0x8002c706,

};

//datatyps

struct CellPhotoImportFileDataSub
{ 
int width; 
int height; 
//CellPhotoImportFormatType format; 
//CellPhotoImportTexRot rotate; 
}; 

struct CellPhotoImportFileData
{ 
char dstFileName; //[CELL_FS_MAX_FS_FILE_NAME_LENGTH]; 
char photo_title; //[CELL_PHOTO_IMPORT_PHOTO_TITLE_MAX_LENGTH*3]; 
char game_title; //[CELL_PHOTO_IMPORT_GAME_TITLE_MAX_SIZE]; 
char game_comment; //[CELL_PHOTO_IMPORT_GAME_COMMENT_MAX_SIZE]; 
CellPhotoImportFileDataSub* data_sub; 
void *reserved; 
};

struct CellPhotoImportSetParam
{ 
	unsigned int fileSizeMax; 
	void *reserved1; 
	void *reserved2; 
};

//import API

int cellPhotoImport () //unsigned int version, const char *dstHddPath, CellPhotoImportSetParam *param, sys_memory_container_t container, CellPhotoImportFinishCallback funcFinish, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoImportUtil);
	return CELL_PHOTO_IMPORT_RET_OK;
}

int cellPhotoImport2 () //unsigned int version, const char *dstHddPath, CellPhotoImportSetParam *param, CellPhotoImportFinishCallback funcFinish, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoImportUtil);
	return CELL_PHOTO_IMPORT_RET_OK;
}


void cellPhotoImportUtil_init ()
{
	cellPhotoImportUtil.AddFunc (0x0783bce0, cellPhotoImport);
	cellPhotoImportUtil.AddFunc (0x1ab8df55, cellPhotoImport2);
}