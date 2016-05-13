#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellRemotePlay("cellRemotePlay", logs::level::notice);

s32 cellRemotePlayGetStatus()
{
	throw EXCEPTION("");
}

s32 cellRemotePlaySetComparativeVolume()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayGetPeerInfo()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayGetSharedMemory()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayEncryptAllData()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayStopPeerVideoOut()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayGetComparativeVolume()
{
	throw EXCEPTION("");
}

s32 cellRemotePlayBreak()
{
	throw EXCEPTION("");
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
