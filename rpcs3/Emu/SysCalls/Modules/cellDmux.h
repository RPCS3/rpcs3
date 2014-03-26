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
	bool peek(T& out, u32 shift = 0)
	{
		if (sizeof(T) + shift > size) return false;

		out = *(T*)Memory.VirtualToRealAddr(addr + shift);
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

	PesHeader(DemuxerStream& stream)
		: pts(0xffffffffffffffff)
		, dts(0xffffffffffffffff)
		, size(0)
		, new_au(false)
	{
		u16 header;
		stream.get(header);
		stream.get(size);
		if (size)
		{
			u8 empty = 0;
			u8 v;
			while (true)
			{
				stream.get(v);
				if (v != 0xFF) break; // skip padding bytes
				empty++;
				if (empty == size) return;
			};

			if ((v & 0xF0) == 0x20 && (size - empty) >= 5) // pts only
			{
				new_au = true;
				pts = stream.get_ts(v);
				stream.skip(size - empty - 5);
			}
			else
			{
				new_au = true;
				if ((v & 0xF0) != 0x30 || (size - empty) < 10)
				{
					ConLog.Error("PesHeader(): pts not found");
					Emu.Pause();
				}
				pts = stream.get_ts(v);
				stream.get(v);
				if ((v & 0xF0) != 0x10)
				{
					ConLog.Error("PesHeader(): dts not found");
					Emu.Pause();				
				}
				dts = stream.get_ts(v);
				stream.skip(size - empty - 10);
			}
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
	const u32 cbFunc;
	const u32 cbArg;
	u32 id;
	volatile bool is_finished;
	volatile bool is_running;

	CPUThread* dmuxCb;

	Demuxer(u32 addr, u32 size, u32 func, u32 arg)
		: is_finished(false)
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
	SMutex mutex;

	SQueue<u32> entries; // AU starting addresses
	u32 put_count; // number of AU written
	u32 released; // number of AU released
	u32 peek_count; // number of AU obtained by GetAu(Ex)

	u32 put; // AU that is being written now
	u32 size; // number of bytes written (after 128b header)
	//u32 first; // AU that will be released
	//u32 peek; // AU that will be obtained by GetAu(Ex)/PeekAu(Ex)

	bool is_full()
	{
		if (released < put_count)
		{
			u32 first = entries.Peek();
			if (first >= put)
			{
				return (first - put) < GetMaxAU();
			}
			else
			{
				// probably, always false
				return (put + GetMaxAU()) > (memAddr + memSize);
			}
		}
		else
		{
			return false;
		}
	}
	
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

	const u32 GetMaxAU() const
	{
		return (fidMajor == 0xbd) ? 4096 : 640 * 1024 + 128; // TODO
	}

	u32 freespace()
	{
		if (size > GetMaxAU())
		{
			ConLog.Error("es::freespace(): last_size too big (size=0x%x, max_au=0x%x)", size, GetMaxAU());
			Emu.Pause();
			return 0;
		}
		return GetMaxAU() - size;
	}

	bool hasunseen()
	{
		SMutexLocker lock(mutex);
		return peek_count < put_count;
	}

	bool hasdata()
	{
		SMutexLocker lock(mutex);
		return size;
	}

	bool isfull()
	{
		SMutexLocker lock(mutex);
		return is_full();
	}

	void finish(DemuxerStream& stream) // not multithread-safe
	{
		u32 addr;
		{
			SMutexLocker lock(mutex);
			//if (fidMajor != 0xbd) ConLog.Write(">>> es::finish(): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", peek, first, put, size);

			addr = put;
			/*if (!first)
			{
				first = put;
			}
			if (!peek)
			{
				peek = put;
			}*/

			mem_ptr_t<CellDmuxAuInfo> info(put);
			//if (fidMajor != 0xbd) ConLog.Warning("es::finish(): (%s) size = 0x%x, info_addr=0x%x, pts = 0x%x",
				//wxString(fidMajor == 0xbd ? "ATRAC3P Audio" : "Video AVC").wx_str(),
				//(u32)info->auSize, put, (u32)info->ptsLower);

			u32 new_addr = a128(put + 128 + size);
			put = ((new_addr + GetMaxAU()) > (memAddr + memSize))
			    ? memAddr : new_addr;

			size = 0;

			put_count++;
			//if (fidMajor != 0xbd) ConLog.Write("<<< es::finish(): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", peek, first, put, size);
		}
		if (!entries.Push(addr))
		{
			ConLog.Error("es::finish() aborted (no space)");
		}
	}

	void push(DemuxerStream& stream, u32 sz, PesHeader& pes)
	{
		SMutexLocker lock(mutex);

		if (is_full())
		{
			ConLog.Error("es::push(): buffer is full");
			Emu.Pause();
			return;
		}

		u32 data_addr = put + 128 + size;
		size += sz;
		if (!Memory.Copy(data_addr, stream.addr, sz))
		{
			ConLog.Error("es::push(): data copying failed");
			Emu.Pause();
			return;
		}
		stream.skip(sz);

		mem_ptr_t<CellDmuxAuInfoEx> info(put);
		info->auAddr = put + 128;
		info->auSize = size;
		if (pes.new_au)
		{
			info->dts.lower = (u32)pes.dts;
			info->dts.upper = (u32)(pes.dts >> 32);
			info->pts.lower = (u32)pes.pts;
			info->pts.upper = (u32)(pes.pts >> 32);
			info->isRap = false; // TODO: set valid value
			info->reserved = 0;
			info->userData = stream.userdata;
		}

		mem_ptr_t<CellDmuxPamfAuSpecificInfoAvc> tail(put + sizeof(CellDmuxAuInfoEx));
		tail->reserved1 = 0;

		mem_ptr_t<CellDmuxAuInfo> inf(put + 64);
		inf->auAddr = put + 128;
		inf->auSize = size;
		if (pes.new_au)
		{
			inf->dtsLower = (u32)pes.dts;
			inf->dtsUpper = (u32)(pes.dts >> 32);
			inf->ptsLower = (u32)pes.pts;
			inf->ptsUpper = (u32)(pes.pts >> 32);
			inf->auMaxSize = 0; // ?????
			inf->userData = stream.userdata;
		}
	}

	bool release()
	{
		SMutexLocker lock(mutex);
		//if (fidMajor != 0xbd) ConLog.Write(">>> es::release(): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", peek, first, put, size);
		if (released >= put_count)
		{
			ConLog.Error("es::release(): buffer is empty");
			return false;
		}

		u32 addr = entries.Peek();

		mem_ptr_t<CellDmuxAuInfo> info(addr);
		//if (fidMajor != 0xbd) ConLog.Warning("es::release(): (%s) size = 0x%x, info = 0x%x, pts = 0x%x",
			//wxString(fidMajor == 0xbd ? "ATRAC3P Audio" : "Video AVC").wx_str(), (u32)info->auSize, first, (u32)info->ptsLower);

		if (released >= peek_count)
		{
			ConLog.Error("es::release(): buffer has not been seen yet");
			return false;
		}

		/*u32 new_addr = a128(info.GetAddr() + 128 + info->auSize);

		if (new_addr == put)
		{
			first = 0;
		}
		else if ((new_addr + GetMaxAU()) > (memAddr + memSize))
		{
			first = memAddr;
		}
		else
		{
			first = new_addr;
		}*/

		released++;
		if (!entries.Pop(addr))
		{
			ConLog.Error("es::release(): entries.Pop() aborted (no entries found)");
			return false;
		}
		//if (fidMajor != 0xbd) ConLog.Write("<<< es::release(): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", peek, first, put, size);
		return true;
	}

	bool peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index)
	{
		SMutexLocker lock(mutex);
		//if (fidMajor != 0xbd) ConLog.Write(">>> es::peek(%sAu%s): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", wxString(update_index ? "Get" : "Peek").wx_str(), 
			//wxString(no_ex ? "" : "Ex").wx_str(), peek, first, put, size);
		if (peek_count >= put_count) return false;

		if (peek_count < released)
		{
			ConLog.Error("es::peek(): sequence error: peek_count < released (peek_count=%d, released=%d)", peek_count, released);
			Emu.Pause();
			return false;
		}

		u32 addr = entries.Peek(peek_count - released);
		mem_ptr_t<CellDmuxAuInfo> info(addr);
		//if (fidMajor != 0xbd) ConLog.Warning("es::peek(%sAu(Ex)): (%s) size = 0x%x, info = 0x%x, pts = 0x%x",
			//wxString(update_index ? "Get" : "Peek").wx_str(),
			//wxString(fidMajor == 0xbd ? "ATRAC3P Audio" : "Video AVC").wx_str(), (u32)info->auSize, peek, (u32)info->ptsLower);

		out_data = addr;
		out_spec = out_data + sizeof(CellDmuxAuInfoEx);
		if (no_ex) out_data += 64;

		if (update_index)
		{
			/*u32 new_addr = a128(peek + 128 + info->auSize);
			if (new_addr == put)
			{
				peek = 0;
			}
			else if ((new_addr + GetMaxAU()) > (memAddr + memSize))
			{
				peek = memAddr;
			}
			else
			{
				peek = new_addr;
			}*/
			peek_count++;
		}

		//if (fidMajor != 0xbd) ConLog.Write("<<< es::peek(%sAu%s): peek=0x%x, first=0x%x, put=0x%x, size=0x%x", wxString(update_index ? "Get" : "Peek").wx_str(), 
			//wxString(no_ex ? "" : "Ex").wx_str(), peek, first, put, size);
		return true;
	}

	void reset()
	{
		SMutexLocker lock(mutex);
		//first = 0;
		//peek = 0;
		put = memAddr;
		size = 0;
		entries.Clear();
		put_count = 0;
		released = 0;
		peek_count = 0;
	}
};
