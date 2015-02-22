#include "stdafx.h"
#if 0

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
}
#endif
