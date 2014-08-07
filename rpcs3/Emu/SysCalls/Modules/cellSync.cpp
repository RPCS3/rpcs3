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
	cellSync->Log("cellSyncBarrierNotify(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: sync, extract m_value, repeat if < 0, increase, compare with second s16, set sign bit if equal, insert it back
	InterlockedCompareExchange(&barrier->m_data(), 0, 0);

	while (true)
	{
		const u32 old_data = barrier->m_data();
		CellSyncBarrier new_barrier;
		new_barrier.m_data() = old_data;

		s16 value = (s16)new_barrier.m_value;
		if (value < 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				LOG_WARNING(HLE, "cellSyncBarrierNotify(barrier_addr=0x%x) aborted", barrier.GetAddr());
				return CELL_OK;
			}
			continue;
		}

		value++;
		if (value == (s16)new_barrier.m_count)
		{
			value |= 0x8000;
		}
		new_barrier.m_value = value;
		if (InterlockedCompareExchange(&barrier->m_data(), new_barrier.m_data(), old_data) == old_data) break;
	}

	return CELL_OK;
}

s32 cellSyncBarrierTryNotify(mem_ptr_t<CellSyncBarrier> barrier)
{
	cellSync->Log("cellSyncBarrierTryNotify(barrier_addr=0x%x)", barrier.GetAddr());

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.GetAddr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	InterlockedCompareExchange(&barrier->m_data(), 0, 0);

	while (true)
	{
		const u32 old_data = barrier->m_data();
		CellSyncBarrier new_barrier;
		new_barrier.m_data() = old_data;

		s16 value = (s16)new_barrier.m_value;
		if (value >= 0)
		{
			value++;
			if (value == (s16)new_barrier.m_count)
			{
				value |= 0x8000;
			}
			new_barrier.m_value = value;
			if (InterlockedCompareExchange(&barrier->m_data(), new_barrier.m_data(), old_data) == old_data) break;
		}		
		else
		{
			if (InterlockedCompareExchange(&barrier->m_data(), new_barrier.m_data(), old_data) == old_data) return CELL_SYNC_ERROR_BUSY;
		}
	}

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

s32 cellSyncQueueInitialize(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr, u32 size, u32 depth)
{
	cellSync->Log("cellSyncQueueInitialize(queue_addr=0x%x, buffer_addr=0x%x, size=0x%x, depth=0x%x)", queue.GetAddr(), buffer_addr, size, depth);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (size && !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32 || buffer_addr % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (!depth || size % 16)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u64, write size in third u32, write depth in fourth u32, write address in third u64 and sync
	queue->m_data() = 0;
	queue->m_size = size;
	queue->m_depth = depth;
	queue->m_addr = (u64)buffer_addr;
	InterlockedCompareExchange(&queue->m_data(), 0, 0);
	return CELL_OK;
}

s32 cellSyncQueuePush(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Log("cellSyncQueuePush(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	if (!queue || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = (u32)queue->m_size;
	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || ((u32)queue->m_v2 & 0xffffff) > depth)
	{
		cellSync->Error("cellSyncQueuePush(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}

	u32 position;
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v1 = (u32)new_queue.m_v1;
		const u32 v2 = (u32)new_queue.m_v2;
		// prx: compare 5th u8 with zero (repeat if not zero)
		// prx: compare (second u32 (u24) + first u8) with depth (repeat if greater or equal)
		if ((v2 >> 24) || ((v2 & 0xffffff) + (v1 >> 24)) >= depth)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				cellSync->Warning("cellSyncQueuePush(queue_addr=0x%x) aborted", queue.GetAddr());
				return CELL_OK;
			}
			continue;
		}

		// prx: extract first u32 (u24) (-> position), calculate (position + 1) % depth, insert it back
		// prx: insert 1 in 5th u8
		// prx: extract second u32 (u24), increase it, insert it back
		position = (v1 & 0xffffff);
		new_queue.m_v1 = (v1 & 0xff000000) | ((position + 1) % depth);
		new_queue.m_v2 = (1 << 24) | ((v2 & 0xffffff) + 1);
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	// prx: memcpy(position * m_size + m_addr, buffer_addr, m_size), sync
	memcpy(Memory + (u64)queue->m_addr + position * size, Memory + buffer_addr, size);

	// prx: atomically insert 0 in 5th u8
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		new_queue.m_v2 &= 0xffffff; // TODO: use InterlockedAnd() or something
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}
	return CELL_OK;
}

s32 cellSyncQueueTryPush(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Log("cellSyncQueueTryPush(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	if (!queue || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = (u32)queue->m_size;
	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || ((u32)queue->m_v2 & 0xffffff) > depth)
	{
		cellSync->Error("cellSyncQueueTryPush(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}

	u32 position;
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v1 = (u32)new_queue.m_v1;
		const u32 v2 = (u32)new_queue.m_v2;
		if ((v2 >> 24) || ((v2 & 0xffffff) + (v1 >> 24)) >= depth)
		{
			return CELL_SYNC_ERROR_BUSY;
		}

		position = (v1 & 0xffffff);
		new_queue.m_v1 = (v1 & 0xff000000) | ((position + 1) % depth);
		new_queue.m_v2 = (1 << 24) | ((v2 & 0xffffff) + 1);
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	memcpy(Memory + (u64)queue->m_addr + position * size, Memory + buffer_addr, size);

	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		new_queue.m_v2 &= 0xffffff; // TODO: use InterlockedAnd() or something
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}
	return CELL_OK;
}

s32 cellSyncQueuePop(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Log("cellSyncQueuePop(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	if (!queue || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = (u32)queue->m_size;
	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || ((u32)queue->m_v2 & 0xffffff) > depth)
	{
		cellSync->Error("cellSyncQueuePop(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}
	
	u32 position;
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v1 = (u32)new_queue.m_v1;
		const u32 v2 = (u32)new_queue.m_v2;
		// prx: extract first u8, repeat if not zero
		// prx: extract second u32 (u24), subtract 5th u8, compare with zero, repeat if less or equal
		if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				cellSync->Warning("cellSyncQueuePop(queue_addr=0x%x) aborted", queue.GetAddr());
				return CELL_OK;
			}
			continue;
		}

		// prx: insert 1 in first u8
		// prx: extract first u32 (u24), add depth, subtract second u32 (u24), calculate (% depth), save to position
		// prx: extract second u32 (u24), decrease it, insert it back
		new_queue.m_v1 = 0x1000000 | v1;
		position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;
		new_queue.m_v2 = (v2 & 0xff000000) | ((v2 & 0xffffff) - 1);
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	// prx: (sync), memcpy(buffer_addr, position * m_size + m_addr, m_size)
	memcpy(Memory + buffer_addr, Memory + (u64)queue->m_addr + position * size, size);

	// prx: atomically insert 0 in first u8
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		new_queue.m_v1 &= 0xffffff; // TODO: use InterlockedAnd() or something
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}
	return CELL_OK;
}

s32 cellSyncQueueTryPop(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Log("cellSyncQueueTryPop(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	if (!queue || !buffer_addr)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = (u32)queue->m_size;
	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || ((u32)queue->m_v2 & 0xffffff) > depth)
	{
		cellSync->Error("cellSyncQueueTryPop(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}

	u32 position;
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v1 = (u32)new_queue.m_v1;
		const u32 v2 = (u32)new_queue.m_v2;
		if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
		{
			return CELL_SYNC_ERROR_BUSY;
		}

		new_queue.m_v1 = 0x1000000 | v1;
		position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;
		new_queue.m_v2 = (v2 & 0xff000000) | ((v2 & 0xffffff) - 1);
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	memcpy(Memory + buffer_addr, Memory + (u64)queue->m_addr + position * size, size);

	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		new_queue.m_v1 &= 0xffffff; // TODO: use InterlockedAnd() or something
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}
	return CELL_OK;
}

s32 cellSyncQueuePeek(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Todo("cellSyncQueuePeek(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	return CELL_OK;
}

s32 cellSyncQueueTryPeek(mem_ptr_t<CellSyncQueue> queue, u32 buffer_addr)
{
	cellSync->Todo("cellSyncQueueTryPeek(queue_addr=0x%x, buffer_addr=0x%x)", queue.GetAddr(), buffer_addr);

	return CELL_OK;
}

s32 cellSyncQueueSize(mem_ptr_t<CellSyncQueue> queue)
{
	cellSync->Log("cellSyncQueueSize(queue_addr=0x%x)", queue.GetAddr());

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 count = (u32)queue->m_v2 & 0xffffff;
	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || count > depth)
	{
		cellSync->Error("cellSyncQueueSize(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}

	return count;
}

s32 cellSyncQueueClear(mem_ptr_t<CellSyncQueue> queue)
{
	cellSync->Log("cellSyncQueueClear(queue_addr=0x%x)", queue.GetAddr());

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.GetAddr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = (u32)queue->m_depth;
	if (((u32)queue->m_v1 & 0xffffff) > depth || ((u32)queue->m_v2 & 0xffffff) > depth)
	{
		cellSync->Error("cellSyncQueueSize(queue_addr=0x%x): m_depth limit broken", queue.GetAddr());
		Emu.Pause();
	}

	// TODO: optimize if possible
	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v1 = (u32)new_queue.m_v1;
		// prx: extract first u8, repeat if not zero, insert 1
		if (v1 >> 24)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				cellSync->Warning("cellSyncQueueClear(queue_addr=0x%x) aborted (I)", queue.GetAddr());
				return CELL_OK;
			}
			continue;
		}
		new_queue.m_v1 = v1 | 0x1000000;
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	while (true)
	{
		const u64 old_data = queue->m_data();
		CellSyncQueue new_queue;
		new_queue.m_data() = old_data;

		const u32 v2 = (u32)new_queue.m_v2;
		// prx: extract 5th u8, repeat if not zero, insert 1
		if (v2 >> 24)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			if (Emu.IsStopped())
			{
				cellSync->Warning("cellSyncQueueClear(queue_addr=0x%x) aborted (II)", queue.GetAddr());
				return CELL_OK;
			}
			continue;
		}
		new_queue.m_v2 = v2 | 0x1000000;
		if (InterlockedCompareExchange(&queue->m_data(), new_queue.m_data(), old_data) == old_data) break;
	}

	queue->m_data() = 0;
	InterlockedCompareExchange(&queue->m_data(), 0, 0);
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

	cellSync->AddFunc(0x3929948d, cellSyncQueueInitialize);
	cellSync->AddFunc(0x5ae841e5, cellSyncQueuePush);
	cellSync->AddFunc(0x705985cd, cellSyncQueueTryPush);
	cellSync->AddFunc(0x4da6d7e0, cellSyncQueuePop);
	cellSync->AddFunc(0xa58df87f, cellSyncQueueTryPop);
	cellSync->AddFunc(0x48154c9b, cellSyncQueuePeek);
	cellSync->AddFunc(0x68af923c, cellSyncQueueTryPeek);
	cellSync->AddFunc(0x4da349b2, cellSyncQueueSize);
	cellSync->AddFunc(0xa5362e73, cellSyncQueueClear);
}
