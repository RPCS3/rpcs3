#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellPhotoDecode_init();
Module cellPhotoDecode(0xf02e, cellPhotoDecode_init);

// Return Codes
enum
{
	CELL_PHOTO_DECODE_RET_OK             = 0,
	CELL_PHOTO_DECODE_RET_CANCEL         = 1,
	CELL_PHOTO_DECODE_ERROR_BUSY         = 0x8002c901,
	CELL_PHOTO_DECODE_ERROR_INTERNAL     = 0x8002c902,
	CELL_PHOTO_DECODE_ERROR_PARAM        = 0x8002c903,
	CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR = 0x8002c904,
	CELL_PHOTO_DECODE_ERROR_INITIALIZE   = 0x8002c905,
	CELL_PHOTO_DECODE_ERROR_DECODE       = 0x8002c906,
};

// Datatypes
struct CellPhotoDecodeSetParam
{ 
	u32 dstBuffer_addr;
	u16 width;
	u16 height;
};

struct CellPhotoDecodeReturnParam
{ 
	u16 width;
	u16 height;
};

// Functions
int cellPhotoDecodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

int cellPhotoDecodeInitialize2()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

int cellPhotoDecodeFinalize()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

int cellPhotoDecodeFromFile()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

void cellPhotoDecode_init()
{
	cellPhotoDecode.AddFunc(0x596f0a56, cellPhotoDecodeInitialize);
	cellPhotoDecode.AddFunc(0x0f424ecb, cellPhotoDecodeInitialize2);
	cellPhotoDecode.AddFunc(0xad7d8f38, cellPhotoDecodeFinalize);
	cellPhotoDecode.AddFunc(0x28b22e44, cellPhotoDecodeFromFile);
}