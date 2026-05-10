#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSync2.h"

#include "Utilities/StrUtil.h"

LOG_CHANNEL(cellSync2);

vm::gvar<CellSync2CallerThreadType> gCellSync2CallerThreadTypePpuThread;
vm::gvar<CellSync2Notifier> gCellSync2NotifierPpuThread;
vm::gvar<CellSync2CallerThreadType> gCellSync2CallerThreadTypePpuFiber;
vm::gvar<CellSync2Notifier> gCellSync2NotifierPpuFiber;
vm::gvar<CellSync2Notifier> gCellSync2NotifierSpursTask;
vm::gvar<CellSync2Notifier> gCellSync2NotifierSpursJobQueueJob;

template<>
void fmt_class_string<CellSync2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SYNC2_ERROR_AGAIN);
		STR_CASE(CELL_SYNC2_ERROR_INVAL);
		STR_CASE(CELL_SYNC2_ERROR_NOMEM);
		STR_CASE(CELL_SYNC2_ERROR_DEADLK);
		STR_CASE(CELL_SYNC2_ERROR_PERM);
		STR_CASE(CELL_SYNC2_ERROR_BUSY);
		STR_CASE(CELL_SYNC2_ERROR_STAT);
		STR_CASE(CELL_SYNC2_ERROR_ALIGN);
		STR_CASE(CELL_SYNC2_ERROR_NULL_POINTER);
		STR_CASE(CELL_SYNC2_ERROR_NOT_SUPPORTED_THREAD);
		STR_CASE(CELL_SYNC2_ERROR_NO_NOTIFIER);
		STR_CASE(CELL_SYNC2_ERROR_NO_SPU_CONTEXT_STORAGE);
		}

		return unknown;
	});
}

error_code _cellSync2MutexAttributeInitialize(vm::ptr<CellSync2MutexAttribute> attr, u32 sdkVersion)
{
	cellSync2.warning("_cellSync2MutexAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 15;
	attr->recursive = false;
	strcpy_trunc(attr->name, "CellSync2Mutex");

	return CELL_OK;
}

error_code cellSync2MutexEstimateBufferSize(vm::cptr<CellSync2MutexAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.todo("cellSync2MutexEstimateBufferSize(attr=*0x%x, bufferSize=*0x%x)", attr, bufferSize);

	if (!attr || !bufferSize)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->maxWaiters > 0x8000)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2MutexInitialize(vm::ptr<CellSync2Mutex> mutex, vm::ptr<void> buffer, vm::cptr<CellSync2MutexAttribute> attr)
{
	cellSync2.todo("cellSync2MutexInitialize(mutex=*0x%x, buffer=*0x%x, attr=*0x%x)", mutex, buffer, attr);

	if (!mutex || !attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if ((attr->maxWaiters > 0) && !buffer)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->maxWaiters > 0x8000)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2MutexFinalize(vm::ptr<CellSync2Mutex> mutex)
{
	cellSync2.todo("cellSync2MutexFinalize(mutex=*0x%x)", mutex);

	if (!mutex)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2MutexLock(vm::ptr<CellSync2Mutex> mutex, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2MutexLock(mutex=*0x%x, config=*0x%x)", mutex, config);

	if (!mutex)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2MutexTryLock(vm::ptr<CellSync2Mutex> mutex, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2MutexTryLock(mutex=*0x%x, config=*0x%x)", mutex, config);

	if (!mutex)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2MutexUnlock(vm::ptr<CellSync2Mutex> mutex, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2MutexUnlock(mutex=*0x%x, config=*0x%x)", mutex, config);

	if (!mutex)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code _cellSync2CondAttributeInitialize(vm::ptr<CellSync2CondAttribute> attr, u32 sdkVersion)
{
	cellSync2.warning("_cellSync2CondAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	attr->sdkVersion = sdkVersion;
	attr->maxWaiters = 15;
	strcpy_trunc(attr->name, "CellSync2Cond");

	return CELL_OK;
}

error_code cellSync2CondEstimateBufferSize(vm::cptr<CellSync2CondAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.todo("cellSync2CondEstimateBufferSize(attr=*0x%x, bufferSize=*0x%x)", attr, bufferSize);

	if (!attr || !bufferSize)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->maxWaiters == 0 || attr->maxWaiters > 0x8000)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2CondInitialize(vm::ptr<CellSync2Cond> cond, vm::ptr<CellSync2Mutex> mutex, vm::ptr<void> buffer, vm::cptr<CellSync2CondAttribute> attr)
{
	cellSync2.todo("cellSync2CondInitialize(cond=*0x%x, mutex=*0x%x, buffer=*0x%x, attr=*0x%x)", cond, mutex, buffer, attr);

	if (!cond || !mutex || !buffer || !attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->maxWaiters == 0)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2CondFinalize(vm::ptr<CellSync2Cond> cond)
{
	cellSync2.todo("cellSync2CondFinalize(cond=*0x%x)", cond);

	if (!cond)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2CondWait(vm::ptr<CellSync2Cond> cond, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2CondWait(cond=*0x%x, config=*0x%x)", cond, config);

	if (!cond)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2CondSignal(vm::ptr<CellSync2Cond> cond, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2CondSignal(cond=*0x%x, config=*0x%x)", cond, config);

	if (!cond)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2CondSignalAll(vm::ptr<CellSync2Cond> cond, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2CondSignalAll(cond=*0x%x, config=*0x%x)", cond, config);

	if (!cond)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code _cellSync2SemaphoreAttributeInitialize(vm::ptr<CellSync2SemaphoreAttribute> attr, u32 sdkVersion)
{
	cellSync2.warning("_cellSync2SemaphoreAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 1;
	strcpy_trunc(attr->name, "CellSync2Semaphore");

	return CELL_OK;
}

error_code cellSync2SemaphoreEstimateBufferSize(vm::cptr<CellSync2SemaphoreAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.todo("cellSync2SemaphoreEstimateBufferSize(attr=*0x%x, bufferSize=*0x%x)", attr, bufferSize);

	if (!attr || !bufferSize)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->maxWaiters == 0 || attr->maxWaiters > 0x8000)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2SemaphoreInitialize(vm::ptr<CellSync2Semaphore> semaphore, vm::ptr<void> buffer, s32 initialValue, vm::cptr<CellSync2SemaphoreAttribute> attr)
{
	cellSync2.todo("cellSync2SemaphoreInitialize(semaphore=*0x%x, buffer=*0x%x, initialValue=0x%x, attr=*0x%x)", semaphore, buffer, initialValue, attr);

	if (!semaphore || !attr || ((attr->maxWaiters >= 2) && !buffer))
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if ((initialValue > s32{0x7FFFFF}) || (initialValue < s32{-0x800000}) || (attr->maxWaiters == 0) || ((attr->maxWaiters == 1) && buffer))
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2SemaphoreFinalize(vm::ptr<CellSync2Semaphore> semaphore)
{
	cellSync2.todo("cellSync2SemaphoreFinalize(semaphore=*0x%x)", semaphore);

	if (!semaphore)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2SemaphoreAcquire(vm::ptr<CellSync2Semaphore> semaphore, u32 count, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2SemaphoreAcquire(semaphore=*0x%x, count=0x%x, config=*0x%x)", semaphore, count, config);

	if (!semaphore)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (count > 0x7FFFFF)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2SemaphoreTryAcquire(vm::ptr<CellSync2Semaphore> semaphore, u32 count, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2SemaphoreTryAcquire(semaphore=*0x%x, count=0x%x, config=*0x%x)", semaphore, count, config);

	if (!semaphore)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (count > 0x7FFFFF)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2SemaphoreRelease(vm::ptr<CellSync2Semaphore> semaphore, u32 count, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2SemaphoreRelease(semaphore=*0x%x, count=0x%x, config=*0x%x)", semaphore, count, config);

	if (!semaphore)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (count > 0x7FFFFF)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2SemaphoreGetCount(vm::ptr<CellSync2Semaphore> semaphore, vm::ptr<s32> count)
{
	cellSync2.todo("cellSync2SemaphoreGetCount(semaphore=*0x%x, count=*0x%x)", semaphore, count);

	if (!semaphore || !count)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code _cellSync2QueueAttributeInitialize(vm::ptr<CellSync2QueueAttribute> attr, u32 sdkVersion)
{
	cellSync2.warning("_cellSync2QueueAttributeInitialize(attr=*0x%x, sdkVersion=0x%x)", attr, sdkVersion);

	if (!attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->elementSize = 16;
	attr->depth = 1024;
	attr->maxPushWaiters = 15;
	attr->maxPopWaiters = 15;
	strcpy_trunc(attr->name, "CellSync2Queue");

	return CELL_OK;
}

error_code cellSync2QueueEstimateBufferSize(vm::cptr<CellSync2QueueAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.todo("cellSync2QueueEstimateBufferSize(attr=*0x%x, bufferSize=*0x%x)", attr, bufferSize);

	if (!attr || !bufferSize)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->elementSize == 0u || attr->elementSize > 0x4000u || attr->elementSize % 16u || attr->depth == 0u || attr->depth > 0xFFFFFFFCu ||
		attr->maxPushWaiters > 0x8000u || attr->maxPopWaiters > 0x8000u)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2QueueInitialize(vm::ptr<CellSync2Queue> queue, vm::ptr<void> buffer, vm::cptr<CellSync2QueueAttribute> attr)
{
	cellSync2.todo("cellSync2QueueInitialize(queue=*0x%x, buffer=*0x%x, attr=*0x%x)", queue, buffer, attr);

	if (!queue || !buffer || !attr)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	if (attr->elementSize == 0u || attr->elementSize > 0x4000u || attr->elementSize % 16u || attr->depth == 0u || attr->depth > 0xFFFFFFFCu ||
		attr->maxPushWaiters > 0x8000u || attr->maxPopWaiters > 0x8000u)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

error_code cellSync2QueueFinalize(vm::ptr<CellSync2Queue> queue)
{
	cellSync2.todo("cellSync2QueueFinalize(queue=*0x%x)", queue);

	if (!queue)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueuePush(vm::ptr<CellSync2Queue> queue, vm::cptr<void> data, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2QueuePush(queue=*0x%x, data=*0x%x, config=*0x%x)", queue, data, config);

	if (!queue || !data)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueueTryPush(vm::ptr<CellSync2Queue> queue, vm::cpptr<void> data, u32 numData, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2QueueTryPush(queue=*0x%x, data=**0x%x, numData=0x%x, config=*0x%x)", queue, data, numData, config);

	if (!queue || !data)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueuePop(vm::ptr<CellSync2Queue> queue, vm::ptr<void> buffer, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2QueuePop(queue=*0x%x, buffer=*0x%x, config=*0x%x)", queue, buffer, config);

	if (!queue || !buffer)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueueTryPop(vm::ptr<CellSync2Queue> queue, vm::ptr<void> buffer, vm::cptr<CellSync2ThreadConfig> config)
{
	cellSync2.todo("cellSync2QueueTryPop(queue=*0x%x, buffer=*0x%x, config=*0x%x)", queue, buffer, config);

	if (!queue || !buffer)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueueGetSize(vm::ptr<CellSync2Queue> queue, vm::ptr<u32> size)
{
	cellSync2.todo("cellSync2QueueGetSize(queue=*0x%x, size=*0x%x)", queue, size);

	if (!queue || !size)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

error_code cellSync2QueueGetDepth(vm::ptr<CellSync2Queue> queue, vm::ptr<u32> depth)
{
	cellSync2.todo("cellSync2QueueGetDepth(queue=*0x%x, depth=*0x%x)", queue, depth);

	if (!queue || !depth)
		return CELL_SYNC2_ERROR_NULL_POINTER;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSync2)("cellSync2", []()
{
	REG_VAR(cellSync2, gCellSync2CallerThreadTypePpuThread);
	REG_VAR(cellSync2, gCellSync2NotifierPpuThread);
	REG_VAR(cellSync2, gCellSync2CallerThreadTypePpuFiber);
	REG_VAR(cellSync2, gCellSync2NotifierPpuFiber);
	REG_VAR(cellSync2, gCellSync2NotifierSpursTask);
	REG_VAR(cellSync2, gCellSync2NotifierSpursJobQueueJob);

	REG_FUNC(cellSync2, _cellSync2MutexAttributeInitialize);
	REG_FUNC(cellSync2, cellSync2MutexEstimateBufferSize);
	REG_FUNC(cellSync2, cellSync2MutexInitialize);
	REG_FUNC(cellSync2, cellSync2MutexFinalize);
	REG_FUNC(cellSync2, cellSync2MutexLock);
	REG_FUNC(cellSync2, cellSync2MutexTryLock);
	REG_FUNC(cellSync2, cellSync2MutexUnlock);

	REG_FUNC(cellSync2, _cellSync2CondAttributeInitialize);
	REG_FUNC(cellSync2, cellSync2CondEstimateBufferSize);
	REG_FUNC(cellSync2, cellSync2CondInitialize);
	REG_FUNC(cellSync2, cellSync2CondFinalize);
	REG_FUNC(cellSync2, cellSync2CondWait);
	REG_FUNC(cellSync2, cellSync2CondSignal);
	REG_FUNC(cellSync2, cellSync2CondSignalAll);

	REG_FUNC(cellSync2, _cellSync2SemaphoreAttributeInitialize);
	REG_FUNC(cellSync2, cellSync2SemaphoreEstimateBufferSize);
	REG_FUNC(cellSync2, cellSync2SemaphoreInitialize);
	REG_FUNC(cellSync2, cellSync2SemaphoreFinalize);
	REG_FUNC(cellSync2, cellSync2SemaphoreAcquire);
	REG_FUNC(cellSync2, cellSync2SemaphoreTryAcquire);
	REG_FUNC(cellSync2, cellSync2SemaphoreRelease);
	REG_FUNC(cellSync2, cellSync2SemaphoreGetCount);

	REG_FUNC(cellSync2, _cellSync2QueueAttributeInitialize);
	REG_FUNC(cellSync2, cellSync2QueueEstimateBufferSize);
	REG_FUNC(cellSync2, cellSync2QueueInitialize);
	REG_FUNC(cellSync2, cellSync2QueueFinalize);
	REG_FUNC(cellSync2, cellSync2QueuePush);
	REG_FUNC(cellSync2, cellSync2QueueTryPush);
	REG_FUNC(cellSync2, cellSync2QueuePop);
	REG_FUNC(cellSync2, cellSync2QueueTryPop);
	REG_FUNC(cellSync2, cellSync2QueueGetSize);
	REG_FUNC(cellSync2, cellSync2QueueGetDepth);
});
