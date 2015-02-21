#include "stdafx.h"
#if 0

void cellCelp8Enc_init();
Module cellCelp8Enc(0x0048, cellCelp8Enc_init);

// Return Codes
enum
{
	CELL_CELP8ENC_ERROR_FAILED      = 0x806140a1,
	CELL_CELP8ENC_ERROR_SEQ         = 0x806140a2,
	CELL_CELP8ENC_ERROR_ARG         = 0x806140a3,
	CELL_CELP8ENC_ERROR_CORE_FAILED = 0x806140b1,
	CELL_CELP8ENC_ERROR_CORE_SEQ    = 0x806140b2,
	CELL_CELP8ENC_ERROR_CORE_ARG    = 0x806140b3,
};

int cellCelp8EncQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncOpen()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncOpenEx()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncClose()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncStart()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncEnd()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncEncodeFrame()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncWaitForOutput()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

int cellCelp8EncGetAu()
{
	UNIMPLEMENTED_FUNC(cellCelp8Enc);
	return CELL_OK;
}

void cellCelp8Enc_init()
{
	REG_FUNC(cellCelp8Enc, cellCelp8EncQueryAttr);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpen);
	REG_FUNC(cellCelp8Enc, cellCelp8EncOpenEx);
	REG_FUNC(cellCelp8Enc, cellCelp8EncClose);
	REG_FUNC(cellCelp8Enc, cellCelp8EncStart);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEnd);
	REG_FUNC(cellCelp8Enc, cellCelp8EncEncodeFrame);
	REG_FUNC(cellCelp8Enc, cellCelp8EncWaitForOutput);
	REG_FUNC(cellCelp8Enc, cellCelp8EncGetAu);
}
#endif
