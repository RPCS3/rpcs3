#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellRemotePlay.h"

LOG_CHANNEL(cellRemotePlay);

template <>
void fmt_class_string<CellRemotePlayError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellRemotePlayError value)
	{
		switch (value)
		{
		STR_CASE(CELL_REMOTEPLAY_ERROR_INTERNAL);
		}

		return unknown;
	});
}

error_code cellRemotePlayGetStatus()
{
	cellRemotePlay.todo("cellRemotePlayGetStatus()");
	return CELL_OK;
}

error_code cellRemotePlaySetComparativeVolume(f32 comparativeAudioVolume)
{
	cellRemotePlay.todo("cellRemotePlaySetComparativeVolume(comparativeAudioVolume=%f)", comparativeAudioVolume);
	return CELL_OK;
}

error_code cellRemotePlayGetPeerInfo()
{
	cellRemotePlay.todo("cellRemotePlayGetPeerInfo()");
	return CELL_OK;
}

error_code cellRemotePlayGetSharedMemory()
{
	cellRemotePlay.todo("cellRemotePlayGetSharedMemory()");
	return CELL_OK;
}

error_code cellRemotePlayEncryptAllData()
{
	cellRemotePlay.todo("cellRemotePlayEncryptAllData()");
	return CELL_OK;
}

error_code cellRemotePlayStopPeerVideoOut()
{
	cellRemotePlay.todo("cellRemotePlayStopPeerVideoOut()");
	return CELL_OK;
}

error_code cellRemotePlayGetComparativeVolume(vm::ptr<f32> pComparativeAudioVolume)
{
	cellRemotePlay.todo("cellRemotePlayGetComparativeVolume(pComparativeAudioVolume=*0x%x)", pComparativeAudioVolume);

	if (pComparativeAudioVolume)
	{
		*pComparativeAudioVolume = 1.f;
	}

	return CELL_OK;
}

error_code cellRemotePlayBreak()
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
