#pragma once

//libsnd3 Error Codes
enum
{
	CELL_SND3_ERROR_PARAM = 0x80310301,
	CELL_SND3_ERROR_CREATE_MUTEX = 0x80310302,
	CELL_SND3_ERROR_SYNTH = 0x80310303,
	CELL_SND3_ERROR_ALREADY = 0x80310304,
	CELL_SND3_ERROR_NOTINIT = 0x80310305,
	CELL_SND3_ERROR_SMFFULL = 0x80310306,
	CELL_SND3_ERROR_HD3ID = 0x80310307,
	CELL_SND3_ERROR_SMF = 0x80310308,
	CELL_SND3_ERROR_SMFCTX = 0x80310309,
	CELL_SND3_ERROR_FORMAT = 0x8031030a,
	CELL_SND3_ERROR_SMFID = 0x8031030b,
	CELL_SND3_ERROR_SOUNDDATAFULL = 0x8031030c,
	CELL_SND3_ERROR_VOICENUM = 0x8031030d,
	CELL_SND3_ERROR_RESERVEDVOICE = 0x8031030e,
	CELL_SND3_ERROR_REQUESTQUEFULL = 0x8031030f,
	CELL_SND3_ERROR_OUTPUTMODE = 0x80310310,
};

//libsnd3 datatypes
struct CellSnd3DataCtx
{
	s8 system;  //[CELL_SND3_DATA_CTX_SIZE], unknown identifier
};

struct CellSnd3SmfCtx
{
	s8 system;  //[CELL_SND3_SMF_CTX_SIZE],  unknown identifier
};

struct CellSnd3KeyOnParam
{
	u8 vel;
	u8 pan;
	u8 panEx;
	be_t<s32> addPitch;
};

struct CellSnd3VoiceBitCtx
{
	be_t<u32> core;  //[CELL_SND3_MAX_CORE],  unknown identifier
};

struct CellSnd3RequestQueueCtx
{
	void *frontQueue;
	be_t<u32> frontQueueSize;
	void *rearQueue;
	be_t<u32> rearQueueSize;
};