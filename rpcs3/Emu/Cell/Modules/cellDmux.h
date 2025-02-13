#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "cellPamf.h"

// Error Codes
enum CellDmuxError :u32
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
	b8 programEndCodeCb;
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

struct CellDmuxResourceSpurs
{
	vm::bptr<void> spurs; // CellSpurs*
	be_t<u64, 1> priority;
	be_t<u32> maxContention;
};

/*
struct CellDmuxResource2Ex
{
	b8 isResourceEx; //true
	CellDmuxResourceEx resourceEx;
};

struct CellDmuxResource2NoEx
{
	b8 isResourceEx; //false
	CellDmuxResource resource;
};
*/

struct CellDmuxResource2
{
	b8 isResourceEx;
	be_t<u32> memAddr;
	be_t<u32> memSize;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<u32> shit[4];
};

using CellDmuxCbMsg = u32(u32 demuxerHandle, vm::ptr<CellDmuxMsg> demuxerMsg, u32 cbArg);

using CellDmuxCbEsMsg = u32(u32 demuxerHandle, u32 esHandle, vm::ptr<CellDmuxEsMsg> esMsg, u32 cbArg);

// Used for internal callbacks as well
template <typename F>
struct DmuxCb
{
	vm::bptr<F> cbFunc;
	be_t<u32> cbArg;
};

using CellDmuxCb = DmuxCb<CellDmuxCbMsg>;

using CellDmuxEsCb = DmuxCb<CellDmuxCbEsMsg>;

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
	b8 isRap;
	be_t<u64> userData;
	CellCodecTimeStamp pts;
	CellCodecTimeStamp dts;
};

struct CellDmuxPamfAttr;
struct CellDmuxPamfEsAttr;

using DmuxNotifyDemuxDone = error_code(vm::ptr<void>, u32, vm::ptr<void>);
using DmuxNotifyFatalErr = error_code(vm::ptr<void>, u32, vm::ptr<void>);
using DmuxNotifyProgEndCode = error_code(vm::ptr<void>, vm::ptr<void>);

using DmuxEsNotifyAuFound = error_code(vm::ptr<void>, vm::cptr<void>, vm::ptr<void>);
using DmuxEsNotifyFlushDone = error_code(vm::ptr<void>, vm::ptr<void>);

using CellDmuxCoreOpQueryAttr = error_code(vm::cptr<void>, vm::ptr<CellDmuxPamfAttr>);
using CellDmuxCoreOpOpen = error_code(vm::cptr<void>, vm::cptr<CellDmuxResource>, vm::cptr<CellDmuxResourceSpurs>, vm::cptr<DmuxCb<DmuxNotifyDemuxDone>>, vm::cptr<DmuxCb<DmuxNotifyProgEndCode>>, vm::cptr<DmuxCb<DmuxNotifyFatalErr>>, vm::pptr<void>);
using CellDmuxCoreOpClose = error_code(vm::ptr<void>);
using CellDmuxCoreOpResetStream = error_code(vm::ptr<void>);
using CellDmuxCoreOpCreateThread = error_code(vm::ptr<void>);
using CellDmuxCoreOpJoinThread = error_code(vm::ptr<void>);
using CellDmuxCoreOpSetStream = error_code(vm::ptr<void>, vm::cptr<void>, u32, b8, u64);
using CellDmuxCoreOpFreeMemory = error_code(vm::ptr<void>, vm::ptr<void>, u32);
using CellDmuxCoreOpQueryEsAttr = error_code(vm::cptr<void>, vm::cptr<void>, vm::ptr<CellDmuxPamfEsAttr>);
using CellDmuxCoreOpEnableEs = error_code(vm::ptr<void>, vm::cptr<void>, vm::cptr<CellDmuxEsResource>, vm::cptr<DmuxCb<DmuxEsNotifyAuFound>>, vm::cptr<DmuxCb<DmuxEsNotifyFlushDone>>, vm::cptr<void>, vm::pptr<void>);
using CellDmuxCoreOpDisableEs = u32(vm::ptr<void>);
using CellDmuxCoreOpFlushEs = u32(vm::ptr<void>);
using CellDmuxCoreOpResetEs = u32(vm::ptr<void>);
using CellDmuxCoreOpResetStreamAndWaitDone = u32(vm::ptr<void>);

struct CellDmuxCoreOps
{
	vm::bptr<CellDmuxCoreOpQueryAttr> queryAttr;
	vm::bptr<CellDmuxCoreOpOpen> open;
	vm::bptr<CellDmuxCoreOpClose> close;
	vm::bptr<CellDmuxCoreOpResetStream> resetStream;
	vm::bptr<CellDmuxCoreOpCreateThread> createThread;
	vm::bptr<CellDmuxCoreOpJoinThread> joinThread;
	vm::bptr<CellDmuxCoreOpSetStream> setStream;
	vm::bptr<CellDmuxCoreOpFreeMemory> freeMemory;
	vm::bptr<CellDmuxCoreOpQueryEsAttr> queryEsAttr;
	vm::bptr<CellDmuxCoreOpEnableEs> enableEs;
	vm::bptr<CellDmuxCoreOpDisableEs> disableEs;
	vm::bptr<CellDmuxCoreOpFlushEs> flushEs;
	vm::bptr<CellDmuxCoreOpResetEs> resetEs;
	vm::bptr<CellDmuxCoreOpResetStreamAndWaitDone> resetStreamAndWaitDone;
};
