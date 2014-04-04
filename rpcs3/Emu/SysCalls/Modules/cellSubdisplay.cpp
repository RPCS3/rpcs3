#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSubdisplay_init();
Module cellSubdisplay(0x0034, cellSubdisplay_init);

// Return Codes
enum
{
	CELL_SUBDISPLAY_ERROR_OUT_OF_MEMORY    = 0x80029851,
	CELL_SUBDISPLAY_ERROR_FATAL            = 0x80029852,
	CELL_SUBDISPLAY_ERROR_NOT_FOUND        = 0x80029853,
	CELL_SUBDISPLAY_ERROR_INVALID_VALUE    = 0x80029854,
	CELL_SUBDISPLAY_ERROR_NOT_INITIALIZED  = 0x80029855,
	CELL_SUBDISPLAY_ERROR_SET_SAMPLE       = 0x80029860,
	CELL_SUBDISPLAY_ERROR_AUDIOOUT_IS_BUSY = 0x80029861,
	CELL_SUBDISPLAY_ERROR_ZERO_REGISTERED  = 0x80029813,
};

int cellSubDisplayInit()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayEnd()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayGetRequiredMemory()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayStart()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayStop()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayGetVideoBuffer()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayAudioOutBlocking()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayAudioOutNonBlocking()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayGetPeerNum()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

int cellSubDisplayGetPeerList()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

void cellSubdisplay_init()
{
	cellSubdisplay.AddFunc(0xf9a7e8a5, cellSubDisplayInit);
	cellSubdisplay.AddFunc(0x551d80a5, cellSubDisplayEnd);
	cellSubdisplay.AddFunc(0x6595ce22, cellSubDisplayGetRequiredMemory);
	cellSubdisplay.AddFunc(0xa5bccb47, cellSubDisplayStart);
	cellSubdisplay.AddFunc(0x6d85ddb3, cellSubDisplayStop);

	cellSubdisplay.AddFunc(0x938ac642, cellSubDisplayGetVideoBuffer);
	cellSubdisplay.AddFunc(0xaee1e0c2, cellSubDisplayAudioOutBlocking);
	cellSubdisplay.AddFunc(0x5468d6b0, cellSubDisplayAudioOutNonBlocking);

	cellSubdisplay.AddFunc(0x8a264d71, cellSubDisplayGetPeerNum);
	cellSubdisplay.AddFunc(0xe2485f79, cellSubDisplayGetPeerList);
}