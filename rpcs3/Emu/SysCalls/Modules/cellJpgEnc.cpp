#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellJpgEnc;

// Error Codes
enum
{
	CELL_JPGENC_ERROR_ARG   = 0x80611191,
	CELL_JPGENC_ERROR_SEQ   = 0x80611192,
	CELL_JPGENC_ERROR_BUSY  = 0x80611193,
	CELL_JPGENC_ERROR_EMPTY = 0x80611194,
	CELL_JPGENC_ERROR_RESET = 0x80611195,
	CELL_JPGENC_ERROR_FATAL = 0x80611196,
};

s32 cellJpgEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpen()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncClose()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncEncodePicture2()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

s32 cellJpgEncReset()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

Module cellJpgEnc("cellJpgEnc", []()
{
	REG_FUNC(cellJpgEnc, cellJpgEncQueryAttr);
	REG_FUNC(cellJpgEnc, cellJpgEncOpen);
	REG_FUNC(cellJpgEnc, cellJpgEncOpenEx);
	REG_FUNC(cellJpgEnc, cellJpgEncClose);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForInput);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture);
	REG_FUNC(cellJpgEnc, cellJpgEncEncodePicture2);
	REG_FUNC(cellJpgEnc, cellJpgEncWaitForOutput);
	REG_FUNC(cellJpgEnc, cellJpgEncGetStreamInfo);
	REG_FUNC(cellJpgEnc, cellJpgEncReset);
});
