#include "stdafx.h"
#if 0

void cellCelpEnc_init();
Module cellCelpEnc(0xf00a, cellCelpEnc_init);

// Return Codes
enum
{
	CELL_CELPENC_ERROR_FAILED      = 0x80614001,
	CELL_CELPENC_ERROR_SEQ         = 0x80614002,
	CELL_CELPENC_ERROR_ARG         = 0x80614003,
	CELL_CELPENC_ERROR_CORE_FAILED = 0x80614081,
	CELL_CELPENC_ERROR_CORE_SEQ    = 0x80614082,
	CELL_CELPENC_ERROR_CORE_ARG    = 0x80614083,
};

int cellCelpEncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncOpen()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncClose()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncStart()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncEnd()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncEncodeFrame()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

int cellCelpEncGetAu()
{
	UNIMPLEMENTED_FUNC(cellCelpEnc);
	return CELL_OK;
}

void cellCelpEnc_init()
{
	REG_FUNC(cellCelpEnc, cellCelpEncQueryAttr);
	REG_FUNC(cellCelpEnc, cellCelpEncOpen);
	REG_FUNC(cellCelpEnc, cellCelpEncOpenEx);
	REG_FUNC(cellCelpEnc, cellCelpEncClose);
	REG_FUNC(cellCelpEnc, cellCelpEncStart);
	REG_FUNC(cellCelpEnc, cellCelpEncEnd);
	REG_FUNC(cellCelpEnc, cellCelpEncEncodeFrame);
	REG_FUNC(cellCelpEnc, cellCelpEncWaitForOutput);
	REG_FUNC(cellCelpEnc, cellCelpEncGetAu);
}
#endif 
