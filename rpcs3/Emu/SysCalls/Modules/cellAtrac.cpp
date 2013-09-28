#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellAtrac_init();
Module cellAtrac(0x0013, cellAtrac_init);

// Return Codes
enum
{
	CELL_ATRAC_OK									= 0x00000000,
	CELL_ATRAC_ERROR_API_FAIL						= 0x80610301,
	CELL_ATRAC_ERROR_READSIZE_OVER_BUFFER			= 0x80610311,
	CELL_ATRAC_ERROR_UNKNOWN_FORMAT					= 0x80610312,
	CELL_ATRAC_ERROR_READSIZE_IS_TOO_SMALL			= 0x80610313,
	CELL_ATRAC_ERROR_ILLEGAL_SAMPLING_RATE			= 0x80610314,
	CELL_ATRAC_ERROR_ILLEGAL_DATA					= 0x80610315,
	CELL_ATRAC_ERROR_NO_DECODER						= 0x80610321,
	CELL_ATRAC_ERROR_UNSET_DATA						= 0x80610322,
	CELL_ATRAC_ERROR_DECODER_WAS_CREATED			= 0x80610323,
	CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED			= 0x80610331,
	CELL_ATRAC_ERROR_NODATA_IN_BUFFER				= 0x80610332,
	CELL_ATRAC_ERROR_NOT_ALIGNED_OUT_BUFFER			= 0x80610333,
	CELL_ATRAC_ERROR_NEED_SECOND_BUFFER				= 0x80610334,
	CELL_ATRAC_ERROR_ALLDATA_IS_ONMEMORY			= 0x80610341,
	CELL_ATRAC_ERROR_ADD_DATA_IS_TOO_BIG			= 0x80610342,
	CELL_ATRAC_ERROR_NONEED_SECOND_BUFFER			= 0x80610351,
	CELL_ATRAC_ERROR_UNSET_LOOP_NUM					= 0x80610361,
	CELL_ATRAC_ERROR_ILLEGAL_SAMPLE					= 0x80610371,
	CELL_ATRAC_ERROR_ILLEGAL_RESET_BYTE				= 0x80610372,
	CELL_ATRAC_ERROR_ILLEGAL_PPU_THREAD_PRIORITY	= 0x80610381,
	CELL_ATRAC_ERROR_ILLEGAL_SPU_THREAD_PRIORITY	= 0x80610382,
};

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
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

int cellAtracGetStreamDataInfo()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
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