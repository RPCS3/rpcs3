#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSync2.h"

extern Module cellSync2;

s32 _cellSync2MutexAttributeInitialize(vm::ptr<CellSync2MutexAttribute> attr, u32 sdkVersion)
{
	cellSync2.Warning("_cellSync2MutexAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 15;
	attr->recursive = false;
	strcpy_trunc(attr->name, "CellSync2Mutex");

	return CELL_OK;
}

s32 cellSync2MutexEstimateBufferSize(vm::ptr<const CellSync2MutexAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.Todo("cellSync2MutexEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

s32 cellSync2MutexInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2MutexFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2MutexLock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2MutexTryLock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2MutexUnlock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 _cellSync2CondAttributeInitialize(vm::ptr<CellSync2CondAttribute> attr, u32 sdkVersion)
{
	cellSync2.Warning("_cellSync2CondAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->maxWaiters = 15;
	strcpy_trunc(attr->name, "CellSync2Cond");

	return CELL_OK;
}

s32 cellSync2CondEstimateBufferSize(vm::ptr<const CellSync2CondAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.Todo("cellSync2CondEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters == 0 || attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

s32 cellSync2CondInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2CondFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2CondWait()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2CondSignal()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2CondSignalAll()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 _cellSync2SemaphoreAttributeInitialize(vm::ptr<CellSync2SemaphoreAttribute> attr, u32 sdkVersion)
{
	cellSync2.Warning("_cellSync2SemaphoreAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

	attr->sdkVersion = sdkVersion;
	attr->threadTypes = CELL_SYNC2_THREAD_TYPE_PPU_THREAD | CELL_SYNC2_THREAD_TYPE_PPU_FIBER |
						CELL_SYNC2_THREAD_TYPE_SPURS_TASK | CELL_SYNC2_THREAD_TYPE_SPURS_JOB |
						CELL_SYNC2_THREAD_TYPE_SPURS_JOBQUEUE_JOB;
	attr->maxWaiters = 1;
	strcpy_trunc(attr->name, "CellSync2Semaphore");

	return CELL_OK;
}

s32 cellSync2SemaphoreEstimateBufferSize(vm::ptr<const CellSync2SemaphoreAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.Todo("cellSync2SemaphoreEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->maxWaiters == 0 || attr->maxWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

s32 cellSync2SemaphoreInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2SemaphoreFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2SemaphoreAcquire()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2SemaphoreTryAcquire()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2SemaphoreRelease()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2SemaphoreGetCount()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 _cellSync2QueueAttributeInitialize(vm::ptr<CellSync2QueueAttribute> attr, u32 sdkVersion)
{
	cellSync2.Warning("_cellSync2QueueAttributeInitialize(attr_addr=0x%x, sdkVersion=0x%x)", attr.addr(), sdkVersion);

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

s32 cellSync2QueueEstimateBufferSize(vm::ptr<const CellSync2QueueAttribute> attr, vm::ptr<u32> bufferSize)
{
	cellSync2.Todo("cellSync2QueueEstimateBufferSize(attr_addr=0x%x, bufferSize_addr=0x%x)", attr.addr(), bufferSize.addr());

	if (attr->elementSize == 0 || attr->elementSize > 16384 || attr->elementSize % 16 || attr->depth == 0 || attr->depth > 4294967292 ||
		attr->maxPushWaiters > 32768 || attr->maxPopWaiters > 32768)
		return CELL_SYNC2_ERROR_INVAL;

	return CELL_OK;
}

s32 cellSync2QueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueueFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueuePush()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueueTryPush()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueuePop()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueueTryPop()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueueGetSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

s32 cellSync2QueueGetDepth()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

Module cellSync2("cellSync2", []()
{
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
