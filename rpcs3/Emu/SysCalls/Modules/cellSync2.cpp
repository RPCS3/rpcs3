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
	cellSync2.AddFunc(0x55836e73, _cellSync2MutexAttributeInitialize);
	cellSync2.AddFunc(0xd51bfae7, cellSync2MutexEstimateBufferSize);
	cellSync2.AddFunc(0xeb81a467, cellSync2MutexInitialize);
	cellSync2.AddFunc(0x27f2d61c, cellSync2MutexFinalize);
	cellSync2.AddFunc(0xa400d82e, cellSync2MutexLock);
	cellSync2.AddFunc(0xa69c749c, cellSync2MutexTryLock);
	cellSync2.AddFunc(0x0080fe88, cellSync2MutexUnlock);

	cellSync2.AddFunc(0xdf3c532a, _cellSync2CondAttributeInitialize);
	cellSync2.AddFunc(0x5b1e4d7a, cellSync2CondEstimateBufferSize);
	cellSync2.AddFunc(0x58be9a0f, cellSync2CondInitialize);
	cellSync2.AddFunc(0x63062249, cellSync2CondFinalize);
	cellSync2.AddFunc(0xbc96d751, cellSync2CondWait);
	cellSync2.AddFunc(0x871af804, cellSync2CondSignal);
	cellSync2.AddFunc(0x8aae07c2, cellSync2CondSignalAll);

	cellSync2.AddFunc(0x2d77fe17, _cellSync2SemaphoreAttributeInitialize);
	cellSync2.AddFunc(0x74c2780f, cellSync2SemaphoreEstimateBufferSize);
	cellSync2.AddFunc(0xc5dee254, cellSync2SemaphoreInitialize);
	cellSync2.AddFunc(0x164843a7, cellSync2SemaphoreFinalize);
	cellSync2.AddFunc(0xd1b0d146, cellSync2SemaphoreAcquire);
	cellSync2.AddFunc(0x5e4b0f87, cellSync2SemaphoreTryAcquire);
	cellSync2.AddFunc(0x0c2983ac, cellSync2SemaphoreRelease);
	cellSync2.AddFunc(0x4e2ee031, cellSync2SemaphoreGetCount);

	cellSync2.AddFunc(0x5e00d433, _cellSync2QueueAttributeInitialize);
	cellSync2.AddFunc(0xc08cc0f9, cellSync2QueueEstimateBufferSize);
	cellSync2.AddFunc(0xf125e044, cellSync2QueueInitialize);
	cellSync2.AddFunc(0x6af85cdf, cellSync2QueueFinalize);
	cellSync2.AddFunc(0x7d967d91, cellSync2QueuePush);
	cellSync2.AddFunc(0x7fd479fe, cellSync2QueueTryPush);
	cellSync2.AddFunc(0xd83ab0c9, cellSync2QueuePop);
	cellSync2.AddFunc(0x0c9a0ea9, cellSync2QueueTryPop);
	cellSync2.AddFunc(0x12f0a27d, cellSync2QueueGetSize);
	cellSync2.AddFunc(0xf0e1471c, cellSync2QueueGetDepth);
});
