#pragma once

#include "Utilities/SQueue.h"

// align size or address to 128
#define a128(x) ((x + 127) & (~127))

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
	be_t<u32> shit[4];
};

typedef mem_func_ptr_t<void (*)(u32 demuxerHandle, mem_ptr_t<CellDmuxMsg> demuxerMsg, u32 cbArg_addr)> CellDmuxCbMsg;

struct CellDmuxCb
{
	// CellDmuxCbMsg callback
	be_t<u32> cbMsgFunc; 
	be_t<u32> cbArg_addr;
};

typedef mem_func_ptr_t<void (*)(u32 demuxerHandle, u32 esHandle, mem_ptr_t<CellDmuxEsMsg> esMsg, u32 cbArg_addr)> CellDmuxCbEsMsg;

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

enum
{
	MAX_AU = 640 * 1024 + 128, // 640 KB
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

		out = *(T*)Memory.VirtualToRealAddr(addr);
		addr += sizeof(T);
		size -= sizeof(T);

		return true;
	}

	template<typename T>
	bool peek(T& out)
	{
		if (sizeof(T) > size) return false;

		out = *(T*)Memory.VirtualToRealAddr(addr);
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
	u8 ch;
	u8 size;

	PesHeader(DemuxerStream& stream)
		: pts(0)
		, dts(0)
		, ch(0)
		, size(0)
	{
		u16 header;
		stream.get(header);
		stream.get(size);
		if (size)
		{
			if (size < 10)
			{
				ConLog.Error("Unknown PesHeader size");
				Emu.Pause();
			}
			u8 v;
			stream.get(v);
			if ((v & 0xF0) != 0x30)
			{
				ConLog.Error("Pts not found");
				Emu.Pause();
			}
			pts = stream.get_ts(v);
			stream.get(v);
			if ((v & 0xF0) != 0x10)
			{
				ConLog.Error("Dts not found");
				Emu.Pause();				
			}
			dts = stream.get_ts(v);
			stream.skip(size - 10);
		}
	}
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
	dmuxReleaseAu,
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
	SQueue<DemuxerTask> job;
	const u32 memAddr;
	const u32 memSize;
	const u32 cbFunc;
	const u32 cbArg;
	u32 id;
	bool is_finished;
	bool is_running;


	Demuxer(u32 addr, u32 size, u32 func, u32 arg)
		: is_finished(false)
		, is_running(false)
		, memAddr(addr)
		, memSize(size)
		, cbFunc(func)
		, cbArg(arg)
	{
	}
};

class ElementaryStream
{
	SMutex mutex;

	u32 first_addr; // AU that will be released
	u32 last_addr; // AU that is being written now
	u32 last_size; // number of bytes written (after 128b header)
	u32 peek_addr; // AU that will be obtained by GetAu(Ex)/PeekAu(Ex)
	
public:
	Demuxer* dmux;
	u32 id;
	const u32 memAddr;
	const u32 memSize;
	const u32 fidMajor;
	const u32 fidMinor;
	const u32 sup1;
	const u32 sup2;
	const u32 cbFunc;
	const u32 cbArg;
	const u32 spec; //addr

	ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, u32 cbFunc, u32 cbArg, u32 spec)
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
		, first_addr(0)
		, peek_addr(0)
		, last_addr(a128(addr))
		, last_size(0)
	{
	}

	volatile bool hasunseen()
	{
		return peek_addr;
	}

	volatile bool hasdata()
	{
		return last_size;
	}

	bool isfull() // not multithread-safe
	{
		if (first_addr)
		{
			if (first_addr > last_addr)
			{
				return (first_addr - last_addr) < MAX_AU;
			}
			else
			{
				return (first_addr + MAX_AU) > (memAddr + memSize);
			}
		}
		else
		{
			return false;
		}
	}

	void finish(DemuxerStream& stream) // not multithread-safe
	{
		SMutexLocker lock(mutex);
		//ConLog.Write("es::finish(): peek=0x%x, first=0x%x, last=0x%x, size=0x%x", peek_addr, first_addr, last_addr, last_size);
		if (!first_addr)
		{
			first_addr = last_addr;
		}
		if (!peek_addr)
		{
			peek_addr = last_addr;
		}
		u32 new_addr = a128(last_addr + 128 + last_size);
		if ((new_addr + MAX_AU) > (memAddr + memSize))
		{
			last_addr = memAddr;
		}
		else
		{
			last_addr = new_addr;
		}
		last_size = 0;
	}

	void push(DemuxerStream& stream, u32 size, PesHeader& pes)
	{
		SMutexLocker lock(mutex);
		//ConLog.Write("es::push(): peek=0x%x, first=0x%x, last=0x%x, size=0x%x", peek_addr, first_addr, last_addr, last_size);
		if (isfull()) 
		{
			ConLog.Error("ElementaryStream::push(): buffer is full");
			Emu.Pause();
			return;
		}

		u32 data_addr = last_addr + 128 + last_size;
		last_size += size;
		if (!Memory.Copy(data_addr, stream.addr, size))
		{
			ConLog.Error("ElementaryStream::push(): data copying failed");
			Emu.Pause();
			return;
		}
		stream.skip(size);

		mem_ptr_t<CellDmuxAuInfoEx> info(last_addr);
		info->auAddr = last_addr + 128;
		info->auSize = last_size;
		if (pes.size)
		{
			info->dts.lower = (u32)pes.dts;
			info->dts.upper = (u32)(pes.dts >> 32);
			info->pts.lower = (u32)pes.pts;
			info->pts.upper = (u32)(pes.pts >> 32);
			info->isRap = false; // TODO: set valid value
			info->reserved = 0;
			info->userData = stream.userdata;
		}

		mem_ptr_t<CellDmuxPamfAuSpecificInfoAvc> tail(last_addr + sizeof(CellDmuxAuInfoEx));
		tail->reserved1 = 0;

		mem_ptr_t<CellDmuxAuInfo> inf(last_addr + 64);
		inf->auAddr = last_addr + 128;
		inf->auSize = last_size;
		if (pes.size)
		{
			inf->dtsLower = (u32)pes.dts;
			inf->dtsUpper = (u32)(pes.dts >> 32);
			inf->ptsLower = (u32)pes.pts;
			inf->ptsUpper = (u32)(pes.pts >> 32);
			inf->auMaxSize = 0; // ?????
			inf->userData = stream.userdata;
		}
	}

	volatile bool canrelease()
	{
		return first_addr;
	}

	void release()
	{
		SMutexLocker lock(mutex);
		ConLog.Write("es::release(): peek=0x%x, first=0x%x, last=0x%x, size=0x%x", peek_addr, first_addr, last_addr, last_size);
		if (!canrelease())
		{
			ConLog.Error("ElementaryStream::release(): buffer is empty");
			Emu.Pause();
			return;
		}

		u32 size = a128(Memory.Read32(first_addr + 4) + 128);
		u32 new_addr = first_addr + size;
		if (peek_addr <= first_addr) peek_addr = new_addr;
		if (new_addr == last_addr)
		{
			first_addr = 0;
		}
		else if ((new_addr + MAX_AU) > (memAddr + memSize))
		{
			first_addr = memAddr;
		}
		else
		{
			first_addr = new_addr;
		}
	}

	bool peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index)
	{
		SMutexLocker lock(mutex);
		/*ConLog.Write("es::peek(%sAu%s): peek=0x%x, first=0x%x, last=0x%x, size=0x%x", wxString(update_index ? "Get" : "Peek").wx_str(), 
			wxString(no_ex ? "" : "Ex").wx_str(), peek_addr, first_addr, last_addr, last_size);*/
		if (!peek_addr) return false;

		out_data = peek_addr;
		out_spec = out_data + sizeof(CellDmuxAuInfoEx);
		if (no_ex) out_data += 64;

		if (update_index)
		{
			u32 size = a128(Memory.Read32(peek_addr + 4) + 128);
			u32 new_addr = peek_addr + size;
			if (new_addr = last_addr)
			{
				peek_addr = 0;
			}
			else if ((new_addr + MAX_AU) > (memAddr + memSize))
			{
				peek_addr = memAddr;
			}
			else
			{
				peek_addr = new_addr;
			}
		}
		return true;
	}

	void reset()
	{
		SMutexLocker lock(mutex);
		first_addr = 0;
		peek_addr = 0;
		last_addr = a128(memAddr);
		last_size = 0;
	}
};
