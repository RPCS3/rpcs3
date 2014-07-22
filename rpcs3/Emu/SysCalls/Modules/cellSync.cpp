#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSync.h"

//void cellSync_init();
//Module cellSync("cellSync", cellSync_init);
Module *cellSync = nullptr;

s32 cellSyncMutexInitialize(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexInitialize(mutex_addr=0x%x)", mutex.GetAddr());

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: set zero and sync
	mutex->m_data() = 0;
	InterlockedCompareExchange(&mutex->m_data(), 0, 0);
	return CELL_OK;
}

s32 cellSyncMutexLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexLock(mutex_addr=0x%x)", mutex.GetAddr());

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
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		if (Emu.IsStopped())
		{
			LOG_WARNING(HLE, "cellSyncMutexLock(mutex_addr=0x%x) aborted", mutex.GetAddr());
			break;
		}
	}

	// prx: sync
	InterlockedCompareExchange(&mutex->m_data(), 0, 0);
	return CELL_OK;
}

s32 cellSyncMutexTryLock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexTryLock(mutex_addr=0x%x)", mutex.GetAddr());

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

s32 cellSyncMutexUnlock(mem_ptr_t<CellSyncMutex> mutex)
{
	cellSync->Log("cellSyncMutexUnlock(mutex_addr=0x%x)", mutex.GetAddr());

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

s32 cellSyncBarrierInitialize(mem_ptr_t<CellSyncBarrier> barrier, u16 total_count)
{
	cellSync->Log("cellSyncBarrierInitialize(barrier_addr=0x%x, total_count=%d)", barrier.GetAddr(), total_count);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (!total_count || total_count > 32767)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u16, write total_count in second u16 and sync
	barrier->m_value = 0;
	barrier->m_count = total_count;
	InterlockedCompareExchange(&barrier->m_data(), 0, 0);
	return CELL_OK;
}

s32 cellSyncBarrierNotify(mem_ptr_t<CellSyncBarrier> barrier)
{
	cellSync->Todo("cellSyncBarrierNotify(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncBarrierTryNotify(mem_ptr_t<CellSyncBarrier> barrier)
{
	cellSync->Todo("cellSyncBarrierTryNotify(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncBarrierWait(mem_ptr_t<CellSyncBarrier> barrier)
{
	cellSync->Todo("cellSyncBarrierWait(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncBarrierTryWait(mem_ptr_t<CellSyncBarrier> barrier)
{
	cellSync->Todo("cellSyncBarrierTryWait(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncRwmInitialize(mem_ptr_t<CellSyncRwm> rwm, u32 buffer_addr, u32 buffer_size)
{
	cellSync->Log("cellSyncRwmInitialize(rwm_addr=0x%x, buffer_addr=0x%x, buffer_size=0x%x)", rwm.GetAddr(), buffer_addr, buffer_size);

	if (!rwm || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.GetAddr() % 16 || buffer_addr % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (buffer_size % 128 || buffer_size > 0x4000)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u16 and second u16, write buffer_size in second u32, write buffer_addr in second u64 and sync
	rwm->m_data() = 0;
	rwm->m_size = buffer_size;
	rwm->m_addr = (u64)buffer_addr;
	InterlockedCompareExchange(&rwm->m_data(), 0, 0);
	return CELL_OK;
}

s32 cellSyncRwmRead(mem_ptr_t<CellSyncRwm> rwm, u32 buffer_addr)
{
	cellSync->Log("cellSyncRwmRead(rwm_addr=0x%x, buffer_addr=0x%x)", rwm.GetAddr(), buffer_addr);

	if (!rwm || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.GetAddr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: atomically load first u32, repeat until second u16 == 0, increase first u16 and sync
	while (true)
	{
		const u32 old_data = rwm->m_data();
		CellSyncRwm new_rwm;
		new_rwm.m_data() = old_data;

		if (new_rwm.m_writers.ToBE())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				cellSync->Warning("cellSyncRwmRead(rwm_addr=0x%x) aborted", rwm.GetAddr());
				return CELL_OK;
			}
			continue;
		}
		
		new_rwm.m_readers++;
		if (InterlockedCompareExchange(&rwm->m_data(), new_rwm.m_data(), old_data) == old_data) break;
	}

	// copy data to buffer_addr
	memcpy(Memory + buffer_addr, Memory + (u64)rwm->m_addr, (u32)rwm->m_size);

	// prx: load first u32, return 0x8041010C if first u16 == 0, atomically decrease it
	while (true)
	{
		const u32 old_data = rwm->m_data();
		CellSyncRwm new_rwm;
		new_rwm.m_data() = old_data;

		if (!new_rwm.m_readers.ToBE())
		{
			cellSync->Error("cellSyncRwmRead(rwm_addr=0x%x): m_readers == 0 (m_writers=%d)", rwm.GetAddr(), (u16)new_rwm.m_writers);
			return CELL_SYNC_ERROR_ABORT;
		}

		new_rwm.m_readers--;
		if (InterlockedCompareExchange(&rwm->m_data(), new_rwm.m_data(), old_data) == old_data) break;
	}
	return CELL_OK;
}

s32 cellSyncRwmTryRead(mem_ptr_t<CellSyncRwm> rwm, u32 buffer_addr)
{
	cellSync->Todo("cellSyncRwmTryRead(rwm_addr=0x%x, buffer_addr=0x%x)", rwm.GetAddr(), buffer_addr);

	if (!rwm || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.GetAddr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncRwmWrite(mem_ptr_t<CellSyncRwm> rwm, u32 buffer_addr)
{
	cellSync->Todo("cellSyncRwmWrite(rwm_addr=0x%x, buffer_addr=0x%x)", rwm.GetAddr(), buffer_addr);

	if (!rwm || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.GetAddr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

s32 cellSyncRwmTryWrite(mem_ptr_t<CellSyncRwm> rwm, u32 buffer_addr)
{
	cellSync->Todo("cellSyncRwmTryWrite(rwm_addr=0x%x, buffer_addr=0x%x)", rwm.GetAddr(), buffer_addr);

	if (!rwm || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.GetAddr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// TODO
	return CELL_OK;
}

void cellSync_init()
{
	cellSync->AddFunc(0xa9072dee, cellSyncMutexInitialize);
	cellSync->AddFunc(0x1bb675c2, cellSyncMutexLock);
	cellSync->AddFunc(0xd06918c4, cellSyncMutexTryLock);
	cellSync->AddFunc(0x91f2b7b0, cellSyncMutexUnlock);

	cellSync->AddFunc(0x07254fda, cellSyncBarrierInitialize);
	cellSync->AddFunc(0xf06a6415, cellSyncBarrierNotify);
	cellSync->AddFunc(0x268edd6d, cellSyncBarrierTryNotify);
	cellSync->AddFunc(0x35f21355, cellSyncBarrierWait);
	cellSync->AddFunc(0x6c272124, cellSyncBarrierTryWait);

	cellSync->AddFunc(0xfc48b03f, cellSyncRwmInitialize);
	cellSync->AddFunc(0xcece771f, cellSyncRwmRead);
	cellSync->AddFunc(0xa6669751, cellSyncRwmTryRead);
	cellSync->AddFunc(0xed773f5f, cellSyncRwmWrite);
	cellSync->AddFunc(0xba5bee48, cellSyncRwmTryWrite);
}
