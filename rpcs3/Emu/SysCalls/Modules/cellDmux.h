#pragma once

#include "Utilities/SQueue.h"

// Error Codes
enum
{
	CELL_DMUX_ERROR_ARG		= 0x80610201,
	CELL_DMUX_ERROR_SEQ		= 0x80610202,
	CELL_DMUX_ERROR_BUSY	= 0x80610203,
	CELL_DMUX_ERROR_EMPTY	= 0x80610204,
	CELL_DMUX_ERROR_FATAL	= 0x80610205,
};

enum CellDmuxStreamType 
{
	CELL_DMUX_STREAM_TYPE_UNDEF = 0,
	CELL_DMUX_STREAM_TYPE_PAMF = 1,
	CELL_DMUX_STREAM_TYPE_TERMINATOR = 2,
};

enum CellDmuxMsgType
{
	CELL_DMUX_MSG_TYPE_DEMUX_DONE = 0,
	CELL_DMUX_MSG_TYPE_FATAL_ERR = 1,
	CELL_DMUX_MSG_TYPE_PROG_END_CODE = 2,
};

enum CellDmuxEsMsgType
{
	CELL_DMUX_ES_MSG_TYPE_AU_FOUND = 0,
	CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE = 1,
};

enum CellDmuxPamfM2vLevel
{
	CELL_DMUX_PAMF_M2V_MP_LL = 0,
	CELL_DMUX_PAMF_M2V_MP_ML,
	CELL_DMUX_PAMF_M2V_MP_H14,
	CELL_DMUX_PAMF_M2V_MP_HL,
};

enum CellDmuxPamfAvcLevel
{
	CELL_DMUX_PAMF_AVC_LEVEL_2P1 = 21,
	CELL_DMUX_PAMF_AVC_LEVEL_3P0 = 30,
	CELL_DMUX_PAMF_AVC_LEVEL_3P1 = 31,
	CELL_DMUX_PAMF_AVC_LEVEL_3P2 = 32,
	CELL_DMUX_PAMF_AVC_LEVEL_4P1 = 41,
	CELL_DMUX_PAMF_AVC_LEVEL_4P2 = 42,
};

struct CellDmuxPamfAuSpecificInfoM2v
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfAuSpecificInfoAvc
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfAuSpecificInfoLpcm
{
	u8 channelAssignmentInfo;
	u8 samplingFreqInfo;
	u8 bitsPerSample;
};

struct CellDmuxPamfAuSpecificInfoAc3
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfAuSpecificInfoAtrac3plus
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfAuSpecificInfoUserData
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfEsSpecificInfoM2v
{
	be_t<u32> profileLevel;
};

struct CellDmuxPamfEsSpecificInfoAvc
{
	be_t<u32> level;
};

struct CellDmuxPamfEsSpecificInfoLpcm
{
	be_t<u32> samplingFreq;
	be_t<u32> numOfChannels;
	be_t<u32> bitsPerSample;
};

struct CellDmuxPamfEsSpecificInfoAc3
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfEsSpecificInfoAtrac3plus
{
	be_t<u32> reserved1;
};

struct CellDmuxPamfEsSpecificInfoUserData
{
	be_t<u32> reserved1;
};

enum CellDmuxPamfSamplingFrequency
{
	CELL_DMUX_PAMF_FS_48K = 48000,
};

enum CellDmuxPamfBitsPerSample
{
	CELL_DMUX_PAMF_BITS_PER_SAMPLE_16 = 16,
	CELL_DMUX_PAMF_BITS_PER_SAMPLE_24 = 24,
};

enum CellDmuxPamfLpcmChannelAssignmentInfo
{
	CELL_DMUX_PAMF_LPCM_CH_M1 = 1,
	CELL_DMUX_PAMF_LPCM_CH_LR = 3,
	CELL_DMUX_PAMF_LPCM_CH_LRCLSRSLFE = 9,
	CELL_DMUX_PAMF_LPCM_CH_LRCLSCS1CS2RSLFE = 11,
};

enum CellDmuxPamfLpcmFs
{
	CELL_DMUX_PAMF_LPCM_FS_48K = 1,
};

enum CellDmuxPamfLpcmBitsPerSamples
{
	CELL_DMUX_PAMF_LPCM_BITS_PER_SAMPLE_16 = 1,
	CELL_DMUX_PAMF_LPCM_BITS_PER_SAMPLE_24 = 3,
};

struct CellDmuxMsg
{
	be_t<CellDmuxMsgType> msgType; //CellDmuxMsgType enum
	be_t<u64> supplementalInfo;
};

struct CellDmuxEsMsg
{
	be_t<CellDmuxEsMsgType> msgType; //CellDmuxEsMsgType enum
	be_t<u64> supplementalInfo;
};

struct CellDmuxType 
{
	be_t<CellDmuxStreamType> streamType;
	be_t<u32> reserved[2]; //0
};

struct CellDmuxPamfSpecificInfo
{
	be_t<u32> thisSize;
	bool programEndCodeCb;
};

struct CellDmuxType2
{
	be_t<CellDmuxStreamType> streamType;
	be_t<u32> streamSpecificInfo_addr;
};

struct CellDmuxResource
{
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<u32> spuThreadPriority;
	be_t<u32> numOfSpus;
};

struct CellDmuxResourceEx
{
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<u32> spurs_addr;
	u8 priority[8];
	be_t<u32> maxContention;
};

/*
struct CellDmuxResource2Ex
{
	bool isResourceEx; //true
	CellDmuxResourceEx resourceEx;
};

struct CellDmuxResource2NoEx
{
	bool isResourceEx; //false
	CellDmuxResource resource;
};
*/

struct CellDmuxResource2
{
	bool isResourceEx;
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	union
	{
		struct
		{
			be_t<u32> noex_spuThreadPriority;
			be_t<u32> noex_numOfSpus;
		};
		struct
		{
			be_t<u32> ex_spurs_addr;
			u8 ex_priority[8];
			be_t<u32> ex_maxContention;
		};
	};
};

typedef mem_func_ptr_t<void (*)(u32 demuxerHandle_addr, mem_ptr_t<CellDmuxMsg> demuxerMsg, u32 cbArg_addr)> CellDmuxCbMsg;

struct CellDmuxCb
{
	// CellDmuxCbMsg callback
	be_t<u32> cbMsgFunc; 
	be_t<u32> cbArg_addr;
};

typedef mem_func_ptr_t<void (*)(u32 demuxerHandle_addr, u32 esHandle_addr, mem_ptr_t<CellDmuxEsMsg> esMsg, u32 cbArg_addr)> CellDmuxCbEsMsg;

struct CellDmuxEsCb
{
	// CellDmuxCbEsMsg callback
	be_t<u32> cbEsMsgFunc;
	be_t<u32> cbArg_addr;
};

struct CellDmuxAttr
{
	be_t<u32> memSize;
	be_t<u32> demuxerVerUpper;
	be_t<u32> demuxerVerLower;
};

struct CellDmuxEsAttr
{
	be_t<u32> memSize;
};

struct CellDmuxEsResource
{
    be_t<u32> memAddr;
    be_t<u32> memSize;
};

struct CellDmuxAuInfo
{
	be_t<u32> auAddr;
	be_t<u32> auSize;
	be_t<u32> auMaxSize;
    be_t<u64> userData;
	be_t<u32> ptsUpper;
	be_t<u32> ptsLower;
	be_t<u32> dtsUpper;
	be_t<u32> dtsLower;
};

struct CellDmuxAuInfoEx
{
	be_t<u32> auAddr;
	be_t<u32> auSize;
	be_t<u32> reserved;
	bool isRap;
	be_t<u64> userData;
	CellCodecTimeStamp pts;
	CellCodecTimeStamp dts;
};

/* Demuxer Thread Classes */

struct AccessUnit
{
	u32 addr;
	u32 size;
	u32 ptsUpper;
	u32 ptsLower;
	u32 dtsUpper;
	u32 dtsLower;
	u64 userData;
	bool isRap;
};

class ElementaryStream;

enum DemuxerJobType
{
	dmuxSetStream,
	dmuxResetStream,
	dmuxEnableEs,
	dmuxDisableEs,
	dmuxResetEs,
	dmuxGetAu,
	dmuxPeekAu,
	dmuxReleaseAu,
	dmuxFlushEs,
	dmuxClose,
};

#pragma pack(push, 1)
struct DemuxerTask
{
	DemuxerJobType type;

	union
	{
		struct
		{
			u32 addr;
			u64 userdata;
			u32 size;
			/*bool*/u32 discontinuity;
		} stream;

		u32 esHandle;

		struct
		{
			u32 es;
			u32 auInfo_ptr_addr;
			u32 auSpec_ptr_addr;
			ElementaryStream* es_ptr;
		} au;
	};

	DemuxerTask()
	{
	}

	DemuxerTask(DemuxerJobType type)
		: type(type)
	{
	}
};
static_assert(sizeof(DemuxerTask) == 24, "");
#pragma pack(pop)

class Demuxer
{
public:
	SQueue<DemuxerTask> job;
	const u32 memAddr;
	const u32 memSize;
	const CellDmuxCbMsg cbFunc;
	const u32 cbArg;
	bool is_finished;


	Demuxer(u32 addr, u32 size, CellDmuxCbMsg func, u32 arg)
		: is_finished(false)
		, memAddr(addr)
		, memSize(size)
		, cbFunc(func)
		, cbArg(arg)
	{
	}
};

class ElementaryStream
{
public:
	Demuxer* dmux;
	const u32 memAddr;
	const u32 memSize;
	const u32 fidMajor;
	const u32 fidMinor;
	const u32 sup1;
	const u32 sup2;
	const CellDmuxCbEsMsg cbFunc;
	const u32 cbArg;
	const u32 spec; //addr

	ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, CellDmuxCbEsMsg cbFunc, u32 cbArg, u32 spec)
		: dmux(dmux)
		, memAddr(addr)
		, memSize(size)
		, fidMajor(fidMajor)
		, fidMinor(fidMinor)
		, sup1(sup1)
		, sup2(sup2)
		, cbFunc(cbFunc)
		, cbArg(cbArg)
		, spec(spec)
	{
	}
};