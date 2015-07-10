#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSubdisplay.h"

extern Module cellSubdisplay;

s32 cellSubDisplayInit()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayEnd()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayGetRequiredMemory(vm::ptr<CellSubDisplayParam> pParam)
{
	cellSubdisplay.Warning("cellSubDisplayGetRequiredMemory(pParam=*0x%x)", pParam);

	if (pParam->version == CELL_SUBDISPLAY_VERSION_0002)
	{
		return CELL_SUBDISPLAY_0002_MEMORY_CONTAINER_SIZE;
	}
	else
	{
		return CELL_SUBDISPLAY_0001_MEMORY_CONTAINER_SIZE;
	}
}

s32 cellSubDisplayStart()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayStop()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayGetVideoBuffer()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayAudioOutBlocking()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayAudioOutNonBlocking()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayGetPeerNum()
{
	UNIMPLEMENTED_FUNC(cellSubdisplay);
	return CELL_OK;
}

s32 cellSubDisplayGetPeerList()
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
