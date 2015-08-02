#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellRemotePlay;

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


Module cellRemotePlay("cellRemotePlay", []()
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
