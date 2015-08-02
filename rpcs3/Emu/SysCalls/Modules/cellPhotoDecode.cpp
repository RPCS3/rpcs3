#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellPhotoDecode;

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
s32 cellPhotoDecodeInitialize()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

s32 cellPhotoDecodeInitialize2()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

s32 cellPhotoDecodeFinalize()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

s32 cellPhotoDecodeFromFile()
{
	UNIMPLEMENTED_FUNC(cellPhotoDecode);
	return CELL_OK;
}

Module cellPhotoDecode("cellPhotoDecode", []()
{
	REG_FUNC(cellPhotoDecode, cellPhotoDecodeInitialize);
	REG_FUNC(cellPhotoDecode, cellPhotoDecodeInitialize2);
	REG_FUNC(cellPhotoDecode, cellPhotoDecodeFinalize);
	REG_FUNC(cellPhotoDecode, cellPhotoDecodeFromFile);
});
