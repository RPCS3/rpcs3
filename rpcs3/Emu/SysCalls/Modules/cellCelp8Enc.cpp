#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellCelp8Enc.AddFunc(0x2d677e0c, cellCelp8EncQueryAttr);
	cellCelp8Enc.AddFunc(0x2eb6efee, cellCelp8EncOpen);
	cellCelp8Enc.AddFunc(0xcd48ad62, cellCelp8EncOpenEx);
	cellCelp8Enc.AddFunc(0xfd2566b4, cellCelp8EncClose);
	cellCelp8Enc.AddFunc(0x0f6ab57b, cellCelp8EncStart);
	cellCelp8Enc.AddFunc(0xbbbc2c1c, cellCelp8EncEnd);
	cellCelp8Enc.AddFunc(0x2099f86e, cellCelp8EncEncodeFrame);
	cellCelp8Enc.AddFunc(0x29da1ea6, cellCelp8EncWaitForOutput);
	cellCelp8Enc.AddFunc(0x48c5020d, cellCelp8EncGetAu);
}