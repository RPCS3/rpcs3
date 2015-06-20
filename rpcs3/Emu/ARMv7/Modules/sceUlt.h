#pragma once

#include "sceLibKernel.h"

struct _SceUltOptParamHeader
{
	le_t<s64> reserved[2];
};

struct SceUltWaitingQueueResourcePoolOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltWaitingQueueResourcePoolOptParam, 128);

struct SceUltWaitingQueueResourcePool
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltWaitingQueueResourcePool, 256);

struct SceUltQueueDataResourcePoolOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltQueueDataResourcePoolOptParam, 128);

struct SceUltQueueDataResourcePool
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltQueueDataResourcePool, 256);

struct SceUltMutexOptParam
{
	_SceUltOptParamHeader header;
	le_t<u32> attribute;
	le_t<u32> reserved_0;
	le_t<u64> reserved[13];
};

CHECK_SIZE(SceUltMutexOptParam, 128);

struct SceUltMutex
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltMutex, 256);

struct SceUltConditionVariableOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltConditionVariableOptParam, 128);

struct SceUltConditionVariable
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltConditionVariable, 256);

struct SceUltQueueOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltQueueOptParam, 128);

struct SceUltQueue
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltQueue, 256);

struct SceUltReaderWriterLockOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltReaderWriterLockOptParam, 128);

struct SceUltReaderWriterLock
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltReaderWriterLock, 256);

struct SceUltSemaphoreOptParam
{
	_SceUltOptParamHeader header;
	le_t<u64> reserved[14];
};

CHECK_SIZE(SceUltSemaphoreOptParam, 128);

struct SceUltSemaphore
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltSemaphore, 256);

struct SceUltUlthreadRuntimeOptParam
{
	_SceUltOptParamHeader header;

	le_t<u32> oneShotThreadStackSize;
	le_t<s32> workerThreadPriority;
	le_t<u32> workerThreadCpuAffinityMask;
	le_t<u32> workerThreadAttr;
	vm::lcptr<SceKernelThreadOptParam> workerThreadOptParam;

	le_t<u64> reserved[11];
};

CHECK_SIZE(SceUltUlthreadRuntimeOptParam, 128);

struct SceUltUlthreadRuntime
{
	le_t<u64> reserved[128];
};

CHECK_SIZE(SceUltUlthreadRuntime, 1024);

struct SceUltUlthreadOptParam
{
	_SceUltOptParamHeader header;
	le_t<u32> attribute;
	le_t<u32> reserved_0;
	le_t<u64> reserved[13];
};

CHECK_SIZE(SceUltUlthreadOptParam, 128);

struct SceUltUlthread
{
	le_t<u64> reserved[32];
};

CHECK_SIZE(SceUltUlthread, 256);

using SceUltUlthreadEntry = func_def<s32(u32 arg)>;

extern psv_log_base sceUlt;
