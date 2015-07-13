#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceLibKernel.h"

extern psv_log_base sceUlt;

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " size")

struct _SceUltOptParamHeader
{
	s64 reserved[2];
};

struct SceUltWaitingQueueResourcePoolOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltWaitingQueueResourcePoolOptParam, 128);

struct SceUltWaitingQueueResourcePool
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltWaitingQueueResourcePool, 256);

struct SceUltQueueDataResourcePoolOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltQueueDataResourcePoolOptParam, 128);

struct SceUltQueueDataResourcePool
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltQueueDataResourcePool, 256);

struct SceUltMutexOptParam
{
	_SceUltOptParamHeader header;
	u32 attribute;
	u32 reserved_0;
	u64 reserved[13];
};

CHECK_SIZE(SceUltMutexOptParam, 128);

struct SceUltMutex
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltMutex, 256);

struct SceUltConditionVariableOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltConditionVariableOptParam, 128);

struct SceUltConditionVariable
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltConditionVariable, 256);

struct SceUltQueueOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltQueueOptParam, 128);

struct SceUltQueue
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltQueue, 256);

struct SceUltReaderWriterLockOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltReaderWriterLockOptParam, 128);

struct SceUltReaderWriterLock
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltReaderWriterLock, 256);

struct SceUltSemaphoreOptParam
{
	_SceUltOptParamHeader header;
	u64 reserved[14];
};

CHECK_SIZE(SceUltSemaphoreOptParam, 128);

struct SceUltSemaphore
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltSemaphore, 256);

struct SceUltUlthreadRuntimeOptParam
{
	_SceUltOptParamHeader header;

	u32 oneShotThreadStackSize;
	s32 workerThreadPriority;
	u32 workerThreadCpuAffinityMask;
	u32 workerThreadAttr;
	vm::psv::ptr<const SceKernelThreadOptParam> workerThreadOptParam;

	u64 reserved[11];
};

CHECK_SIZE(SceUltUlthreadRuntimeOptParam, 128);

struct SceUltUlthreadRuntime
{
	u64 reserved[128];
};

CHECK_SIZE(SceUltUlthreadRuntime, 1024);

struct SceUltUlthreadOptParam
{
	_SceUltOptParamHeader header;
	u32 attribute;
	u32 reserved_0;
	u64 reserved[13];
};

CHECK_SIZE(SceUltUlthreadOptParam, 128);

struct SceUltUlthread
{
	u64 reserved[32];
};

CHECK_SIZE(SceUltUlthread, 256);

typedef vm::psv::ptr<s32(u32 arg)> SceUltUlthreadEntry;

// Functions

s32 _sceUltWaitingQueueResourcePoolOptParamInitialize(vm::psv::ptr<SceUltWaitingQueueResourcePoolOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltWaitingQueueResourcePoolGetWorkAreaSize(u32 numThreads, u32 numSyncObjects)
{
	throw __FUNCTION__;
}

s32 _sceUltWaitingQueueResourcePoolCreate(
	vm::psv::ptr<SceUltWaitingQueueResourcePool> pool,
	vm::psv::ptr<const char> name,
	u32 numThreads,
	u32 numSyncObjects,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltWaitingQueueResourcePoolOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltWaitingQueueResourcePoolDestroy(vm::psv::ptr<SceUltWaitingQueueResourcePool> pool)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueDataResourcePoolOptParamInitialize(vm::psv::ptr<SceUltQueueDataResourcePoolOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltQueueDataResourcePoolGetWorkAreaSize(u32 numData, u32 dataSize, u32 numQueueObject)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueDataResourcePoolCreate(
	vm::psv::ptr<SceUltQueueDataResourcePool> pool,
	vm::psv::ptr<const char> name,
	u32 numData,
	u32 dataSize,
	u32 numQueueObject,
	vm::psv::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltQueueDataResourcePoolOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltQueueDataResourcePoolDestroy(vm::psv::ptr<SceUltQueueDataResourcePool> pool)
{
	throw __FUNCTION__;
}

u32 sceUltMutexGetStandaloneWorkAreaSize(u32 waitingQueueDepth, u32 numConditionVariable)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexOptParamInitialize(vm::psv::ptr<SceUltMutexOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexCreate(
	vm::psv::ptr<SceUltMutex> mutex,
	vm::psv::ptr<const char> name,
	vm::psv::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::psv::ptr<const SceUltMutexOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexCreateStandalone(
	vm::psv::ptr<SceUltMutex> mutex,
	vm::psv::ptr<const char> name,
	u32 numConditionVariable,
	u32 maxNumThreads,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltMutexOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltMutexLock(vm::psv::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexTryLock(vm::psv::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexUnlock(vm::psv::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexDestroy(vm::psv::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 _sceUltConditionVariableOptParamInitialize(vm::psv::ptr<SceUltConditionVariableOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltConditionVariableCreate(
	vm::psv::ptr<SceUltConditionVariable> conditionVariable,
	vm::psv::ptr<const char> name,
	vm::psv::ptr<SceUltMutex> mutex,
	vm::psv::ptr<const SceUltConditionVariableOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableSignal(vm::psv::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableSignalAll(vm::psv::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableWait(vm::psv::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableDestroy(vm::psv::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueOptParamInitialize(vm::psv::ptr<SceUltQueueOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltQueueGetStandaloneWorkAreaSize(u32 queueDepth,
	u32 dataSize,
	u32 waitingQueueLength)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueCreate(
	vm::psv::ptr<SceUltQueue> queue,
	vm::psv::ptr<const char> _name,
	u32 dataSize,
	vm::psv::ptr<SceUltWaitingQueueResourcePool> resourcePool,
	vm::psv::ptr<SceUltQueueDataResourcePool> queueResourcePool,
	vm::psv::ptr<const SceUltQueueOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueCreateStandalone(
	vm::psv::ptr<SceUltQueue> queue,
	vm::psv::ptr<const char> name,
	u32 queueDepth,
	u32 dataSize,
	u32 waitingQueueLength,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltQueueOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltQueuePush(vm::psv::ptr<SceUltQueue> queue, vm::psv::ptr<const void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueTryPush(vm::psv::ptr<SceUltQueue> queue, vm::psv::ptr<const void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueuePop(vm::psv::ptr<SceUltQueue> queue, vm::psv::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueTryPop(vm::psv::ptr<SceUltQueue> queue, vm::psv::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueDestroy(vm::psv::ptr<SceUltQueue> queue)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockOptParamInitialize(vm::psv::ptr<SceUltReaderWriterLockOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockCreate(
	vm::psv::ptr<SceUltReaderWriterLock> rwlock,
	vm::psv::ptr<const char> name,
	vm::psv::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::psv::ptr<const SceUltReaderWriterLockOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockCreateStandalone(
	vm::psv::ptr<SceUltReaderWriterLock> rwlock,
	vm::psv::ptr<const char> name,
	u32 waitingQueueDepth,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltReaderWriterLockOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltReaderWriterLockGetStandaloneWorkAreaSize(u32 waitingQueueDepth)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockLockRead(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockTryLockRead(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockUnlockRead(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockLockWrite(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockTryLockWrite(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockUnlockWrite(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockDestroy(vm::psv::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 _sceUltSemaphoreOptParamInitialize(vm::psv::ptr<SceUltSemaphoreOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltSemaphoreCreate(
	vm::psv::ptr<SceUltSemaphore> semaphore,
	vm::psv::ptr<const char> name,
	s32 numInitialResource,
	vm::psv::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::psv::ptr<const SceUltSemaphoreOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreAcquire(vm::psv::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreTryAcquire(vm::psv::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreRelease(vm::psv::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreDestroy(vm::psv::ptr<SceUltSemaphore> semaphore)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadRuntimeOptParamInitialize(vm::psv::ptr<SceUltUlthreadRuntimeOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltUlthreadRuntimeGetWorkAreaSize(u32 numMaxUlthread, u32 numWorkerThread)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadRuntimeCreate(
	vm::psv::ptr<SceUltUlthreadRuntime> runtime,
	vm::psv::ptr<const char> name,
	u32 numMaxUlthread,
	u32 numWorkerThread,
	vm::psv::ptr<void> workArea,
	vm::psv::ptr<const SceUltUlthreadRuntimeOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadRuntimeDestroy(vm::psv::ptr<SceUltUlthreadRuntime> runtime)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadOptParamInitialize(vm::psv::ptr<SceUltUlthreadOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadCreate(
	vm::psv::ptr<SceUltUlthread> ulthread,
	vm::psv::ptr<const char> name,
	SceUltUlthreadEntry entry,
	u32 arg,
	vm::psv::ptr<void> context,
	u32 sizeContext,
	vm::psv::ptr<SceUltUlthreadRuntime> runtime,
	vm::psv::ptr<const SceUltUlthreadOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadYield()
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadExit(s32 status)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadJoin(vm::psv::ptr<SceUltUlthread> ulthread, vm::psv::ptr<s32> status)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadTryJoin(vm::psv::ptr<SceUltUlthread> ulthread, vm::psv::ptr<s32> status)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadGetSelf(vm::psv::pptr<SceUltUlthread> ulthread)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceUlt, #name, name)

psv_log_base sceUlt("SceUlt", []()
{
	sceUlt.on_load = nullptr;
	sceUlt.on_unload = nullptr;
	sceUlt.on_stop = nullptr;

	REG_FUNC(0xEF094E35, _sceUltWaitingQueueResourcePoolOptParamInitialize);
	REG_FUNC(0x644DA029, sceUltWaitingQueueResourcePoolGetWorkAreaSize);
	REG_FUNC(0x62F9493E, _sceUltWaitingQueueResourcePoolCreate);
	REG_FUNC(0xC9E96714, sceUltWaitingQueueResourcePoolDestroy);
	REG_FUNC(0x8A4F88A2, _sceUltQueueDataResourcePoolOptParamInitialize);
	REG_FUNC(0xECDA7FEE, sceUltQueueDataResourcePoolGetWorkAreaSize);
	REG_FUNC(0x40856827, _sceUltQueueDataResourcePoolCreate);
	REG_FUNC(0x2B8D33F1, sceUltQueueDataResourcePoolDestroy);
	REG_FUNC(0x24D87E05, _sceUltMutexOptParamInitialize);
	REG_FUNC(0x5AFEC7A1, _sceUltMutexCreate);
	REG_FUNC(0x001EAC8A, sceUltMutexLock);
	REG_FUNC(0xE5936A69, sceUltMutexTryLock);
	REG_FUNC(0x897C9097, sceUltMutexUnlock);
	REG_FUNC(0xEEBD9052, sceUltMutexDestroy);
	REG_FUNC(0x0603FCC1, _sceUltConditionVariableOptParamInitialize);
	REG_FUNC(0xD76A156C, _sceUltConditionVariableCreate);
	REG_FUNC(0x9FE7CB9F, sceUltConditionVariableSignal);
	REG_FUNC(0xEBB6FC1E, sceUltConditionVariableSignalAll);
	REG_FUNC(0x2CD0F57C, sceUltConditionVariableWait);
	REG_FUNC(0x53420ED2, sceUltConditionVariableDestroy);
	REG_FUNC(0xF7A83023, _sceUltQueueOptParamInitialize);
	REG_FUNC(0x14DA1BB4, _sceUltQueueCreate);
	REG_FUNC(0xA7E78FF9, sceUltQueuePush);
	REG_FUNC(0x6D356B29, sceUltQueueTryPush);
	REG_FUNC(0x1AD58A53, sceUltQueuePop);
	REG_FUNC(0x2A1A8EA6, sceUltQueueTryPop);
	REG_FUNC(0xF37862DE, sceUltQueueDestroy);
	REG_FUNC(0xD8334A1F, _sceUltReaderWriterLockOptParamInitialize);
	REG_FUNC(0x2FB0EB32, _sceUltReaderWriterLockCreate);
	REG_FUNC(0x9AD07630, sceUltReaderWriterLockLockRead);
	REG_FUNC(0x2629C055, sceUltReaderWriterLockTryLockRead);
	REG_FUNC(0x218D4743, sceUltReaderWriterLockUnlockRead);
	REG_FUNC(0xF5F63E2C, sceUltReaderWriterLockLockWrite);
	REG_FUNC(0x944FB222, sceUltReaderWriterLockTryLockWrite);
	REG_FUNC(0x2A5741F5, sceUltReaderWriterLockUnlockWrite);
	REG_FUNC(0xB1FEB79B, sceUltReaderWriterLockDestroy);
	REG_FUNC(0x8E31B9FE, _sceUltSemaphoreOptParamInitialize);
	REG_FUNC(0xDD59562C, _sceUltSemaphoreCreate);
	REG_FUNC(0xF220D3AE, sceUltSemaphoreAcquire);
	REG_FUNC(0xAF15606D, sceUltSemaphoreTryAcquire);
	REG_FUNC(0x65376E2D, sceUltSemaphoreRelease);
	REG_FUNC(0x8EC57420, sceUltSemaphoreDestroy);
	REG_FUNC(0x8486DDE6, _sceUltUlthreadRuntimeOptParamInitialize);
	REG_FUNC(0x5435C586, sceUltUlthreadRuntimeGetWorkAreaSize);
	REG_FUNC(0x86DDA3AE, _sceUltUlthreadRuntimeCreate);
	REG_FUNC(0x4E9A745C, sceUltUlthreadRuntimeDestroy);
	REG_FUNC(0x7F373376, _sceUltUlthreadOptParamInitialize);
	REG_FUNC(0xB1290375, _sceUltUlthreadCreate);
	REG_FUNC(0xCAD57BAD, sceUltUlthreadYield);
	REG_FUNC(0x1E401DF8, sceUltUlthreadExit);
	REG_FUNC(0x63483381, sceUltUlthreadJoin);
	REG_FUNC(0xB4CF88AC, sceUltUlthreadTryJoin);
	REG_FUNC(0xA798C5D7, sceUltUlthreadGetSelf);
});
