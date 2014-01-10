#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSync_init();
Module cellSync("cellSync", cellSync_init);
wxMutex g_SyncMutex;

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

struct CellSyncMutex {
	be_t<u16> m_freed;
	be_t<u16> m_order;
	/*
	(???) Lock: increase m_order and wait until m_freed == old m_order
	(???) Unlock: increase m_freed
	(???) TryLock: ?????
	*/
};

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
	{
		wxMutexLocker lock(g_SyncMutex);
		mutex->m_freed = 0;
		mutex->m_order = 0;
	}
	return CELL_OK;
}

int cellSyncMutexLock(mem_ptr_t<CellSyncMutex> mutex)
{
	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	be_t<u16> old_order;
	{
		wxMutexLocker lock(g_SyncMutex);
		cellSync.Log("cellSyncMutexLock(mutex=0x%x[order=%d,freed=%d])", mutex.GetAddr(), (u16)mutex->m_order, (u16)mutex->m_freed);
		old_order = mutex->m_order;
		mutex->m_order = mutex->m_order + 1;
	}
	while (old_order != mutex->m_freed) Sleep(1);
	return CELL_OK;
}

int cellSyncMutexTryLock(mem_ptr_t<CellSyncMutex> mutex)
{
	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	wxMutexLocker lock(g_SyncMutex);
	cellSync.Log("cellSyncMutexTryLock(mutex=0x%x[order=%d,freed=%d])", mutex.GetAddr(), (u16)mutex->m_order, (u16)mutex->m_freed);
	if (mutex->m_order != mutex->m_freed)
	{
		return CELL_SYNC_ERROR_BUSY;
	}
	mutex->m_order = mutex->m_order + 1;
	return CELL_OK;
}

int cellSyncMutexUnlock(mem_ptr_t<CellSyncMutex> mutex)
{
	if (!mutex.IsGood())
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	wxMutexLocker lock(g_SyncMutex);
	cellSync.Log("cellSyncMutexUnlock(mutex=0x%x[order=%d,freed=%d])", mutex.GetAddr(), (u16)mutex->m_order, (u16)mutex->m_freed);
	mutex->m_freed = mutex->m_freed + 1;
	return CELL_OK;
}

void cellSync_init()
{
	cellSync.AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync.AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync.AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync.AddFunc(0x91f2b7b0, cellSyncMutexUnlock);
}