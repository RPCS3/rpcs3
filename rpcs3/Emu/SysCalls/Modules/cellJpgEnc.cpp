#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellJpgEnc_init();
Module cellJpgEnc(0x003d, cellJpgEnc_init);

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

int cellJpgEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncOpen()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncClose()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncWaitForInput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncEncodePicture()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncEncodePicture2()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

int cellJpgEncReset()
{
	UNIMPLEMENTED_FUNC(cellJpgEnc);
	return CELL_OK;
}

void cellJpgEnc_init()
{
	cellJpgEnc.AddFunc(0x12d9b6c5, cellJpgEncQueryAttr);
	cellJpgEnc.AddFunc(0xa4bfae51, cellJpgEncOpen);
	cellJpgEnc.AddFunc(0x6f2d371c, cellJpgEncOpenEx);
	cellJpgEnc.AddFunc(0x969fc5f7, cellJpgEncClose);
	cellJpgEnc.AddFunc(0x2ae79be8, cellJpgEncWaitForInput);
	cellJpgEnc.AddFunc(0xa9e81214, cellJpgEncEncodePicture);
	cellJpgEnc.AddFunc(0x636dc89e, cellJpgEncEncodePicture2);
	cellJpgEnc.AddFunc(0x9b4e3a74, cellJpgEncWaitForOutput);
	cellJpgEnc.AddFunc(0x4262e880, cellJpgEncGetStreamInfo);
	cellJpgEnc.AddFunc(0x0cf2b78b, cellJpgEncReset);
}