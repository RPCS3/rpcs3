#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellPngEnc;

// Error Codes
enum
{
	CELL_PNGENC_ERROR_ARG   = 0x80611291,
	CELL_PNGENC_ERROR_SEQ   = 0x80611292,
	CELL_PNGENC_ERROR_BUSY  = 0x80611293,
	CELL_PNGENC_ERROR_EMPTY = 0x80611294,
	CELL_PNGENC_ERROR_RESET = 0x80611295,
	CELL_PNGENC_ERROR_FATAL = 0x80611296,
};

s32 cellPngEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncOpen()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncClose()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

s32 cellPngEncReset()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

Module cellPngEnc("cellPngEnc", []()
{
	REG_FUNC(cellPngEnc, cellPngEncQueryAttr);
	REG_FUNC(cellPngEnc, cellPngEncOpen);
	REG_FUNC(cellPngEnc, cellPngEncOpenEx);
	REG_FUNC(cellPngEnc, cellPngEncClose);
	REG_FUNC(cellPngEnc, cellPngEncWaitForInput);
	REG_FUNC(cellPngEnc, cellPngEncEncodePicture);
	REG_FUNC(cellPngEnc, cellPngEncWaitForOutput);
	REG_FUNC(cellPngEnc, cellPngEncGetStreamInfo);
	REG_FUNC(cellPngEnc, cellPngEncReset);
});
