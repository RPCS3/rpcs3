#pragma once

namespace vm { using namespace ps3; }

// Error Codes
enum
{
	CELL_DMUX_ERROR_ARG     = 0x80610201,
	CELL_DMUX_ERROR_SEQ     = 0x80610202,
	CELL_DMUX_ERROR_BUSY    = 0x80610203,
	CELL_DMUX_ERROR_EMPTY   = 0x80610204,
	CELL_DMUX_ERROR_FATAL   = 0x80610205,
};

enum CellDmuxStreamType : s32
{
	CELL_DMUX_STREAM_TYPE_UNDEF = 0,
	CELL_DMUX_STREAM_TYPE_PAMF = 1,
	CELL_DMUX_STREAM_TYPE_TERMINATOR = 2,
};

enum CellDmuxMsgType : s32
{
	CELL_DMUX_MSG_TYPE_DEMUX_DONE = 0,
	CELL_DMUX_MSG_TYPE_FATAL_ERR = 1,
	CELL_DMUX_MSG_TYPE_PROG_END_CODE = 2,
};

enum CellDmuxEsMsgType : s32
{
	CELL_DMUX_ES_MSG_TYPE_AU_FOUND = 0,
	CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE = 1,
};

enum CellDmuxPamfM2vLevel : s32
{
	CELL_DMUX_PAMF_M2V_MP_LL = 0,
	CELL_DMUX_PAMF_M2V_MP_ML,
	CELL_DMUX_PAMF_M2V_MP_H14,
	CELL_DMUX_PAMF_M2V_MP_HL,
};

enum CellDmuxPamfAvcLevel : s32
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

enum CellDmuxPamfSamplingFrequency : s32
{
	CELL_DMUX_PAMF_FS_48K = 48000,
};

enum CellDmuxPamfBitsPerSample : s32
{
	CELL_DMUX_PAMF_BITS_PER_SAMPLE_16 = 16,
	CELL_DMUX_PAMF_BITS_PER_SAMPLE_24 = 24,
};

enum CellDmuxPamfLpcmChannelAssignmentInfo : s32
{
	CELL_DMUX_PAMF_LPCM_CH_M1 = 1,
	CELL_DMUX_PAMF_LPCM_CH_LR = 3,
	CELL_DMUX_PAMF_LPCM_CH_LRCLSRSLFE = 9,
	CELL_DMUX_PAMF_LPCM_CH_LRCLSCS1CS2RSLFE = 11,
};

enum CellDmuxPamfLpcmFs : s32
{
	CELL_DMUX_PAMF_LPCM_FS_48K = 1,
};

enum CellDmuxPamfLpcmBitsPerSamples : s32
{
	CELL_DMUX_PAMF_LPCM_BITS_PER_SAMPLE_16 = 1,
	CELL_DMUX_PAMF_LPCM_BITS_PER_SAMPLE_24 = 3,
};

struct CellDmuxMsg
{
	be_t<s32> msgType; // CellDmuxMsgType
	be_t<u64> supplementalInfo;
};

struct CellDmuxEsMsg
{
	be_t<s32> msgType; // CellDmuxEsMsgType
	be_t<u64> supplementalInfo;
};

struct CellDmuxType 
{
	be_t<s32> streamType; // CellDmuxStreamType
	be_t<u32> reserved[2];
};

struct CellDmuxPamfSpecificInfo
{
	be_t<u32> thisSize;
	bool programEndCodeCb;
};

struct CellDmuxType2
{
	be_t<s32> streamType; // CellDmuxStreamType
	be_t<u32> streamSpecificInfo;
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

using CellDmuxCbMsg = func_def<u32(u32 demuxerHandle, vm::ptr<CellDmuxMsg> demuxerMsg, u32 cbArg)>;

struct CellDmuxCb
{
	vm::bptr<CellDmuxCbMsg> cbMsgFunc; 
	be_t<u32> cbArg;
};

using CellDmuxCbEsMsg = func_def<u32(u32 demuxerHandle, u32 esHandle, vm::ptr<CellDmuxEsMsg> esMsg, u32 cbArg)>;

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

	PACK_START_CODE          = 0x000001ba,
	SYSTEM_HEADER_START_CODE = 0x000001bb,
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

	bool check(u32 count) const
	{
		return count <= size;
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
};

struct PesHeader
{
	u64 pts;
	u64 dts;
	u8 size;
	bool has_ts;
	bool is_ok;

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
	squeue_t<DemuxerTask, 32> job;
	const u32 memAddr;
	const u32 memSize;
	const vm::ptr<CellDmuxCbMsg> cbFunc;
	const u32 cbArg;
	u32 id;
	volatile bool is_finished;
	volatile bool is_closed;
	std::atomic<bool> is_running;
	std::atomic<bool> is_working;

	std::shared_ptr<PPUThread> dmuxCb;

	Demuxer(u32 addr, u32 size, vm::ptr<CellDmuxCbMsg> func, u32 arg)
		: is_finished(false)
		, is_closed(false)
		, is_running(false)
		, is_working(false)
		, memAddr(addr)
		, memSize(size)
		, cbFunc(func)
		, cbArg(arg)
	{
	}
};

class ElementaryStream
{
	std::mutex m_mutex;

	squeue_t<u32> entries; // AU starting addresses
	u32 put_count; // number of AU written
	u32 got_count; // number of AU obtained by GetAu(Ex)
	u32 released; // number of AU released

	u32 put; // AU that is being written now

	bool is_full(u32 space);
	
public:
	ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, vm::ptr<CellDmuxCbEsMsg> cbFunc, u32 cbArg, u32 spec);

	Demuxer* dmux;
	const u32 id;
	const u32 memAddr;
	const u32 memSize;
	const u32 fidMajor;
	const u32 fidMinor;
	const u32 sup1;
	const u32 sup2;
	const vm::ptr<CellDmuxCbEsMsg> cbFunc;
	const u32 cbArg;
	const u32 spec; //addr

	std::vector<u8> raw_data; // demultiplexed data stream (managed by demuxer thread)
	size_t raw_pos; // should be <= raw_data.size()
	u64 last_dts;
	u64 last_pts;

	void push(DemuxerStream& stream, u32 size); // called by demuxer thread (not multithread-safe)

	bool isfull(u32 space);

	void push_au(u32 size, u64 dts, u64 pts, u64 userdata, bool rap, u32 specific);

	bool release();

	bool peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index);

	void reset();
};
