#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellPngEnc_init();
Module cellPngEnc(0x0052, cellPngEnc_init);

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

int cellPngEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncOpen()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncClose()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

int cellPngEncReset()
{
	UNIMPLEMENTED_FUNC(cellPngEnc);
	return CELL_OK;
}

void cellPngEnc_init()
{
	cellPngEnc.AddFunc(0x496cfcd0, cellPngEncQueryAttr);
	cellPngEnc.AddFunc(0x19256dc5, cellPngEncOpen);
	cellPngEnc.AddFunc(0xc82558ce, cellPngEncOpenEx);
	cellPngEnc.AddFunc(0x117cd726, cellPngEncClose);
	cellPngEnc.AddFunc(0x662bd637, cellPngEncWaitForInput);
	cellPngEnc.AddFunc(0x5b546ca4, cellPngEncEncodePicture);
	cellPngEnc.AddFunc(0x90ef2963, cellPngEncWaitForOutput);
	cellPngEnc.AddFunc(0x585269bc, cellPngEncGetStreamInfo);
	cellPngEnc.AddFunc(0x6ac91de3, cellPngEncReset);
}