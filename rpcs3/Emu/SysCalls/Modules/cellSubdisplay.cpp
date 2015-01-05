#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSubdisplay.h"

Module *cellSubdisplay = nullptr;

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

int cellSubDisplayGetRequiredMemory(vm::ptr<CellSubDisplayParam> pParam)
{
	cellSubdisplay->Warning("cellSubDisplayGetRequiredMemory(pParam_addr=0x%x)", pParam.addr());

	if (pParam->version == CELL_SUBDISPLAY_VERSION_0002)
	{
		return CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE;
	}
	else
	{
		return CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE;
	}
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

void cellSubdisplay_init(Module *pxThis)
{
	cellSubdisplay = pxThis;

	cellSubdisplay->AddFunc(0xf9a7e8a5, cellSubDisplayInit);
	cellSubdisplay->AddFunc(0x551d80a5, cellSubDisplayEnd);
	cellSubdisplay->AddFunc(0x6595ce22, cellSubDisplayGetRequiredMemory);
	cellSubdisplay->AddFunc(0xa5bccb47, cellSubDisplayStart);
	cellSubdisplay->AddFunc(0x6d85ddb3, cellSubDisplayStop);

	cellSubdisplay->AddFunc(0x938ac642, cellSubDisplayGetVideoBuffer);
	cellSubdisplay->AddFunc(0xaee1e0c2, cellSubDisplayAudioOutBlocking);
	cellSubdisplay->AddFunc(0x5468d6b0, cellSubDisplayAudioOutNonBlocking);

	cellSubdisplay->AddFunc(0x8a264d71, cellSubDisplayGetPeerNum);
	cellSubdisplay->AddFunc(0xe2485f79, cellSubDisplayGetPeerList);
}
