#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceUlt.h"

// Functions

s32 _sceUltWaitingQueueResourcePoolOptParamInitialize(vm::ptr<SceUltWaitingQueueResourcePoolOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltWaitingQueueResourcePoolGetWorkAreaSize(u32 numThreads, u32 numSyncObjects)
{
	throw __FUNCTION__;
}

s32 _sceUltWaitingQueueResourcePoolCreate(
	vm::ptr<SceUltWaitingQueueResourcePool> pool,
	vm::ptr<const char> name,
	u32 numThreads,
	u32 numSyncObjects,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltWaitingQueueResourcePoolOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltWaitingQueueResourcePoolDestroy(vm::ptr<SceUltWaitingQueueResourcePool> pool)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueDataResourcePoolOptParamInitialize(vm::ptr<SceUltQueueDataResourcePoolOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltQueueDataResourcePoolGetWorkAreaSize(u32 numData, u32 dataSize, u32 numQueueObject)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueDataResourcePoolCreate(
	vm::ptr<SceUltQueueDataResourcePool> pool,
	vm::ptr<const char> name,
	u32 numData,
	u32 dataSize,
	u32 numQueueObject,
	vm::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltQueueDataResourcePoolOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltQueueDataResourcePoolDestroy(vm::ptr<SceUltQueueDataResourcePool> pool)
{
	throw __FUNCTION__;
}

u32 sceUltMutexGetStandaloneWorkAreaSize(u32 waitingQueueDepth, u32 numConditionVariable)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexOptParamInitialize(vm::ptr<SceUltMutexOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexCreate(
	vm::ptr<SceUltMutex> mutex,
	vm::ptr<const char> name,
	vm::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::ptr<const SceUltMutexOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltMutexCreateStandalone(
	vm::ptr<SceUltMutex> mutex,
	vm::ptr<const char> name,
	u32 numConditionVariable,
	u32 maxNumThreads,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltMutexOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltMutexLock(vm::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexTryLock(vm::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexUnlock(vm::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 sceUltMutexDestroy(vm::ptr<SceUltMutex> mutex)
{
	throw __FUNCTION__;
}

s32 _sceUltConditionVariableOptParamInitialize(vm::ptr<SceUltConditionVariableOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltConditionVariableCreate(
	vm::ptr<SceUltConditionVariable> conditionVariable,
	vm::ptr<const char> name,
	vm::ptr<SceUltMutex> mutex,
	vm::ptr<const SceUltConditionVariableOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableSignal(vm::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableSignalAll(vm::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableWait(vm::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 sceUltConditionVariableDestroy(vm::ptr<SceUltConditionVariable> conditionVariable)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueOptParamInitialize(vm::ptr<SceUltQueueOptParam> optParam, u32 buildVersion)
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
	vm::ptr<SceUltQueue> queue,
	vm::ptr<const char> _name,
	u32 dataSize,
	vm::ptr<SceUltWaitingQueueResourcePool> resourcePool,
	vm::ptr<SceUltQueueDataResourcePool> queueResourcePool,
	vm::ptr<const SceUltQueueOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltQueueCreateStandalone(
	vm::ptr<SceUltQueue> queue,
	vm::ptr<const char> name,
	u32 queueDepth,
	u32 dataSize,
	u32 waitingQueueLength,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltQueueOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltQueuePush(vm::ptr<SceUltQueue> queue, vm::ptr<const void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueTryPush(vm::ptr<SceUltQueue> queue, vm::ptr<const void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueuePop(vm::ptr<SceUltQueue> queue, vm::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueTryPop(vm::ptr<SceUltQueue> queue, vm::ptr<void> data)
{
	throw __FUNCTION__;
}

s32 sceUltQueueDestroy(vm::ptr<SceUltQueue> queue)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockOptParamInitialize(vm::ptr<SceUltReaderWriterLockOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockCreate(
	vm::ptr<SceUltReaderWriterLock> rwlock,
	vm::ptr<const char> name,
	vm::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::ptr<const SceUltReaderWriterLockOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltReaderWriterLockCreateStandalone(
	vm::ptr<SceUltReaderWriterLock> rwlock,
	vm::ptr<const char> name,
	u32 waitingQueueDepth,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltReaderWriterLockOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltReaderWriterLockGetStandaloneWorkAreaSize(u32 waitingQueueDepth)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockLockRead(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockTryLockRead(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockUnlockRead(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockLockWrite(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockTryLockWrite(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockUnlockWrite(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 sceUltReaderWriterLockDestroy(vm::ptr<SceUltReaderWriterLock> rwlock)
{
	throw __FUNCTION__;
}

s32 _sceUltSemaphoreOptParamInitialize(vm::ptr<SceUltSemaphoreOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltSemaphoreCreate(
	vm::ptr<SceUltSemaphore> semaphore,
	vm::ptr<const char> name,
	s32 numInitialResource,
	vm::ptr<SceUltWaitingQueueResourcePool> waitingQueueResourcePool,
	vm::ptr<const SceUltSemaphoreOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreAcquire(vm::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreTryAcquire(vm::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreRelease(vm::ptr<SceUltSemaphore> semaphore, s32 numResource)
{
	throw __FUNCTION__;
}

s32 sceUltSemaphoreDestroy(vm::ptr<SceUltSemaphore> semaphore)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadRuntimeOptParamInitialize(vm::ptr<SceUltUlthreadRuntimeOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

u32 sceUltUlthreadRuntimeGetWorkAreaSize(u32 numMaxUlthread, u32 numWorkerThread)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadRuntimeCreate(
	vm::ptr<SceUltUlthreadRuntime> runtime,
	vm::ptr<const char> name,
	u32 numMaxUlthread,
	u32 numWorkerThread,
	vm::ptr<void> workArea,
	vm::ptr<const SceUltUlthreadRuntimeOptParam> optParam,
	u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadRuntimeDestroy(vm::ptr<SceUltUlthreadRuntime> runtime)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadOptParamInitialize(vm::ptr<SceUltUlthreadOptParam> optParam, u32 buildVersion)
{
	throw __FUNCTION__;
}

s32 _sceUltUlthreadCreate(
	vm::ptr<SceUltUlthread> ulthread,
	vm::ptr<const char> name,
	vm::ptr<SceUltUlthreadEntry> entry,
	u32 arg,
	vm::ptr<void> context,
	u32 sizeContext,
	vm::ptr<SceUltUlthreadRuntime> runtime,
	vm::ptr<const SceUltUlthreadOptParam> optParam,
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

s32 sceUltUlthreadJoin(vm::ptr<SceUltUlthread> ulthread, vm::ptr<s32> status)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadTryJoin(vm::ptr<SceUltUlthread> ulthread, vm::ptr<s32> status)
{
	throw __FUNCTION__;
}

s32 sceUltUlthreadGetSelf(vm::pptr<SceUltUlthread> ulthread)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceUlt, #name, name)

psv_log_base sceUlt("SceUlt", []()
{
	sceUlt.on_load = nullptr;
	sceUlt.on_unload = nullptr;
	sceUlt.on_stop = nullptr;
	sceUlt.on_error = nullptr;

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
