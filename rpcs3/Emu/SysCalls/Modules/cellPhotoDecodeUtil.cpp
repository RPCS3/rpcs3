#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
											
void cellPhotoDecodeUtil_init ();
Module cellPhotoDecodeUtil ("cellPhotoDecodeUtil", cellPhotoDecodeUtil_init);

//Error codes
enum
{
	CELL_PHOTO_DECODE_RET_OK					= 0, 
	CELL_PHOTO_DECODE_RET_CANCEL				= 1, 
	CELL_PHOTO_DECODE_ERROR_BUSY				= 0x8002c901,  
	CELL_PHOTO_DECODE_ERROR_INTERNAL			= 0x8002c902,  
	CELL_PHOTO_DECODE_ERROR_PARAM				= 0x8002c903,  
	CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR		= 0x8002c904,   
	CELL_PHOTO_DECODE_ERROR_INITIALIZE			= 0x8002c905,  
	CELL_PHOTO_DECODE_ERROR_DECODE				= 0x8002c906,
};

//datatypes
struct CellPhotoDecodeSetParam
{ 
	void *dstBuffer; 
	u16 width; 
	u16 height; 
	void *reserved1; 
	void *reserved2; 
};

struct CellPhotoDecodeReturnParam
{ 
	u16 width; 
	u16 height; 
	void *reserved1; 
	void *reserved2; 
};


int cellPhotoDecodeInitialize () //unsigned int version, sys_memory_container_t container1, sys_memory_container_t container2, CellPhotoDecodeFinishCallback funcFinish, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoDecodeUtil);
	return CELL_PHOTO_DECODE_RET_OK;
}

int cellPhotoDecodeInitialize2 () //unsigned int version, sys_memory_container_t container2, CellPhotoDecodeFinishCallback funcFinish, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoDecodeUtil);
	return CELL_PHOTO_DECODE_RET_OK;
}

int cellPhotoDecodeFinalize () //CellPhotoDecodeFinishCallback funcFinish, void *userdata
{
	UNIMPLEMENTED_FUNC(cellPhotoDecodeUtil);
	return CELL_PHOTO_DECODE_RET_OK;
}

int cellPhotoDecodeFromFile () //const char *srcHddDir, const char *srcHddFile, CellPhotoDecodeSetParam *set_param, CellPhotoDecodeReturnParam *return_param
{
	UNIMPLEMENTED_FUNC(cellPhotoDecodeUtil);
	return CELL_PHOTO_DECODE_RET_OK;
}

void ( *CellPhotoDecodeFinishCallback) (); //int result, void *userdata

void cellPhotoDecodeUtil_init ()
{
	cellPhotoDecodeUtil.AddFunc (0x0f424ecb, cellPhotoDecodeInitialize2);
	cellPhotoDecodeUtil.AddFunc (0x28b22e44, cellPhotoDecodeFromFile);
	cellPhotoDecodeUtil.AddFunc (0x596f0a56, cellPhotoDecodeInitialize);
	cellPhotoDecodeUtil.AddFunc (0xad7d8f38, cellPhotoDecodeFinalize);
}
