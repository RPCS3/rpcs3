#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSubdisplay.h"

extern Module cellSubdisplay;

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
	cellSubdisplay.Warning("cellSubDisplayGetRequiredMemory(pParam_addr=0x%x)", pParam.addr());

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

Module cellSubdisplay("cellSubdisplay", []()
{
	REG_FUNC(cellSubdisplay, cellSubDisplayInit);
	REG_FUNC(cellSubdisplay, cellSubDisplayEnd);
	REG_FUNC(cellSubdisplay, cellSubDisplayGetRequiredMemory);
	REG_FUNC(cellSubdisplay, cellSubDisplayStart);
	REG_FUNC(cellSubdisplay, cellSubDisplayStop);

	REG_FUNC(cellSubdisplay, cellSubDisplayGetVideoBuffer);
	REG_FUNC(cellSubdisplay, cellSubDisplayAudioOutBlocking);
	REG_FUNC(cellSubdisplay, cellSubDisplayAudioOutNonBlocking);

	REG_FUNC(cellSubdisplay, cellSubDisplayGetPeerNum);
	REG_FUNC(cellSubdisplay, cellSubDisplayGetPeerList);
});
