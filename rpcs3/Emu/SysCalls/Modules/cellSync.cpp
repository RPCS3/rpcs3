#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSync.h"

//void cellSync_init();
//Module cellSync("cellSync", cellSync_init);
Module *cellSync = nullptr;

int cellSyncMutexInitialize(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexInitialize(mutex=0x%x)", mutex.GetAddr());

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: set zero and sync
	mutex->m_data = 0;
	InterlockedCompareExchange(&mutex->m_data(), 0, 0);
	return CELL_OK;
}

int cellSyncMutexLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexLock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: increase u16 and remember its old value
	be_t<u16> old_order;
	while (true)
	{
		const u32 old_data = mutex->m_data();
		CellSyncMutex new_mutex;
		new_mutex.m_data() = old_data;

		old_order = new_mutex.m_order;
		new_mutex.m_order++; // increase m_order
		if (InterlockedCompareExchange(&mutex->m_data(), new_mutex.m_data(), old_data) == old_data) break;
	}

	// prx: wait until another u16 value == old value
	while (old_order != mutex->m_freed)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "cellSyncMutexLock(mutex=0x%x) aborted", mutex.GetAddr());
			break;
		}
	}

	// prx: sync
	InterlockedCompareExchange(&mutex->m_data(), 0, 0);
	return CELL_OK;
}

int cellSyncMutexTryLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexTryLock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex)
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

		// prx: compare two u16 values and exit if not equal
		if (new_mutex.m_order != new_mutex.m_freed)
		{
			return CELL_SYNC_ERROR_BUSY;
		}
		else
		{
			new_mutex.m_order++;
		}
		if (InterlockedCompareExchange(&mutex->m_data(), new_mutex.m_data(), old_data) == old_data) break;
	}

	return CELL_OK;
}

int cellSyncMutexUnlock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexUnlock(mutex=0x%x)", mutex.GetAddr());

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	InterlockedCompareExchange(&mutex->m_data(), 0, 0);

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
	cellSync->AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync->AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync->AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync->AddFunc(0x91f2b7b0, cellSyncMutexUnlock);
}
