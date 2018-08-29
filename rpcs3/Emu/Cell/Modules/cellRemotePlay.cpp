#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellRemotePlay);

s32 cellRemotePlayGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlaySetComparativeVolume()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayGetPeerInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayGetSharedMemory()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayEncryptAllData()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayStopPeerVideoOut()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayGetComparativeVolume()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellRemotePlayBreak()
{
	fmt::throw_exception("Unimplemented" HERE);
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
