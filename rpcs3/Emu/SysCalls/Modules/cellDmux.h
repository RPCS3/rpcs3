#pragma once

#include "Utilities/SQueue.h"

// align size or address to 128
#define a128(x) ((x + 127) & (~127))

// Error Codes
enum
{
	CELL_DMUX_ERROR_ARG     = 0x80610201,
	CELL_DMUX_ERROR_SEQ     = 0x80610202,
	CELL_DMUX_ERROR_BUSY    = 0x80610203,
	CELL_DMUX_ERROR_EMPTY   = 0x80610204,
	CELL_DMUX_ERROR_FATAL   = 0x80610205,
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
	be_t<u32> shit[4];
};

typedef u32(*CellDmuxCbMsg)(u32 demuxerHandle, vm::ptr<CellDmuxMsg> demuxerMsg, u32 cbArg);

struct CellDmuxCb
{
	vm::bptr<CellDmuxCbMsg> cbMsgFunc; 
	be_t<u32> cbArg;
};

typedef u32(*CellDmuxCbEsMsg)(u32 demuxerHandle, u32 esHandle, vm::ptr<CellDmuxEsMsg> esMsg, u32 cbArg);

struct CellDmuxEsCb
{
	vm::bptr<CellDmuxCbEsMsg> cbEsMsgFunc;
	be_t<u32> cbArg;
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

enum
{
	/* http://dvd.sourceforge.net/dvdinfo/mpeghdrs.html */

	PACKET_START_CODE_MASK   = 0xffffff00,
	PACKET_START_CODE_PREFIX = 0x00000100,

	USER_DATA_START_CODE     = 0x000001b2,
	SEQUENCE_START_CODE      = 0x000001b3,
	EXT_START_CODE           = 0x000001b5,
	SEQUENCE_END_CODE        = 0x000001b7,
	GOP_START_CODE           = 0x000001b8,
	ISO_11172_END_CODE       = 0x000001b9,
	PACK_START_CODE          = 0x000001ba,
	SYSTEM_HEADER_START_CODE = 0x000001bb,
	PROGRAM_STREAM_MAP       = 0x000001bc,
	PRIVATE_STREAM_1         = 0x000001bd,
	PADDING_STREAM           = 0x000001be,
	PRIVATE_STREAM_2         = 0x000001bf,
};

struct DemuxerStream
{
	u32 addr;
	u32 size;
	u64 userdata;
	bool discontinuity;

	template<typename T>
	bool get(T& out)
	{
		if (sizeof(T) > size) return false;

		out = vm::get_ref<T>(addr);
		addr += sizeof(T);
		size -= sizeof(T);

		return true;
	}

	template<typename T>
	bool peek(T& out, u32 shift = 0)
	{
		if (sizeof(T) + shift > size) return false;

		out = vm::get_ref<T>(addr + shift);
		return true;
	}

	void skip(u32 count)
	{
		addr += count;
		size = size > count ? size - count : 0;
	}

	u64 get_ts(u8 c)
	{
		u8 v[4]; get((u32&)v); 
		return
			(((u64)c & 0x0e) << 29) | 
			(((u64)v[0]) << 21) |
			(((u64)v[1] & 0x7e) << 15) |
			(((u64)v[2]) << 7) | ((u64)v[3] >> 1);
	}

	u64 get_ts()
	{
		u8 v; get(v);
		return get_ts(v);
	}
};

struct PesHeader
{
	u64 pts;
	u64 dts;
	u8 size;
	bool new_au;

	PesHeader(DemuxerStream& stream);
};

class ElementaryStream;

enum DemuxerJobType
{
	dmuxSetStream,
	dmuxResetStream,
	dmuxResetStreamAndWaitDone,
	dmuxEnableEs,
	dmuxDisableEs,
	dmuxResetEs,
	dmuxFlushEs,
	dmuxClose,
};

struct DemuxerTask
{
	DemuxerJobType type;

	union
	{
		DemuxerStream stream;

		struct
		{
			u32 es;
			u32 auInfo_ptr_addr;
			u32 auSpec_ptr_addr;
			ElementaryStream* es_ptr;
		} es;
	};

	DemuxerTask()
	{
	}

	DemuxerTask(DemuxerJobType type)
		: type(type)
	{
	}
};

class Demuxer
{
public:
	SQueue<DemuxerTask, 32> job;
	SQueue<u32, 16> fbSetStream;
	const u32 memAddr;
	const u32 memSize;
	const vm::ptr<CellDmuxCbMsg> cbFunc;
	const u32 cbArg;
	u32 id;
	volatile bool is_finished;
	volatile bool is_closed;
	volatile bool is_running;

	PPUThread* dmuxCb;

	Demuxer(u32 addr, u32 size, vm::ptr<CellDmuxCbMsg> func, u32 arg)
		: is_finished(false)
		, is_closed(false)
		, is_running(false)
		, memAddr(addr)
		, memSize(size)
		, cbFunc(func)
		, cbArg(arg)
		, dmuxCb(nullptr)
	{
	}
};

class ElementaryStream
{
	std::mutex m_mutex;

	SQueue<u32> entries; // AU starting addresses
	u32 put_count; // number of AU written
	u32 released; // number of AU released
	u32 peek_count; // number of AU obtained by GetAu(Ex)

	u32 put; // AU that is being written now
	u32 size; // number of bytes written (after 128b header)
	//u32 first; // AU that will be released
	//u32 peek; // AU that will be obtained by GetAu(Ex)/PeekAu(Ex)

	bool is_full();
	
public:
	Demuxer* dmux;
	u32 id;
	const u32 memAddr;
	const u32 memSize;
	const u32 fidMajor;
	const u32 fidMinor;
	const u32 sup1;
	const u32 sup2;
	const vm::ptr<CellDmuxCbEsMsg> cbFunc;
	const u32 cbArg;
	const u32 spec; //addr

	ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, vm::ptr<CellDmuxCbEsMsg> cbFunc, u32 cbArg, u32 spec)
		: dmux(dmux)
		, memAddr(a128(addr))
		, memSize(size - (addr - memAddr))
		, fidMajor(fidMajor)
		, fidMinor(fidMinor)
		, sup1(sup1)
		, sup2(sup2)
		, cbFunc(cbFunc)
		, cbArg(cbArg)
		, spec(spec)
		//, first(0)
		//, peek(0)
		, put(memAddr)
		, size(0)
		, put_count(0)
		, released(0)
		, peek_count(0)
	{
	}

	const u32 GetMaxAU() const;

	u32 freespace();

	bool hasunseen();

	bool hasdata();

	bool isfull();

	void finish(DemuxerStream& stream);

	void push(DemuxerStream& stream, u32 sz, PesHeader& pes);

	bool release();

	bool peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index);

	void reset();
};
