#pragma once

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
	CellDmuxStreamType streamType;
	be_t<u32> reserved[2]; //0
};

struct CellDmuxType2
{
	CellDmuxStreamType streamType;
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
			be_t<u32> spuThreadPriority;
			be_t<u32> numOfSpus;
		};
		struct
		{
			be_t<u32> ex_spurs_addr;
			u8 ex_priority[8];
			be_t<u32> ex_maxContention;
		};
	};
};

struct CellDmuxCb
{
	// CellDmuxCbMsg callback
	be_t<mem_func_ptr_t<void (*)(u32 demuxerHandle_addr, mem_ptr_t<CellDmuxMsg> demuxerMsg, u32 cbArg_addr)>> cbMsgFunc; 
	be_t<u32> cbArg_addr;
};

struct CellDmuxEsCb
{
	// CellDmuxCbEsMsg callback
	be_t<mem_func_ptr_t<void (*)(u32 demuxerHandle_addr, u32 esHandle_addr, mem_ptr_t<CellDmuxEsMsg> esMsg, u32 cbArg_addr)>> cbEsMsgFunc;
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
    CellCodecTimeStamp  pts;
    CellCodecTimeStamp  dts;
};