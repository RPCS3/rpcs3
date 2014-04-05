#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSync2_init();
Module cellSync2(0x0055, cellSync2_init);

// Return Codes
enum
{
	CELL_SYNC2_ERROR_AGAIN                  = 0x80410C01,
	CELL_SYNC2_ERROR_INVAL                  = 0x80410C02,
	CELL_SYNC2_ERROR_NOMEM                  = 0x80410C04,
	CELL_SYNC2_ERROR_DEADLK                 = 0x80410C08,
	CELL_SYNC2_ERROR_PERM                   = 0x80410C09,
	CELL_SYNC2_ERROR_BUSY                   = 0x80410C0A,
	CELL_SYNC2_ERROR_STAT                   = 0x80410C0F,
	CELL_SYNC2_ERROR_ALIGN                  = 0x80410C10,
	CELL_SYNC2_ERROR_NULL_POINTER           = 0x80410C11,
	CELL_SYNC2_ERROR_NOT_SUPPORTED_THREAD   = 0x80410C12,
	CELL_SYNC2_ERROR_NO_NOTIFIER            = 0x80410C13,
	CELL_SYNC2_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410C14,
};

int _cellSync2MutexAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexEstimateBufferSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexLock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexTryLock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2MutexUnlock()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int _cellSync2CondAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondEstimateBufferSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondWait()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondSignal()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2CondSignalAll()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int _cellSync2SemaphoreAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreEstimateBufferSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreAcquire()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreTryAcquire()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreRelease()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2SemaphoreGetCount()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int _cellSync2QueueAttributeInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueEstimateBufferSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueInitialize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueFinalize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueuePush()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueTryPush()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueuePop()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueTryPop()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueGetSize()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

int cellSync2QueueGetDepth()
{
	UNIMPLEMENTED_FUNC(cellSync2);
	return CELL_OK;
}

void cellSync2_init()
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
}