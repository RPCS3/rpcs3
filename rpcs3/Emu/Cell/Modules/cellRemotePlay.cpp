#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellRemotePlay);

s32 cellRemotePlayGetStatus()
{
	cellRemotePlay.todo("cellRemotePlayGetStatus()");
	return CELL_OK;
}

s32 cellRemotePlaySetComparativeVolume()
{
	cellRemotePlay.todo("cellRemotePlaySetComparativeVolume()");
	return CELL_OK;
}

s32 cellRemotePlayGetPeerInfo()
{
	cellRemotePlay.todo("cellRemotePlayGetPeerInfo()");
	return CELL_OK;
}

s32 cellRemotePlayGetSharedMemory()
{
	cellRemotePlay.todo("cellRemotePlayGetSharedMemory()");
	return CELL_OK;
}

s32 cellRemotePlayEncryptAllData()
{
	cellRemotePlay.todo("cellRemotePlayEncryptAllData()");
	return CELL_OK;
}

s32 cellRemotePlayStopPeerVideoOut()
{
	cellRemotePlay.todo("cellRemotePlayStopPeerVideoOut()");
	return CELL_OK;
}

s32 cellRemotePlayGetComparativeVolume()
{
	cellRemotePlay.todo("cellRemotePlayGetComparativeVolume()");
	return CELL_OK;
}

s32 cellRemotePlayBreak()
{
	cellRemotePlay.todo("cellRemotePlayBreak()");
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellRemotePlay)("cellRemotePlay", []()
{
	REG_FUNC(cellRemotePlay, cellRemotePlayGetStatus);
	REG_FUNC(cellRemotePlay, cellRemotePlaySetComparativeVolume);
	REG_FUNC(cellRemotePlay, cellRemotePlayGetPeerInfo);
	REG_FUNC(cellRemotePlay, cellRemotePlayGetSharedMemory);
	REG_FUNC(cellRemotePlay, cellRemotePlayEncryptAllData);
	REG_FUNC(cellRemotePlay, cellRemotePlayStopPeerVideoOut);
	REG_FUNC(cellRemotePlay, cellRemotePlayGetComparativeVolume);
	REG_FUNC(cellRemotePlay, cellRemotePlayBreak);
});
