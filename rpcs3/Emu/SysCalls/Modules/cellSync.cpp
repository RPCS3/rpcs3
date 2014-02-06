#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include <mutex>

void cellSync_init();
Module cellSync("cellSync", cellSync_init);

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

#pragma pack(push, 1)
struct CellSyncMutex {
	union { 
		struct {
			be_t<u16> m_freed;
			be_t<u16> m_order;
		};
		volatile u32 m_data;
	};
	/*
	(???) Initialize: set zeros
	(???) Lock: increase m_order and wait until m_freed == old m_order
	(???) Unlock: increase m_freed
	(???) TryLock: ?????
	*/
};
#pragma pack(pop)

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
		SMutexLocker lock(reservation.mutex);
		if ((reservation.addr + reservation.size > mutex.GetAddr() && reservation.addr <= mutex.GetAddr() + 4) || 
			(mutex.GetAddr() + 4 > reservation.addr && mutex.GetAddr() <= reservation.addr + reservation.size))
		{
			reservation.clear();
		}
		mutex->m_data = 0;
		return CELL_OK;
	}
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
	{
		SMutexLocker lock(reservation.mutex);
		if ((reservation.addr + reservation.size > mutex.GetAddr() && reservation.addr <= mutex.GetAddr() + 4) || 
			(mutex.GetAddr() + 4 > reservation.addr && mutex.GetAddr() <= reservation.addr + reservation.size))
		{
			reservation.clear();
		}
		old_order = mutex->m_order;
		mutex->m_order = mutex->m_order + 1;
		if (old_order == mutex->m_freed)
		{
			return CELL_OK;
		}
	}

	int counter = 0;
	while (*(u16*)&old_order != *(u16*)&mutex->m_freed) 
	{
		Sleep(1);
		if (++counter >= 5000)
		{
			counter = 0;
			cellSync.Warning("cellSyncMutexLock(mutex=0x%x, old_order=%d, order=%d, freed=%d): TIMEOUT", 
				mutex.GetAddr(), (u16)old_order, (u16)mutex->m_order, (u16)mutex->m_freed);
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
	{
		SMutexLocker lock(reservation.mutex);
		if ((reservation.addr + reservation.size > mutex.GetAddr() && reservation.addr <= mutex.GetAddr() + 4) || 
			(mutex.GetAddr() + 4 > reservation.addr && mutex.GetAddr() <= reservation.addr + reservation.size))
		{
			reservation.clear();
		}
		if (mutex->m_order != mutex->m_freed)
		{
			return CELL_SYNC_ERROR_BUSY;
		}
		mutex->m_order = mutex->m_order + 1;
		return CELL_OK;
	}
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

	{ /* global mutex */
		SMutexLocker lock(reservation.mutex);
		if ((reservation.addr + reservation.size > mutex.GetAddr() && reservation.addr <= mutex.GetAddr() + 4) || 
			(mutex.GetAddr() + 4 > reservation.addr && mutex.GetAddr() <= reservation.addr + reservation.size))
		{
			reservation.clear();
		}
		mutex->m_freed = mutex->m_freed + 1;
		return CELL_OK;
	}
}

void cellSync_init()
{
	cellSync.AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync.AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync.AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync.AddFunc(0x91f2b7b0, cellSyncMutexUnlock);
}