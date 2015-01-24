#pragma once

union SceKernelSysClock
{
	struct
	{
		u32 low;
		u32 hi;
	};

	u64 quad;
};

struct SceKernelCallFrame
{
	u32 sp;
	u32 pc;
};

// Memory Manager definitions

typedef s32 SceKernelMemoryType;

struct SceKernelMemBlockInfo
{
	u32 size;
	vm::psv::ptr<void> mappedBase;
	u32 mappedSize;
	SceKernelMemoryType memoryType;
	u32 access;
};

struct SceKernelAllocMemBlockOpt
{
	u32 size;
	u32 attr;
	u32 alignment;
	s32 uidBaseBlock;
	vm::psv::ptr<const char> strBaseBlockName;
};

// Thread Manager definitions (threads)

typedef s32(*SceKernelThreadEntry)(u32 argSize, vm::psv::ptr<void> pArgBlock);

struct SceKernelThreadOptParam
{
	u32 size;
	u32 attr;
};

struct SceKernelThreadInfo
{
	u32 size;
	s32 processId;
	char name[32];
	u32 attr;
	u32 status;
	vm::psv::ptr<SceKernelThreadEntry> entry;
	vm::psv::ptr<void> pStack;
	u32 stackSize;
	s32 initPriority;
	s32 currentPriority;
	s32 initCpuAffinityMask;
	s32 currentCpuAffinityMask;
	s32 currentCpuId;
	s32 lastExecutedCpuId;
	u32 waitType;
	s32 waitId;
	s32 exitStatus;
	SceKernelSysClock runClocks;
	u32 intrPreemptCount;
	u32 threadPreemptCount;
	u32 threadReleaseCount;
	s32 changeCpuCount;
	s32 fNotifyCallback;
	s32 reserved;
};

struct SceKernelThreadRunStatus
{
	u32 size;

	struct
	{
		s32 processId;
		s32 threadId;
		s32 priority;

	} cpuInfo[4];
};

struct SceKernelSystemInfo
{
	u32 size;
	u32 activeCpuMask;

	struct
	{
		SceKernelSysClock idleClock;
		u32 comesOutOfIdleCount;
		u32 threadSwitchCount;

	} cpuInfo[4];
};

// Thread Manager definitions (callbacks)

typedef s32(*SceKernelCallbackFunction)(s32 notifyId, s32 notifyCount, s32 notifyArg, vm::psv::ptr<void> pCommon);

struct SceKernelCallbackInfo
{
	u32 size;
	s32 callbackId;
	char name[32];
	u32 attr;
	s32 threadId;
	vm::psv::ptr<SceKernelCallbackFunction> callbackFunc;
	s32 notifyId;
	s32 notifyCount;
	s32 notifyArg;
	vm::psv::ptr<void> pCommon;
};

// Thread Manager definitions (events)

typedef s32(*SceKernelThreadEventHandler)(s32 type, s32 threadId, s32 arg, vm::psv::ptr<void> pCommon);

struct SceKernelEventInfo
{
	u32 size;
	s32 eventId;
	char name[32];
	u32 attr;
	u32 eventPattern;
	u64 userData;
	u32 numWaitThreads;
	s32 reserved[1];
};

struct SceKernelWaitEvent
{
	s32 eventId;
	u32 eventPattern;
};

struct SceKernelResultEvent
{
	s32 eventId;
	s32 result;
	u32 resultPattern;
	s32 reserved[1];
	u64 userData;
};

// Thread Manager definitions (event flags)

struct SceKernelEventFlagOptParam
{
	u32 size;
};

struct SceKernelEventFlagInfo
{
	u32 size;
	s32 evfId;
	char name[32];
	u32 attr;
	u32 initPattern;
	u32 currentPattern;
	s32 numWaitThreads;
};

// Thread Manager definitions (semaphores)

struct SceKernelSemaOptParam
{
	u32 size;
};

struct SceKernelSemaInfo
{
	u32 size;
	s32 semaId;
	char name[32];
	u32 attr;
	s32 initCount;
	s32 currentCount;
	s32 maxCount;
	s32 numWaitThreads;
};

// Thread Manager definitions (mutexes)

struct SceKernelMutexOptParam
{
	u32 size;
	s32 ceilingPriority;
};

struct SceKernelMutexInfo
{
	u32 size;
	s32 mutexId;
	char name[32];
	u32 attr;
	s32 initCount;
	s32 currentCount;
	s32 currentOwnerId;
	s32 numWaitThreads;
};

// Thread Manager definitions (lightweight mutexes)

struct SceKernelLwMutexWork
{
	s32 data[4];
};

struct SceKernelLwMutexOptParam
{
	u32 size;
};

struct SceKernelLwMutexInfo
{
	u32 size;
	s32 uid;
	char name[32];
	u32 attr;
	vm::psv::ptr<SceKernelLwMutexWork> pWork;
	s32 initCount;
	s32 currentCount;
	s32 currentOwnerId;
	s32 numWaitThreads;
};

// Thread Manager definitions (condition variables)

struct SceKernelCondOptParam
{
	u32 size;
};

struct SceKernelCondInfo
{
	u32 size;
	s32 condId;
	char name[32];
	u32 attr;
	s32 mutexId;
	u32 numWaitThreads;
};

// Thread Manager definitions (lightweight condition variables)

struct SceKernelLwCondWork
{
	s32 data[4];
};

struct SceKernelLwCondOptParam
{
	u32 size;
};

struct SceKernelLwCondInfo
{
	u32 size;
	s32 uid;
	char name[32];
	u32 attr;
	vm::psv::ptr<SceKernelLwCondWork> pWork;
	vm::psv::ptr<SceKernelLwMutexWork> pLwMutex;
	u32 numWaitThreads;
};

// Thread Manager definitions (timers)

struct SceKernelTimerOptParam
{
	u32 size;
};

struct SceKernelTimerInfo
{
	u32 size;
	s32 timerId;
	char name[32];
	u32 attr;
	s32 fActive;
	SceKernelSysClock baseTime;
	SceKernelSysClock currentTime;
	SceKernelSysClock schedule;
	SceKernelSysClock interval;
	s32 type;
	s32 fRepeat;
	s32 numWaitThreads;
	s32 reserved[1];
};

// Thread Manager definitions (reader/writer locks)

struct SceKernelRWLockOptParam
{
	u32 size;
};

struct SceKernelRWLockInfo
{
	u32 size;
	s32 rwLockId;
	char name[32];
	u32 attr;
	s32 lockCount;
	s32 writeOwnerId;
	s32 numReadWaitThreads;
	s32 numWriteWaitThreads;
};

// IO/File Manager definitions

struct SceIoStat
{
	s32 mode;
	u32 attr;
	s64 size;
	SceDateTime ctime;
	SceDateTime atime;
	SceDateTime mtime;
	u64 _private[6];
};

struct SceIoDirent
{
	SceIoStat d_stat;
	char d_name[256];
	vm::psv::ptr<void> d_private;
	s32 dummy;
};

// Module

extern psv_log_base sceLibKernel;
