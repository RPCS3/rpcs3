#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSync_init();
Module cellSync("cellSync", cellSync_init);

// Return Codes
enum
{
	CELL_SYNC_ERROR_AGAIN                  = 0x80410101,
	CELL_SYNC_ERROR_INVAL                  = 0x80410102,
	CELL_SYNC_ERROR_NOMEM                  = 0x80410104,
	CELL_SYNC_ERROR_DEADLK                 = 0x80410108,
	CELL_SYNC_ERROR_PERM                   = 0x80410109,
	CELL_SYNC_ERROR_BUSY                   = 0x8041010A,
	CELL_SYNC_ERROR_STAT                   = 0x8041010F,
	CELL_SYNC_ERROR_ALIGN                  = 0x80410110,
	CELL_SYNC_ERROR_NULL_POINTER           = 0x80410111,
	CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD   = 0x80410112,
	CELL_SYNC_ERROR_NO_NOTIFIER            = 0x80410113,
	CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE = 0x80410114,
};

struct CellSyncMutex
{
	be_t<u16> m_freed;
	be_t<u16> m_order;

	volatile u32& m_data()
	{
		return *reinterpret_cast<u32*>(this);
	};
	/*
	(???) Initialize: set zeros
	(???) Lock: increase m_order and wait until m_freed == old m_order
	(???) Unlock: increase m_freed
	(???) TryLock: ?????
	*/
};

static_assert(sizeof(CellSyncMutex) == 4, "CellSyncMutex: wrong sizeof");

int cellSyncMutexInitialize(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexInitialize(mutex=0x%x)", mutex.GetAddr());

	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->m_data() = 0;
	return CELL_OK;
}

int cellSyncMutexLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexLock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	be_t<u16> old_order;
	while (true)
	{
		const u32 old_data = mutex->m_data();
		CellSyncMutex new_mutex;
		new_mutex.m_data() = old_data;

		old_order = new_mutex.m_order;
		new_mutex.m_order++;
		if (InterlockedCompareExchange(&mutex->m_data(), new_mutex.m_data(), old_data) == old_data) break;
	}

	while (old_order != mutex->m_freed) 
	{
		Sleep(1);
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellSyncMutexLock(mutex=0x%x) aborted", mutex.GetAddr());
			break;
		}
	}
	_mm_mfence();
	return CELL_OK;
}

int cellSyncMutexTryLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexTryLock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	int res;

	while (true)
	{
		const u32 old_data = mutex->m_data();
		CellSyncMutex new_mutex;
		new_mutex.m_data() = old_data;

		if (new_mutex.m_order != new_mutex.m_freed)
		{
			res = CELL_SYNC_ERROR_BUSY;
		}
		else
		{
			new_mutex.m_order++;
			res = CELL_OK;
		}
		if (InterlockedCompareExchange(&mutex->m_data(), new_mutex.m_data(), old_data) == old_data) break;
	}

	return res;
}

int cellSyncMutexUnlock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexUnlock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const u32 old_data = mutex->m_data();
		CellSyncMutex new_mutex;
		new_mutex.m_data() = old_data;

		new_mutex.m_freed++;
		if (InterlockedCompareExchange(&mutex->m_data(), new_mutex.m_data(), old_data) == old_data) break;
	}

	return CELL_OK;
}

void cellSync_init()
{
	cellSync.AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync.AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync.AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync.AddFunc(0x91f2b7b0, cellSyncMutexUnlock);
}
