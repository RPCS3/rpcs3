#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellAtrac_init();
Module cellAtrac(0x0013, cellAtrac_init);

#include "cellAtrac.h"

int cellAtracSetDataAndGetMemSize()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracCreateDecoder()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracCreateDecoderExt()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracDeleteDecoder()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracDecode()
{
	//UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetStreamDataInfo()
{
	//UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracAddStreamData()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetRemainFrame()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetVacantSize()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracIsSecondBufferNeeded()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetSecondBufferInfo()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracSetSecondBuffer()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetChannel()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetMaxSample()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetNextSample()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetSoundInfo()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetNextDecodePosition()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetBitrate()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetLoopInfo()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracSetLoopNum()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetBufferInfoForResetting()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracResetPlayPosition()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetInternalErrorInfo()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

void cellAtrac_init()
{
	cellAtrac.AddFunc(0x66afc68e, cellAtracSetDataAndGetMemSize);

	cellAtrac.AddFunc(0xfa293e88, cellAtracCreateDecoder);
	cellAtrac.AddFunc(0x2642d4cc, cellAtracCreateDecoderExt);
	cellAtrac.AddFunc(0x761cb9be, cellAtracDeleteDecoder);

	cellAtrac.AddFunc(0x8eb0e65f, cellAtracDecode);

	cellAtrac.AddFunc(0x2bfff084, cellAtracGetStreamDataInfo);
	cellAtrac.AddFunc(0x46cfc013, cellAtracAddStreamData);
	cellAtrac.AddFunc(0xdfab73aa, cellAtracGetRemainFrame);
	cellAtrac.AddFunc(0xc9a95fcb, cellAtracGetVacantSize);
	cellAtrac.AddFunc(0x99efe171, cellAtracIsSecondBufferNeeded);
	cellAtrac.AddFunc(0xbe07f05e, cellAtracGetSecondBufferInfo);
	cellAtrac.AddFunc(0x06ddb53e, cellAtracSetSecondBuffer);

	cellAtrac.AddFunc(0x0f9667b6, cellAtracGetChannel);
	cellAtrac.AddFunc(0x5f62d546, cellAtracGetMaxSample);
	cellAtrac.AddFunc(0x4797d1ff, cellAtracGetNextSample);
	cellAtrac.AddFunc(0xcf01d5d4, cellAtracGetSoundInfo);
	cellAtrac.AddFunc(0x7b22e672, cellAtracGetNextDecodePosition);
	cellAtrac.AddFunc(0x006016da, cellAtracGetBitrate);

	cellAtrac.AddFunc(0xab6b6dbf, cellAtracGetLoopInfo);
	cellAtrac.AddFunc(0x78ba5c41, cellAtracSetLoopNum);

	cellAtrac.AddFunc(0x99fb73d1, cellAtracGetBufferInfoForResetting);
	cellAtrac.AddFunc(0x7772eb2b, cellAtracResetPlayPosition);

	cellAtrac.AddFunc(0xb5c11938, cellAtracGetInternalErrorInfo);
}