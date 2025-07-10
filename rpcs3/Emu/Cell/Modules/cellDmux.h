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

struct CellDmuxType2
{
	be_t<s32> streamType; // CellDmuxStreamType
	be_t<u32> streamSpecificInfo;
};

struct CellDmuxResource
{
	vm::bptr<void> memAddr;
	be_t<u32> memSize;
	be_t<u32> ppuThreadPriority;
	be_t<u32> ppuThreadStackSize;
	be_t<u32> spuThreadPriority;
	be_t<u32> numOfSpus;
};

struct CellDmuxResourceEx
{
	vm::bptr<void> memAddr;
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

using CellDmuxCbMsg = u32(u32 demuxerHandle, vm::cptr<CellDmuxMsg> demuxerMsg, vm::ptr<void> cbArg);

using CellDmuxCbEsMsg = u32(u32 demuxerHandle, u32 esHandle, vm::cptr<CellDmuxEsMsg> esMsg, vm::ptr<void> cbArg);

// Used for internal callbacks as well
template <typename F>
struct DmuxCb
{
	vm::bptr<F> cbFunc;
	vm::bptr<void> cbArg;
};

using CellDmuxCb = DmuxCb<CellDmuxCbMsg>;

using CellDmuxEsCb = DmuxCb<CellDmuxCbEsMsg>;

struct CellDmuxAttr
{
	be_t<u32> memSize;
	be_t<u32> demuxerVerUpper;
	be_t<u32> demuxerVerLower;
};

struct CellDmuxPamfAttr
{
	be_t<u32> maxEnabledEsNum;
	be_t<u32> version;
	be_t<u32> memSize;
};

struct CellDmuxEsAttr
{
	be_t<u32> memSize;
};

struct CellDmuxPamfEsAttr
{
	be_t<u32> auQueueMaxSize;
	be_t<u32> memSize;
	be_t<u32> specificInfoSize;
};

struct CellDmuxEsResource
{
	vm::bptr<void> memAddr;
	be_t<u32> memSize;
};

struct CellDmuxAuInfo
{
	vm::bptr<void> auAddr;
	be_t<u32> auSize;
	be_t<u32> auMaxSize;
	b8 isRap;
	be_t<u64> userData;
	CellCodecTimeStamp pts;
	CellCodecTimeStamp dts;
};

using CellDmuxAuInfoEx = CellDmuxAuInfo;

struct DmuxAuInfo
{
	CellDmuxAuInfo info;
	vm::bptr<void> specific_info;
	be_t<u32> specific_info_size;
};

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
using CellDmuxCoreOpReleaseAu = error_code(vm::ptr<void>, vm::ptr<void>, u32);
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
	vm::bptr<CellDmuxCoreOpReleaseAu> releaseAu;
	vm::bptr<CellDmuxCoreOpQueryEsAttr> queryEsAttr;
	vm::bptr<CellDmuxCoreOpEnableEs> enableEs;
	vm::bptr<CellDmuxCoreOpDisableEs> disableEs;
	vm::bptr<CellDmuxCoreOpFlushEs> flushEs;
	vm::bptr<CellDmuxCoreOpResetEs> resetEs;
	vm::bptr<CellDmuxCoreOpResetStreamAndWaitDone> resetStreamAndWaitDone;
};
