#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "cellVoice.h"

LOG_CHANNEL(cellVoice);


s32 cellVoiceConnectIPortToOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceCreateNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceCreatePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceDeletePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceDisconnectIPortFromOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceEnd()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetBitRate()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetMuteFlag()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetPortAttr()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetPortInfo()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetSignalState()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceGetVolume()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceInit()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceInitEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoicePausePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoicePausePortAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceResetPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceResumePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceResumePortAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetBitRate()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetMuteFlag()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetMuteFlagAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetPortAttr()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceSetVolume()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceStart()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceStartEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceStop()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceUpdatePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceWriteToIPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceWriteToIPortEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceWriteToIPortEx2()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceReadFromOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

s32 cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVoice)("cellVoice", []()
{
	REG_FUNC(cellVoice, cellVoiceConnectIPortToOPort);
	REG_FUNC(cellVoice, cellVoiceCreateNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceCreatePort);
	REG_FUNC(cellVoice, cellVoiceDeletePort);
	REG_FUNC(cellVoice, cellVoiceDisconnectIPortFromOPort);
	REG_FUNC(cellVoice, cellVoiceEnd);
	REG_FUNC(cellVoice, cellVoiceGetBitRate);
	REG_FUNC(cellVoice, cellVoiceGetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceGetPortAttr);
	REG_FUNC(cellVoice, cellVoiceGetPortInfo);
	REG_FUNC(cellVoice, cellVoiceGetSignalState);
	REG_FUNC(cellVoice, cellVoiceGetVolume);
	REG_FUNC(cellVoice, cellVoiceInit);
	REG_FUNC(cellVoice, cellVoiceInitEx);
	REG_FUNC(cellVoice, cellVoicePausePort);
	REG_FUNC(cellVoice, cellVoicePausePortAll);
	REG_FUNC(cellVoice, cellVoiceRemoveNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceResetPort);
	REG_FUNC(cellVoice, cellVoiceResumePort);
	REG_FUNC(cellVoice, cellVoiceResumePortAll);
	REG_FUNC(cellVoice, cellVoiceSetBitRate);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlagAll);
	REG_FUNC(cellVoice, cellVoiceSetNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceSetPortAttr);
	REG_FUNC(cellVoice, cellVoiceSetVolume);
	REG_FUNC(cellVoice, cellVoiceStart);
	REG_FUNC(cellVoice, cellVoiceStartEx);
	REG_FUNC(cellVoice, cellVoiceStop);
	REG_FUNC(cellVoice, cellVoiceUpdatePort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx2);
	REG_FUNC(cellVoice, cellVoiceReadFromOPort);
	REG_FUNC(cellVoice, cellVoiceDebugTopology);
});
