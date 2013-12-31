#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSync_init();
Module cellSync(0x0054, cellSync_init);

// Return Codes
enum
{
	CELL_SYNC_ERROR_AGAIN					= 0x80410101,
	CELL_SYNC_ERROR_INVAL					= 0x80410102,
	CELL_SYNC_ERROR_NOMEM					= 0x80410104,
	CELL_SYNC_ERROR_DEADLK					= 0x80410108,
	CELL_SYNC_ERROR_PERM					= 0x80410109,
	CELL_SYNC_ERROR_BUSY					= 0x8041010A,
	CELL_SYNC_ERROR_STAT					= 0x8041010F,
	CELL_SYNC_ERROR_ALIGN					= 0x80410110,
	CELL_SYNC_ERROR_NULL_POINTER			= 0x80410111,
	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD	= 0x80410112,
	CELL_SYNC_ERROR_NO_NOTIFIER				= 0x80410113,
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE	= 0x80410114,
};

int cellSyncMutexInitialize(mem32_t mutex)
{
	const u32 mutex_addr = mutex.GetAddr();
	if (!mutex_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex_addr % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	mutex = 0;
	return CELL_OK;
}

int cellSyncMutexLock(mem32_t mutex)
{
	const u32 mutex_addr = mutex.GetAddr();
	if (!mutex_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex_addr % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	//aggressive spin-wait
	while (_InterlockedExchange((volatile long*)(Memory + mutex_addr), 1));
	_mm_lfence();
	return CELL_OK;
}

int cellSyncMutexTryLock(mem32_t mutex)
{
	const u32 mutex_addr = mutex.GetAddr();
	if (!mutex_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex_addr % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (_InterlockedExchange((volatile long*)(Memory + mutex_addr), 1))
	{
		return CELL_SYNC_ERROR_BUSY;
	}
	_mm_lfence();
	return CELL_OK;
}

int cellSyncMutexUnlock(mem32_t mutex)
{
	const u32 mutex_addr = mutex.GetAddr();
	if (!mutex_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex_addr % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	_mm_sfence();
	mutex = 0;
	return CELL_OK;
}

void cellSync_init()
{
	cellSync.AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync.AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync.AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync.AddFunc(0x91f2b7b0, cellSyncMutexUnlock);
}