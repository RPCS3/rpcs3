#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
void cellPhotoUtility_init ();
Module cellPhotoUtility ("cellPhotoUtility", cellPhotoUtility_init);

//Error codes
enum
{
	CELL_PHOTO_EXPORT_UTIL_RET_OK				= 0, 
	CELL_PHOTO_EXPORT_UTIL_RET_CANCEL			= 1, 
	CELL_PHOTO_EXPORT_UTIL_ERROR_BUSY			= 0x8002c201, 
	CELL_PHOTO_EXPORT_UTIL_ERROR_INTERNAL		= 0x8002c202,  
	CELL_PHOTO_EXPORT_UTIL_ERROR_PARAM			= 0x8002c203,  
	CELL_PHOTO_EXPORT_UTIL_ERROR_ACCESS_ERROR	= 0x8002c204,
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_INTERNAL	= 0x8002c205, 
	CELL_PHOTO_EXPORT_UTIL_ERROR_DB_REGIST		= 0x8002c206,
	CELL_PHOTO_EXPORT_UTIL_ERROR_SET_META		= 0x8002c207,
	CELL_PHOTO_EXPORT_UTIL_ERROR_FLUSH_META		= 0x8002c208, 
	CELL_PHOTO_EXPORT_UTIL_ERROR_MOVE			= 0x8002c209, 
	CELL_PHOTO_EXPORT_UTIL_ERROR_INITIALIZE		= 0x8002c20a,  
};

//datatyps

struct CellPhotoExportSetParam
{ 
	char *photo_title; 
	char *game_title; 
	char *game_comment; 
	void *reserved; 
};


//export API
int cellPhotoExportInitialize ()
{
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

int cellPhotoExportInitialize2() //unsigned int version, CellPhotoExportUtilFinishCallback func, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

int cellPhotoExportFinalize () //CellPhotoExportUtilFinishCallback func, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

int cellPhotoExportFromFile () //const char *srcHddDir, const char *srcHddFile, CellPhotoExportSetParam *param, CellPhotoExportUtilFinishCallback func,void *userdata
{	
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

int cellPhotoExportFromFileWithCopy () //const char *srcHddDir, const char *srcHddFile, CellPhotoExportSetParam *param, CellPhotoExportUtilFinishCallback func, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

int cellPhotoExportProgress () //CellPhotoExportUtilFinishCallback func, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoUtility);
	return CELL_PHOTO_EXPORT_UTIL_RET_OK;
}

void ( * CellPhotoExportUtilFinishCallback ) (); //int result, void *userdata



void ( * CellPhotoImportFinishCallback ) (); //int result, CellPhotoImportFileData *filedata, void *userdata


void cellPhotoUtility_init ()
{
	cellPhotoUtility.AddFunc (0x08cbd8e1, cellPhotoExportInitialize2);
	cellPhotoUtility.AddFunc (0x09ce84ac, cellPhotoExportFromFile);
	//cellPhotoUtility.AddFunc (0x3f7fc0af, cellPhotoFinalize); //TODO
	//cellPhotoUtility.AddFunc (0x42a32983, cellPhotoRegistFromFile); //TODO
	cellPhotoUtility.AddFunc (0x4357c77f, cellPhotoExportInitialize); //TODO
	//cellPhotoUtility.AddFunc (0x55c70783, cellPhotoInitialize); //TODO
	cellPhotoUtility.AddFunc (0xde509ead, cellPhotoExportProgress);
	cellPhotoUtility.AddFunc (0xed4a0148, cellPhotoExportFinalize);
}