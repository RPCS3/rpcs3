#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellCelpEnc.AddFunc(0x6b148570, cellCelpEncQueryAttr);
	cellCelpEnc.AddFunc(0x77b3b29a, cellCelpEncOpen);
	cellCelpEnc.AddFunc(0x9eb084db, cellCelpEncOpenEx);
	cellCelpEnc.AddFunc(0x15ec0cca, cellCelpEncClose);
	cellCelpEnc.AddFunc(0x55dc23de, cellCelpEncStart);
	cellCelpEnc.AddFunc(0xf2b85dff, cellCelpEncEnd);
	cellCelpEnc.AddFunc(0x81fe030c, cellCelpEncEncodeFrame);
	cellCelpEnc.AddFunc(0x9b244272, cellCelpEncWaitForOutput);
	cellCelpEnc.AddFunc(0x3773692f, cellCelpEncGetAu);
}