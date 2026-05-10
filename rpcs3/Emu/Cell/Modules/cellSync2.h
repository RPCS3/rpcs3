#pragma once

#include "Emu/Memory/vm_ptr.h"

// Return Codes
enum CellSync2Error : u32
{
	CELL_SYNC2_ERROR_AGAIN = 0x80410C01,
	CELL_SYNC2_ERROR_INVAL = 0x80410C02,
	CELL_SYNC2_ERROR_NOMEM = 0x80410C04,
	CELL_SYNC2_ERROR_DEADLK = 0x80410C08,
	CELL_SYNC2_ERROR_PERM = 0x80410C09,
	CELL_SYNC2_ERROR_BUSY = 0x80410C0A,
	CELL_SYNC2_ERROR_STAT = 0x80410C0F,
	CELL_SYNC2_ERROR_ALIGN = 0x80410C10,
	CELL_SYNC2_ERROR_NULL_POINTER = 0x80410C11,
	CELL_SYNC2_ERROR_NOT_SUPPORTED_THREAD = 0x80410C12,
	CELL_SYNC2_ERROR_NO_NOTIFIER = 0x80410C13,
	CELL_SYNC2_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410C14,
};

// Constants
enum
{
	CELL_SYNC2_NAME_MAX_LENGTH = 31,
	CELL_SYNC2_THREAD_TYPE_PPU_THREAD = 1 << 0,
	CELL_SYNC2_THREAD_TYPE_PPU_FIBER = 1 << 1,
	CELL_SYNC2_THREAD_TYPE_SPURS_TASK = 1 << 2,
	CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB = 1 << 3,
	CELL_SYNC2_THREAD_TYPE_SPURS_JOB = 1 << 8,
};

struct CellSync2MutexAttribute
{
	be_t<u32> sdkVersion;
	be_t<u16> threadTypes;
	be_t<u16> maxWaiters;
	b8 recursive;
	u8 padding;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[86];
};

CHECK_SIZE(CellSync2MutexAttribute, 128);

struct CellSync2CondAttribute
{
	be_t<u32> sdkVersion;
	be_t<u16> maxWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[90];
};

CHECK_SIZE(CellSync2CondAttribute, 128);

struct CellSync2SemaphoreAttribute
{
	be_t<u32> sdkVersion;
	be_t<u16> threadTypes;
	be_t<u16> maxWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[88];
};

CHECK_SIZE(CellSync2SemaphoreAttribute, 128);

struct CellSync2QueueAttribute
{
	be_t<u32> sdkVersion;
	be_t<u32> threadTypes;
	be_t<u32> elementSize;
	be_t<u32> depth;
	be_t<u16> maxPushWaiters;
	be_t<u16> maxPopWaiters;
	char name[CELL_SYNC2_NAME_MAX_LENGTH + 1];
	u8 reserved[76];
};

CHECK_SIZE(CellSync2QueueAttribute, 128);

struct CellSync2CallerThreadType
{
	be_t<u16> threadTypeId;
	vm::bptr<u64(u64)> self;
	vm::bptr<s32(u64, s32, u64, u64)> waitSignal;
	vm::bptr<s32(vm::ptr<u64>, s32, u64, u64)> allocateSignalReceiver;
	vm::bptr<s32(u64, u64)> freeSignalReceiver;
	be_t<u32> spinWaitNanoSec;
	be_t<u64> callbackArg;
};

struct CellSync2Notifier
{
	be_t<u16> threadTypeId;
	vm::bptr<s32(u64, u64)> sendSignal;
	be_t<u64> callbackArg;
};

struct alignas(128) CellSync2Mutex
{
	u8 skip[128];
};

CHECK_SIZE_ALIGN(CellSync2Mutex, 128, 128);

struct alignas(128) CellSync2Cond
{
	u8 skip[128];
};

CHECK_SIZE_ALIGN(CellSync2Cond, 128, 128);

struct alignas(128) CellSync2Semaphore
{
	u8 skip[128];
};

CHECK_SIZE_ALIGN(CellSync2Semaphore, 128, 128);

struct alignas(128) CellSync2Queue
{
	u8 skip[128];
};

CHECK_SIZE_ALIGN(CellSync2Queue, 128, 128);

struct CellSync2ThreadConfig
{
	vm::bptr<CellSync2CallerThreadType> callerThreadType;
	vm::bpptr<CellSync2Notifier> notifierTable;
	be_t<u32> numNotifier;
};
