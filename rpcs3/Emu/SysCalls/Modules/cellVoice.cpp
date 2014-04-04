#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellVoice_init();
Module cellVoice(0x0046, cellVoice_init);

// Error Codes
enum
{
	CELL_VOICE_ERROR_ADDRESS_INVALID       = 0x8031080a,
	CELL_VOICE_ERROR_ARGUMENT_INVALID      = 0x80310805,
	CELL_VOICE_ERROR_CONTAINER_INVALID     = 0x80310806,
	CELL_VOICE_ERROR_DEVICE_NOT_PRESENT    = 0x80310812,
	CELL_VOICE_ERROR_EVENT_DISPATCH        = 0x80310811,
	CELL_VOICE_ERROR_EVENT_QUEUE           = 0x8031080f,
	CELL_VOICE_ERROR_GENERAL               = 0x80310803,
	CELL_VOICE_ERROR_LIBVOICE_INITIALIZED  = 0x80310802,
	CELL_VOICE_ERROR_LIBVOICE_NOT_INIT     = 0x80310801,
	CELL_VOICE_ERROR_NOT_IMPLEMENTED       = 0x80310809,
	CELL_VOICE_ERROR_PORT_INVALID          = 0x80310804,
	CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT = 0x80310808,
	CELL_VOICE_ERROR_SERVICE_ATTACHED      = 0x8031080c,
	CELL_VOICE_ERROR_SERVICE_DETACHED      = 0x8031080b,
	CELL_VOICE_ERROR_SERVICE_HANDLE        = 0x80310810,
	CELL_VOICE_ERROR_SERVICE_NOT_FOUND     = 0x8031080d,
	CELL_VOICE_ERROR_SHAREDMEMORY          = 0x8031080e,
	CELL_VOICE_ERROR_TOPOLOGY              = 0x80310807,
};

int cellVoiceConnectIPortToOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceCreateNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceCreatePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceDeletePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceDisconnectIPortFromOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceEnd()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetBitRate()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetMuteFlag()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetPortAttr()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetPortInfo()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetSignalState()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceGetVolume()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceInit()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceInitEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoicePausePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoicePausePortAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceRemoveNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceResetPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceResumePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceResumePortAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetBitRate()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetMuteFlag()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetMuteFlagAll()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetNotifyEventQueue()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetPortAttr()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceSetVolume()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceStart()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceStartEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceStop()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceUpdatePort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceWriteToIPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceWriteToIPortEx()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceReadFromOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

int cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

void cellVoice_init()
{
	cellVoice.AddFunc(0xae6a21d5, cellVoiceConnectIPortToOPort);
	cellVoice.AddFunc(0x2a01013e, cellVoiceCreateNotifyEventQueue);
	cellVoice.AddFunc(0x2de54871, cellVoiceCreatePort);
	cellVoice.AddFunc(0x9f70c475, cellVoiceDeletePort);
	cellVoice.AddFunc(0x18d3df30, cellVoiceDisconnectIPortFromOPort);
	cellVoice.AddFunc(0xe0e1ae12, cellVoiceEnd);
	cellVoice.AddFunc(0xbef53a2b, cellVoiceGetBitRate);
	cellVoice.AddFunc(0x474609e2, cellVoiceGetMuteFlag);
	cellVoice.AddFunc(0xf629ed67, cellVoiceGetPortAttr);
	cellVoice.AddFunc(0x54ac3519, cellVoiceGetPortInfo);
	cellVoice.AddFunc(0xd6811aa7, cellVoiceGetSignalState);
	cellVoice.AddFunc(0x762dc193, cellVoiceGetVolume);
	cellVoice.AddFunc(0xc7cf1182, cellVoiceInit);
	cellVoice.AddFunc(0xb1a2c38f, cellVoiceInitEx);
	cellVoice.AddFunc(0x87c71b06, cellVoicePausePort);
	cellVoice.AddFunc(0xd14e784d, cellVoicePausePortAll);
	cellVoice.AddFunc(0xdd000886, cellVoiceRemoveNotifyEventQueue);
	cellVoice.AddFunc(0xff0fa43a, cellVoiceResetPort);
	cellVoice.AddFunc(0x7bf17b15, cellVoiceResumePort);
	cellVoice.AddFunc(0x7f3963f7, cellVoiceResumePortAll);
	cellVoice.AddFunc(0x7e60adc6, cellVoiceSetBitRate);
	cellVoice.AddFunc(0xdde35a0c, cellVoiceSetMuteFlag);
	cellVoice.AddFunc(0xd4d80ea5, cellVoiceSetMuteFlagAll);
	cellVoice.AddFunc(0x35d84910, cellVoiceSetNotifyEventQueue);
	cellVoice.AddFunc(0x9d0f4af1, cellVoiceSetPortAttr);
	cellVoice.AddFunc(0xd5ae37d8, cellVoiceSetVolume);
	cellVoice.AddFunc(0x0a563878, cellVoiceStart);
	cellVoice.AddFunc(0x94d51f92, cellVoiceStartEx);
	cellVoice.AddFunc(0xd3a84be1, cellVoiceStop);
	cellVoice.AddFunc(0x2f24fea3, cellVoiceUpdatePort);
	cellVoice.AddFunc(0x3dad26e7, cellVoiceWriteToIPort);
	cellVoice.AddFunc(0x30f0b5ab, cellVoiceWriteToIPortEx);
	cellVoice.AddFunc(0x36472c57, cellVoiceReadFromOPort);
	cellVoice.AddFunc(0x20bafe31, cellVoiceDebugTopology);
}