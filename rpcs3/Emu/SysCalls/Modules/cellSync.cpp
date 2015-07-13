#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/SysCalls/lv2/sleep_queue.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_process.h"
#include "Emu/Event.h"
#include "cellSync.h"

extern Module cellSync;

waiter_map_t g_sync_mutex_wm("sync_mutex_wm");
waiter_map_t g_sync_barrier_wait_wm("sync_barrier_wait_wm");
waiter_map_t g_sync_barrier_notify_wm("sync_barrier_notify_wm");
waiter_map_t g_sync_rwm_read_wm("sync_rwm_read_wm");
waiter_map_t g_sync_rwm_write_wm("sync_rwm_write_wm");
waiter_map_t g_sync_queue_wm("sync_queue_wm");

s32 syncMutexInitialize(vm::ptr<CellSyncMutex> mutex)
{
	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (mutex.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->sync_var.exchange({});

	return CELL_OK;
}

s32 cellSyncMutexInitialize(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexInitialize(mutex=*0x%x)", mutex);

	return syncMutexInitialize(mutex);
}

s32 cellSyncMutexLock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexLock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (mutex.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: increase acquire_count and remember its old value
	const auto order = mutex->acquire_count++;

	// prx: wait until release_count is equal to old acquire_count
	g_sync_mutex_wm.wait_op(mutex.addr(), [mutex, order]()
	{
		return order == mutex->release_count.read_relaxed();
	});

	mutex->sync_var.read_sync();

	return CELL_OK;
}

s32 cellSyncMutexTryLock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexTryLock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (mutex.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: lock only if acquire_count and release_count are equal
	return mutex->sync_var.atomic_op(CELL_OK, [](CellSyncMutex::sync_t& mutex) -> s32
	{
		return (mutex.acquire_count++ != mutex.release_count) ? CELL_SYNC_ERROR_BUSY : CELL_OK;
	});
}

s32 cellSyncMutexUnlock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.Log("cellSyncMutexUnlock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (mutex.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: increase release count
	mutex->release_count++;

	g_sync_mutex_wm.notify(mutex.addr());

	return CELL_OK;
}

s32 syncBarrierInitialize(vm::ptr<CellSyncBarrier> barrier, u16 total_count)
{
	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (!total_count || total_count > 32767)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u16, write total_count in second u16 and sync
	barrier->data.exchange({ be_t<s16>::make(0), be_t<s16>::make(total_count) });

	return CELL_OK;
}

s32 cellSyncBarrierInitialize(vm::ptr<CellSyncBarrier> barrier, u16 total_count)
{
	cellSync.Log("cellSyncBarrierInitialize(barrier=*0x%x, total_count=%d)", barrier, total_count);

	return syncBarrierInitialize(barrier, total_count);
}

s32 syncBarrierTryNotifyOp(CellSyncBarrier::data_t& barrier)
{
	// prx: extract m_value (repeat if < 0), increase, compare with second s16, set sign bit if equal, insert it back
	s16 value = barrier.m_value;

	if (value < 0)
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	if (++value == barrier.m_count)
	{
		value |= 0x8000;
	}

	barrier.m_value = value;

	return CELL_OK;
};

s32 cellSyncBarrierNotify(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.Log("cellSyncBarrierNotify(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	g_sync_barrier_notify_wm.wait_op(barrier.addr(), [barrier]()
	{
		return barrier->data.atomic_op_sync(CELL_OK, syncBarrierTryNotifyOp) == CELL_OK;
	});

	g_sync_barrier_wait_wm.notify(barrier.addr());

	return CELL_OK;
}

s32 cellSyncBarrierTryNotify(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.Log("cellSyncBarrierTryNotify(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (s32 res = barrier->data.atomic_op_sync(CELL_OK, syncBarrierTryNotifyOp))
	{
		return res;
	}

	g_sync_barrier_wait_wm.notify(barrier.addr());

	return CELL_OK;
}

s32 syncBarrierTryWaitOp(CellSyncBarrier::data_t& barrier)
{
	// prx: extract m_value (repeat if >= 0), decrease it, set 0 if == 0x8000, insert it back
	s16 value = barrier.m_value;

	if (value >= 0)
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	if (--value == -0x8000)
	{
		value = 0;
	}

	barrier.m_value = value;

	return CELL_OK;
}

s32 cellSyncBarrierWait(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.Log("cellSyncBarrierWait(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	g_sync_barrier_wait_wm.wait_op(barrier.addr(), [barrier]()
	{
		return barrier->data.atomic_op_sync(CELL_OK, syncBarrierTryWaitOp) == CELL_OK;
	});

	g_sync_barrier_notify_wm.notify(barrier.addr());

	return CELL_OK;
}

s32 cellSyncBarrierTryWait(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.Log("cellSyncBarrierTryWait(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (barrier.addr() % 4)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (s32 res = barrier->data.atomic_op_sync(CELL_OK, syncBarrierTryWaitOp))
	{
		return res;
	}

	g_sync_barrier_notify_wm.notify(barrier.addr());

	return CELL_OK;
}

s32 syncRwmInitialize(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer, u32 buffer_size)
{
	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.addr() % 16 || buffer.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (buffer_size % 128 || buffer_size > 0x4000)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u16 and second u16, write buffer_size in second u32, write buffer_addr in second u64 and sync
	rwm->m_size = buffer_size;
	rwm->m_buffer = buffer;
	rwm->data.exchange({});

	return CELL_OK;
}

s32 cellSyncRwmInitialize(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer, u32 buffer_size)
{
	cellSync.Log("cellSyncRwmInitialize(rwm=*0x%x, buffer=*0x%x, buffer_size=0x%x)", rwm, buffer, buffer_size);

	return syncRwmInitialize(rwm, buffer, buffer_size);
}

s32 syncRwmTryReadBeginOp(CellSyncRwm::data_t& rwm)
{
	if (rwm.m_writers.data())
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	rwm.m_readers++;

	return CELL_OK;
}

s32 syncRwmReadEndOp(CellSyncRwm::data_t& rwm)
{
	if (!rwm.m_readers.data())
	{
		return CELL_SYNC_ERROR_ABORT;
	}

	rwm.m_readers--;

	return CELL_OK;
}

s32 cellSyncRwmRead(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncRwmRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: increase m_readers, wait until m_writers is zero
	g_sync_rwm_read_wm.wait_op(rwm.addr(), [rwm]()
	{
		return rwm->data.atomic_op(CELL_OK, syncRwmTryReadBeginOp) == CELL_OK;
	});

	// copy data to buffer_addr
	memcpy(buffer.get_ptr(), rwm->m_buffer.get_ptr(), rwm->m_size);

	// prx: decrease m_readers (return 0x8041010C if already zero)
	if (s32 res = rwm->data.atomic_op(CELL_OK, syncRwmReadEndOp))
	{
		cellSync.Error("syncRwmReadEndOp(rwm=0x%x) failed: m_readers == 0", rwm);
		return res;
	}

	g_sync_rwm_write_wm.notify(rwm.addr());

	return CELL_OK;
}

s32 cellSyncRwmTryRead(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncRwmTryRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (s32 res = rwm->data.atomic_op(CELL_OK, syncRwmTryReadBeginOp))
	{
		return res;
	}

	memcpy(buffer.get_ptr(), rwm->m_buffer.get_ptr(), rwm->m_size);

	if (s32 res = rwm->data.atomic_op(CELL_OK, syncRwmReadEndOp))
	{
		return res;
	}

	g_sync_rwm_write_wm.notify(rwm.addr());

	return CELL_OK;
}

s32 syncRwmTryWriteBeginOp(CellSyncRwm::data_t& rwm)
{
	if (rwm.m_writers.data())
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	rwm.m_writers = 1;

	return CELL_OK;
}

s32 cellSyncRwmWrite(vm::ptr<CellSyncRwm> rwm, vm::ptr<const void> buffer)
{
	cellSync.Log("cellSyncRwmWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	g_sync_rwm_read_wm.wait_op(rwm.addr(), [rwm]()
	{
		return rwm->data.atomic_op(CELL_OK, syncRwmTryWriteBeginOp) == CELL_OK;
	});

	// prx: wait until m_readers == 0
	g_sync_rwm_write_wm.wait_op(rwm.addr(), [rwm]()
	{
		return rwm->data.read_relaxed().m_readers.data() == 0;
	});

	// prx: copy data from buffer_addr
	memcpy(rwm->m_buffer.get_ptr(), buffer.get_ptr(), rwm->m_size);

	// prx: sync and zeroize m_readers and m_writers
	rwm->data.exchange({});

	g_sync_rwm_read_wm.notify(rwm.addr());

	return CELL_OK;
}

s32 cellSyncRwmTryWrite(vm::ptr<CellSyncRwm> rwm, vm::ptr<const void> buffer)
{
	cellSync.Log("cellSyncRwmTryWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (rwm.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: compare m_readers | m_writers with 0, return if not zero, set m_writers to 1
	if (!rwm->data.compare_and_swap_test({}, { be_t<u16>::make(0), be_t<u16>::make(1) }))
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	// prx: copy data from buffer_addr
	memcpy(rwm->m_buffer.get_ptr(), buffer.get_ptr(), rwm->m_size);

	// prx: sync and zeroize m_readers and m_writers
	rwm->data.exchange({});

	g_sync_rwm_read_wm.notify(rwm.addr());

	return CELL_OK;
}

s32 syncQueueInitialize(vm::ptr<CellSyncQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth)
{
	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (size && !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32 || buffer.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}
	if (!depth || size % 16)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// prx: zeroize first u64, write size in third u32, write depth in fourth u32, write address in third u64 and sync
	queue->m_size = size;
	queue->m_depth = depth;
	queue->m_buffer = buffer;
	queue->data.exchange({});

	return CELL_OK;
}

s32 cellSyncQueueInitialize(vm::ptr<CellSyncQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth)
{
	cellSync.Log("cellSyncQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x)", queue, buffer, size, depth);

	return syncQueueInitialize(queue, buffer, size, depth);
}

s32 syncQueueTryPushOp(CellSyncQueue::data_t& queue, u32 depth, u32& position)
{
	const u32 v1 = queue.m_v1;
	const u32 v2 = queue.m_v2;

	// prx: compare 5th u8 with zero (break if not zero)
	// prx: compare (second u32 (u24) + first u8) with depth (break if greater or equal)
	if ((v2 >> 24) || ((v2 & 0xffffff) + (v1 >> 24)) >= depth)
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	// prx: extract first u32 (u24) (-> position), calculate (position + 1) % depth, insert it back
	// prx: insert 1 in 5th u8
	// prx: extract second u32 (u24), increase it, insert it back
	position = (v1 & 0xffffff);
	queue.m_v1 = (v1 & 0xff000000) | ((position + 1) % depth);
	queue.m_v2 = (1 << 24) | ((v2 & 0xffffff) + 1);

	return CELL_OK;
}

s32 cellSyncQueuePush(vm::ptr<CellSyncQueue> queue, vm::ptr<const void> buffer)
{
	cellSync.Log("cellSyncQueuePush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	u32 position;
	g_sync_queue_wm.wait_op(queue.addr(), [queue, depth, &position]()
	{
		return CELL_OK == queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
		{
			return syncQueueTryPushOp(queue, depth, position);
		});
	});

	// prx: memcpy(position * m_size + m_addr, buffer_addr, m_size), sync
	memcpy(&queue->m_buffer[position * size], buffer.get_ptr(), size);

	// prx: atomically insert 0 in 5th u8
	queue->data &= { be_t<u32>::make(~0), be_t<u32>::make(0xffffff) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 cellSyncQueueTryPush(vm::ptr<CellSyncQueue> queue, vm::ptr<const void> buffer)
{
	cellSync.Log("cellSyncQueueTryPush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	u32 position;
	s32 res = queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
	{
		return syncQueueTryPushOp(queue, depth, position);
	});
	if (res)
	{
		return res;
	}

	memcpy(&queue->m_buffer[position * size], buffer.get_ptr(), size);

	queue->data &= { be_t<u32>::make(~0), be_t<u32>::make(0xffffff) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 syncQueueTryPopOp(CellSyncQueue::data_t& queue, u32 depth, u32& position)
{
	const u32 v1 = queue.m_v1;
	const u32 v2 = queue.m_v2;

	// prx: extract first u8, repeat if not zero
	// prx: extract second u32 (u24), subtract 5th u8, compare with zero, repeat if less or equal
	if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	// prx: insert 1 in first u8
	// prx: extract first u32 (u24), add depth, subtract second u32 (u24), calculate (% depth), save to position
	// prx: extract second u32 (u24), decrease it, insert it back
	queue.m_v1 = 0x1000000 | v1;
	position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;
	queue.m_v2 = (v2 & 0xff000000) | ((v2 & 0xffffff) - 1);

	return CELL_OK;
}

s32 cellSyncQueuePop(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncQueuePop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);
	
	u32 position;
	g_sync_queue_wm.wait_op(queue.addr(), [queue, depth, &position]()
	{
		return CELL_OK == queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
		{
			return syncQueueTryPopOp(queue, depth, position);
		});
	});

	// prx: (sync), memcpy(buffer_addr, position * m_size + m_addr, m_size)
	memcpy(buffer.get_ptr(), &queue->m_buffer[position * size], size);

	// prx: atomically insert 0 in first u8
	queue->data &= { be_t<u32>::make(0xffffff), be_t<u32>::make(~0) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 cellSyncQueueTryPop(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncQueueTryPop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	u32 position;
	s32 res = queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
	{
		return syncQueueTryPopOp(queue, depth, position);
	});
	if (res)
	{
		return res;
	}

	memcpy(buffer.get_ptr(), &queue->m_buffer[position * size], size);

	queue->data &= { be_t<u32>::make(0xffffff), be_t<u32>::make(~0) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 syncQueueTryPeekOp(CellSyncQueue::data_t& queue, u32 depth, u32& position)
{
	const u32 v1 = queue.m_v1;
	const u32 v2 = queue.m_v2;

	if ((v1 >> 24) || ((v2 & 0xffffff) <= (v2 >> 24)))
	{
		return CELL_SYNC_ERROR_BUSY;
	}

	queue.m_v1 = 0x1000000 | v1;
	position = ((v1 & 0xffffff) + depth - (v2 & 0xffffff)) % depth;

	return CELL_OK;
}

s32 cellSyncQueuePeek(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncQueuePeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	u32 position;
	g_sync_queue_wm.wait_op(queue.addr(), [queue, depth, &position]()
	{
		return CELL_OK == queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
		{
			return syncQueueTryPeekOp(queue, depth, position);
		});
	});

	memcpy(buffer.get_ptr(), &queue->m_buffer[position * size], size);

	queue->data &= { be_t<u32>::make(0xffffff), be_t<u32>::make(~0) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 cellSyncQueueTryPeek(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.Log("cellSyncQueueTryPeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 size = queue->m_size;
	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	u32 position;
	s32 res = queue->data.atomic_op(CELL_OK, [depth, &position](CellSyncQueue::data_t& queue) -> s32
	{
		return syncQueueTryPeekOp(queue, depth, position);
	});
	if (res)
	{
		return res;
	}

	memcpy(buffer.get_ptr(), &queue->m_buffer[position * size], size);

	queue->data &= { be_t<u32>::make(0xffffff), be_t<u32>::make(~0) };

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

s32 cellSyncQueueSize(vm::ptr<CellSyncQueue> queue)
{
	cellSync.Log("cellSyncQueueSize(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const auto data = queue->data.read_relaxed();
	const u32 count = data.m_v2 & 0xffffff;
	const u32 depth = queue->m_depth;
	assert((data.m_v1 & 0xffffff) <= depth && count <= depth);

	return count;
}

s32 cellSyncQueueClear(vm::ptr<CellSyncQueue> queue)
{
	cellSync.Log("cellSyncQueueClear(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 32)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->m_depth;
	const auto data = queue->data.read_relaxed();
	assert((data.m_v1 & 0xffffff) <= depth && (data.m_v2 & 0xffffff) <= depth);

	// TODO: optimize if possible
	g_sync_queue_wm.wait_op(queue.addr(), [queue, depth]()
	{
		return CELL_OK == queue->data.atomic_op(CELL_OK, [depth](CellSyncQueue::data_t& queue) -> s32
		{
			const u32 v1 = queue.m_v1;

			// prx: extract first u8, repeat if not zero, insert 1
			if (v1 >> 24)
			{
				return CELL_SYNC_ERROR_BUSY;
			}

			queue.m_v1 = v1 | 0x1000000;

			return CELL_OK;
		});
	});

	g_sync_queue_wm.wait_op(queue.addr(), [queue, depth]()
	{
		return CELL_OK == queue->data.atomic_op(CELL_OK, [depth](CellSyncQueue::data_t& queue) -> s32
		{
			const u32 v2 = queue.m_v2;

			// prx: extract 5th u8, repeat if not zero, insert 1
			if (v2 >> 24)
			{
				return CELL_SYNC_ERROR_BUSY;
			}

			queue.m_v2 = v2 | 0x1000000;

			return CELL_OK;
		});
	});

	queue->data.exchange({});

	g_sync_queue_wm.notify(queue.addr());

	return CELL_OK;
}

// LFQueue functions

void syncLFQueueInit(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth, CellSyncQueueDirection direction, vm::ptr<void> eaSignal)
{
	queue->m_size = size;
	queue->m_depth = depth;
	queue->m_buffer = buffer;
	queue->m_direction = direction;
	memset(queue->m_hs1, 0, sizeof(queue->m_hs1));
	memset(queue->m_hs2, 0, sizeof(queue->m_hs2));
	queue->m_eaSignal = eaSignal;

	if (direction == CELL_SYNC_QUEUE_ANY2ANY)
	{
		queue->pop1 = {};
		queue->push1 = {};
		queue->m_buffer.set(queue->m_buffer.addr() | 1);
		queue->m_bs[0] = -1;
		queue->m_bs[1] = -1;
		//m_bs[2]
		//m_bs[3]
		queue->m_v1 = -1;
		queue->push2 = { { be_t<u16>::make(-1) } };
		queue->pop2 = { { be_t<u16>::make(-1) } };
	}
	else
	{
		queue->pop1 = { { be_t<u16>::make(0), be_t<u16>::make(0), queue->pop1.read_relaxed().m_h3, be_t<u16>::make(0) } };
		queue->push1 = { { be_t<u16>::make(0), be_t<u16>::make(0), queue->push1.read_relaxed().m_h7, be_t<u16>::make(0) } };
		queue->m_bs[0] = -1; // written as u32
		queue->m_bs[1] = -1;
		queue->m_bs[2] = -1;
		queue->m_bs[3] = -1;
		queue->m_v1 = 0;
		queue->push2 = {};
		queue->pop2 = {};
	}

	queue->m_v2 = 0;
	queue->m_eq_id = 0;
}

s32 syncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth, CellSyncQueueDirection direction, vm::ptr<void> eaSignal)
{
	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (size)
	{
		if (!buffer)
		{
			return CELL_SYNC_ERROR_NULL_POINTER;
		}
		if (size > 0x4000 || size % 16)
		{
			return CELL_SYNC_ERROR_INVAL;
		}
	}
	if (!depth || (depth >> 15) || direction > 3)
	{
		return CELL_SYNC_ERROR_INVAL;
	}
	if (queue.addr() % 128 || buffer.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// prx: get sdk version of current process, return non-zero result of sys_process_get_sdk_version
	s32 sdk_ver;
	s32 ret = process_get_sdk_version(process_getpid(), sdk_ver);
	if (ret != CELL_OK)
	{
		return ret;
	}
	if (sdk_ver == -1)
	{
		sdk_ver = 0x460000;
	}

	// prx: reserve u32 at 0x2c offset
	u32 old_value;
	while (true)
	{
		const auto old = queue->init.read_relaxed();
		auto init = old;

		if (old.data())
		{
			if (sdk_ver > 0x17ffff && old != 2)
			{
				return CELL_SYNC_ERROR_STAT;
			}
			old_value = old;
		}
		else
		{
			if (sdk_ver > 0x17ffff)
			{
				auto data = vm::get_ptr<u64>(queue.addr());
				for (u32 i = 0; i < sizeof(CellSyncLFQueue) / sizeof(u64); i++)
				{
					if (data[i])
					{
						return CELL_SYNC_ERROR_STAT;
					}
				}
			}
			init = 1;
			old_value = 1;
		}

		if (queue->init.compare_and_swap_test(old, init)) break;
	}

	if (old_value == 2)
	{
		if (queue->m_size != size || queue->m_depth != depth || queue->m_buffer != buffer)
		{
			return CELL_SYNC_ERROR_INVAL;
		}
		if (sdk_ver > 0x17ffff)
		{
			if (queue->m_eaSignal.addr() != eaSignal.addr() || queue->m_direction != direction)
			{
				return CELL_SYNC_ERROR_INVAL;
			}
		}
	}
	else
	{
		// prx: call internal function with same arguments
		syncLFQueueInit(queue, buffer, size, depth, direction, eaSignal);

		// prx: sync, zeroize u32 at 0x2c offset
		queue->init.exchange({});
	}

	// prx: sync
	queue->init.read_sync();
	return CELL_OK;
}

s32 cellSyncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth, CellSyncQueueDirection direction, vm::ptr<void> eaSignal)
{
	cellSync.Warning("cellSyncLFQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x, direction=%d, eaSignal=*0x%x)", queue, buffer, size, depth, direction, eaSignal);

	return syncLFQueueInitialize(queue, buffer, size, depth, direction, eaSignal);
}

s32 syncLFQueueGetPushPointer(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue)
{
	if (queue->m_direction != CELL_SYNC_QUEUE_PPU2SPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	u32 var1 = 0;

	while (true)
	{
		while (true)
		{
			if (Emu.IsStopped())
			{
				return -1;
			}

			const auto old = queue->push1.read_sync();
			auto push = old;

			if (var1)
			{
				push.m_h7 = 0;
			}
			if (isBlocking && useEventQueue && *(u32*)queue->m_bs == -1)
			{
				return CELL_SYNC_ERROR_STAT;
			}

			s32 var2 = (s16)push.m_h8;
			s32 res;
			if (useEventQueue && ((s32)push.m_h5 != var2 || push.m_h7.data() != 0))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 -= (s32)(u16)queue->pop1.read_relaxed().m_h1;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 < depth)
				{
					pointer = (s16)push.m_h8;
					if (pointer + 1 >= depth * 2)
					{
						push.m_h8 = 0;
					}
					else
					{
						push.m_h8++;
					}
					res = CELL_OK;
				}
				else if (!isBlocking)
				{
					return CELL_SYNC_ERROR_AGAIN;
				}
				else if (!useEventQueue)
				{
					continue;
				}
				else
				{
					res = CELL_OK;
					push.m_h7 = 3;
					if (isBlocking != 3)
					{
						break;
					}
				}
			}

			if (queue->push1.compare_and_swap_test(old, push))
			{
				if (!push.m_h7.data() || res)
				{
					return res;
				}
				break;
			}
		}

		if (s32 res = sys_event_queue_receive(GetCurrentPPUThread(), queue->m_eq_id, vm::null, 0))
		{
			assert(!"sys_event_queue_receive() failed");
		}
		var1 = 1;
	}
}

s32 _cellSyncLFQueueGetPushPointer(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> pointer, u32 isBlocking, u32 useEventQueue)
{
	cellSync.Warning("_cellSyncLFQueueGetPushPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	s32 pointer_value;
	s32 result = syncLFQueueGetPushPointer(queue, pointer_value, isBlocking, useEventQueue);

	*pointer = pointer_value;

	return result;
}

s32 syncLFQueueGetPushPointer2(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueGetPushPointer2(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPushPointer
	cellSync.Todo("_cellSyncLFQueueGetPushPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	s32 pointer_value;
	s32 result = syncLFQueueGetPushPointer2(queue, pointer_value, isBlocking, useEventQueue);

	*pointer = pointer_value;

	return result;
}

s32 syncLFQueueCompletePushPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal)
{
	if (queue->m_direction != CELL_SYNC_QUEUE_PPU2SPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->push2.read_sync();
		auto push2 = old;

		const auto old2 = queue->push3.read_relaxed();
		auto push3 = old2;

		s32 var1 = pointer - (u16)push3.m_h5;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = (s32)(s16)queue->pop1.read_relaxed().m_h4 - (s32)(u16)queue->pop1.read_relaxed().m_h1;
		if (var2 < 0)
		{
			var2 += depth * 2;
		}

		s32 var9_ = 15 - var1;
		// calculate (u16)(1 slw (15 - var1))
		if (var9_ & 0x30)
		{
			var9_ = 0;
		}
		else
		{
			var9_ = 1 << var9_;
		}
		s32 var9 = cntlz32((u32)(u16)~(var9_ | (u16)push3.m_h6)) - 16; // count leading zeros in u16
		
		s32 var5 = (s32)(u16)push3.m_h6 | var9_;
		if (var9 & 0x30)
		{
			var5 = 0;
		}
		else
		{
			var5 <<= var9;
		}

		s32 var3 = (u16)push3.m_h5 + var9;
		if (var3 >= depth * 2)
		{
			var3 -= depth * 2;
		}

		u16 pack = push2.pack; // three packed 5-bit fields

		s32 var4 = ((pack >> 10) & 0x1f) - ((pack >> 5) & 0x1f);
		if (var4 < 0)
		{
			var4 += 0x1e;
		}

		u32 var6;
		if (var2 + var4 <= 15 && ((pack >> 10) & 0x1f) != (pack & 0x1f))
		{
			s32 var8 = (pack & 0x1f) - ((pack >> 10) & 0x1f);
			if (var8 < 0)
			{
				var8 += 0x1e;
			}

			if (var9 > 1 && (u32)var8 > 1)
			{
				assert(16 - var2 <= 1);
			}

			s32 var11 = (pack >> 10) & 0x1f;
			if (var11 >= 15)
			{
				var11 -= 15;
			}

			u16 var12 = (pack >> 10) & 0x1f;
			if (var12 == 0x1d)
			{
				var12 = 0;
			}
			else
			{
				var12 = (var12 + 1) << 10;
			}

			push2.pack = (pack & 0x83ff) | var12;
			var6 = (u16)queue->m_hs1[var11];
		}
		else
		{
			var6 = -1;
		}

		push3.m_h5 = (u16)var3;
		push3.m_h6 = (u16)var5;

		if (queue->push2.compare_and_swap_test(old, push2))
		{
			assert(var2 + var4 < 16);
			if (var6 != -1)
			{
				bool exch = queue->push3.compare_and_swap_test(old2, push3);
				assert(exch);
				if (exch)
				{
					assert(fpSendSignal);
					return fpSendSignal((u32)queue->m_eaSignal.addr(), var6);
				}
			}
			else
			{
				pack = queue->push2.read_relaxed().pack;
				if ((pack & 0x1f) == ((pack >> 10) & 0x1f))
				{
					if (queue->push3.compare_and_swap_test(old2, push3))
					{
						return CELL_OK;
					}
				}
			}
		}
	}
}

s32 _cellSyncLFQueueCompletePushPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	cellSync.Warning("_cellSyncLFQueueCompletePushPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	return syncLFQueueCompletePushPointer(queue, pointer, fpSendSignal);
}

s32 syncLFQueueCompletePushPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueCompletePushPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer
	cellSync.Todo("_cellSyncLFQueueCompletePushPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	return syncLFQueueCompletePushPointer2(queue, pointer, fpSendSignal);
}

s32 _cellSyncLFQueuePushBody(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, vm::ptr<const void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePush has 1 in isBlocking param, cellSyncLFQueueTryPush has 0
	cellSync.Warning("_cellSyncLFQueuePushBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128 || buffer.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	s32 position;

	while (true)
	{
		s32 res;

		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = syncLFQueueGetPushPointer(queue, position, isBlocking, 0);
		}
		else
		{
			res = syncLFQueueGetPushPointer2(queue, position, isBlocking, 0);
		}

		if (!isBlocking || res != CELL_SYNC_ERROR_AGAIN)
		{
			if (res)
			{
				return res;
			}
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (Emu.IsStopped())
		{
			cellSync.Warning("_cellSyncLFQueuePushBody(queue=*0x%x) aborted", queue);
			return CELL_OK;
		}
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const u32 addr = vm::cast<u64>((queue->m_buffer.addr() & ~1ull) + size * (position >= depth ? position - depth : position));
	memcpy(vm::get_ptr<void>(addr), buffer.get_ptr(), size);

	s32 res;

	if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
	{
		res = syncLFQueueCompletePushPointer(queue, position, nullptr);
	}
	else
	{
		res = syncLFQueueCompletePushPointer2(queue, position, nullptr);
	}

	return res;
}

s32 syncLFQueueGetPopPointer(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32, u32 useEventQueue)
{
	if (queue->m_direction != CELL_SYNC_QUEUE_SPU2PPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	u32 var1 = 0;

	while (true)
	{
		while (true)
		{
			if (Emu.IsStopped())
			{
				return -1;
			}

			const auto old = queue->pop1.read_sync();
			auto pop = old;

			if (var1)
			{
				pop.m_h3 = 0;
			}
			if (isBlocking && useEventQueue && *(u32*)queue->m_bs == -1)
			{
				return CELL_SYNC_ERROR_STAT;
			}

			s32 var2 = (s32)(s16)pop.m_h4;
			s32 res;
			if (useEventQueue && ((s32)(u16)pop.m_h1 != var2 || pop.m_h3.data() != 0))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 = (s32)(u16)queue->push1.read_relaxed().m_h5 - var2;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 > 0)
				{
					pointer = (s16)pop.m_h4;
					if (pointer + 1 >= depth * 2)
					{
						pop.m_h4 = 0;
					}
					else
					{
						pop.m_h4++;
					}
					res = CELL_OK;
				}
				else if (!isBlocking)
				{
					return CELL_SYNC_ERROR_AGAIN;
				}
				else if (!useEventQueue)
				{
					continue;
				}
				else
				{
					res = CELL_OK;
					pop.m_h3 = 3;
					if (isBlocking != 3)
					{
						break;
					}
				}
			}

			if (queue->pop1.compare_and_swap_test(old, pop))
			{
				if (!pop.m_h3.data() || res)
				{
					return res;
				}
				break;
			}
		}

		if (s32 res = sys_event_queue_receive(GetCurrentPPUThread(), queue->m_eq_id, vm::null, 0))
		{
			assert(!"sys_event_queue_receive() failed");
		}
		var1 = 1;
	}
}

s32 _cellSyncLFQueueGetPopPointer(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> pointer, u32 isBlocking, u32 arg4, u32 useEventQueue)
{
	cellSync.Warning("_cellSyncLFQueueGetPopPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, arg4=%d, useEventQueue=%d)", queue, pointer, isBlocking, arg4, useEventQueue);

	s32 pointer_value;
	s32 result = syncLFQueueGetPopPointer(queue, pointer_value, isBlocking, arg4, useEventQueue);

	*pointer = pointer_value;

	return result;
}

s32 syncLFQueueGetPopPointer2(vm::ptr<CellSyncLFQueue> queue, s32& pointer, u32 isBlocking, u32 useEventQueue)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueGetPopPointer2(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPopPointer
	cellSync.Todo("_cellSyncLFQueueGetPopPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	s32 pointer_value;
	s32 result = syncLFQueueGetPopPointer2(queue, pointer_value, isBlocking, useEventQueue);

	*pointer = pointer_value;

	return result;
}

s32 syncLFQueueCompletePopPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	if (queue->m_direction != CELL_SYNC_QUEUE_SPU2PPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->pop2.read_sync();
		auto pop2 = old;

		const auto old2 = queue->pop3.read_relaxed();
		auto pop3 = old2;

		s32 var1 = pointer - (u16)pop3.m_h1;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = (s32)(s16)queue->push1.read_relaxed().m_h8 - (s32)(u16)queue->push1.read_relaxed().m_h5;
		if (var2 < 0)
		{
			var2 += depth * 2;
		}

		s32 var9_ = 15 - var1;
		// calculate (u16)(1 slw (15 - var1))
		if (var9_ & 0x30)
		{
			var9_ = 0;
		}
		else
		{
			var9_ = 1 << var9_;
		}
		s32 var9 = cntlz32((u32)(u16)~(var9_ | (u16)pop3.m_h2)) - 16; // count leading zeros in u16

		s32 var5 = (s32)(u16)pop3.m_h2 | var9_;
		if (var9 & 0x30)
		{
			var5 = 0;
		}
		else
		{
			var5 <<= var9;
		}

		s32 var3 = (u16)pop3.m_h1 + var9;
		if (var3 >= depth * 2)
		{
			var3 -= depth * 2;
		}

		u16 pack = pop2.pack; // three packed 5-bit fields

		s32 var4 = ((pack >> 10) & 0x1f) - ((pack >> 5) & 0x1f);
		if (var4 < 0)
		{
			var4 += 0x1e;
		}

		u32 var6;
		if (noQueueFull || var2 + var4 > 15 || ((pack >> 10) & 0x1f) == (pack & 0x1f))
		{
			var6 = -1;
		}
		else
		{
			s32 var8 = (pack & 0x1f) - ((pack >> 10) & 0x1f);
			if (var8 < 0)
			{
				var8 += 0x1e;
			}

			if (var9 > 1 && (u32)var8 > 1)
			{
				assert(16 - var2 <= 1);
			}

			s32 var11 = (pack >> 10) & 0x1f;
			if (var11 >= 15)
			{
				var11 -= 15;
			}

			u16 var12 = (pack >> 10) & 0x1f;
			if (var12 == 0x1d)
			{
				var12 = 0;
			}
			else
			{
				var12 = (var12 + 1) << 10;
			}

			pop2.pack = (pack & 0x83ff) | var12;
			var6 = (u16)queue->m_hs2[var11];
		}

		pop3.m_h1 = (u16)var3;
		pop3.m_h2 = (u16)var5;

		if (queue->pop2.compare_and_swap_test(old, pop2))
		{
			if (var6 != -1)
			{
				bool exch = queue->pop3.compare_and_swap_test(old2, pop3);
				assert(exch);
				if (exch)
				{
					assert(fpSendSignal);
					return fpSendSignal((u32)queue->m_eaSignal.addr(), var6);
				}
			}
			else
			{
				pack = queue->pop2.read_relaxed().pack;
				if ((pack & 0x1f) == ((pack >> 10) & 0x1f))
				{
					if (queue->pop3.compare_and_swap_test(old2, pop3))
					{
						return CELL_OK;
					}
				}
			}
		}
	}
}

s32 _cellSyncLFQueueCompletePopPointer(vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer + unknown argument (noQueueFull taken from LFQueue2CompletePopPointer)
	cellSync.Warning("_cellSyncLFQueueCompletePopPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	return syncLFQueueCompletePopPointer(queue, pointer, fpSendSignal, noQueueFull);
}

s32 syncLFQueueCompletePopPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, const std::function<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueCompletePopPointer2(vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePopPointer
	cellSync.Todo("_cellSyncLFQueueCompletePopPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	return syncLFQueueCompletePopPointer2(queue, pointer, fpSendSignal, noQueueFull);
}

s32 _cellSyncLFQueuePopBody(PPUThread& CPU, vm::ptr<CellSyncLFQueue> queue, vm::ptr<void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePop has 1 in isBlocking param, cellSyncLFQueueTryPop has 0
	cellSync.Warning("_cellSyncLFQueuePopBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128 || buffer.addr() % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	s32 position;

	while (true)
	{
		s32 res;
		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = syncLFQueueGetPopPointer(queue, position, isBlocking, 0, 0);
		}
		else
		{
			res = syncLFQueueGetPopPointer2(queue, position, isBlocking, 0);
		}

		if (!isBlocking || res != CELL_SYNC_ERROR_AGAIN)
		{
			if (res)
			{
				return res;
			}
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

		if (Emu.IsStopped())
		{
			cellSync.Warning("_cellSyncLFQueuePopBody(queue=*0x%x) aborted", queue);
			return CELL_OK;
		}
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const u32 addr = vm::cast<u64>((queue->m_buffer.addr() & ~1) + size * (position >= depth ? position - depth : position));
	memcpy(buffer.get_ptr(), vm::get_ptr<void>(addr), size);

	s32 res;

	if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
	{
		res = syncLFQueueCompletePopPointer(queue, position, nullptr, 0);
	}
	else
	{
		res = syncLFQueueCompletePopPointer2(queue, position, nullptr, 0);
	}

	return res;
}

s32 cellSyncLFQueueClear(vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.Warning("cellSyncLFQueueClear(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop1.read_sync();
		auto pop = old;

		const auto push = queue->push1.read_relaxed();

		s32 var1, var2;
		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			var1 = var2 = (u16)queue->pop2.read_relaxed().pack;
		}
		else
		{
			var1 = (u16)push.m_h7;
			var2 = (u16)pop.m_h3;
		}

		if ((s32)(s16)pop.m_h4 != (s32)(u16)pop.m_h1 ||
			(s32)(s16)push.m_h8 != (s32)(u16)push.m_h5 ||
			((var2 >> 10) & 0x1f) != (var2 & 0x1f) ||
			((var1 >> 10) & 0x1f) != (var1 & 0x1f))
		{
			return CELL_SYNC_ERROR_BUSY;
		}

		pop.m_h1 = push.m_h5;
		pop.m_h2 = push.m_h6;
		pop.m_h3 = push.m_h7;
		pop.m_h4 = push.m_h8;

		if (queue->pop1.compare_and_swap_test(old, pop)) break;
	}

	return CELL_OK;
}

s32 cellSyncLFQueueSize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> size)
{
	cellSync.Warning("cellSyncLFQueueSize(queue=*0x%x, size=*0x%x)", queue, size);

	if (!queue || !size)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop3.read_sync();

		u32 var1 = (u16)queue->pop1.read_relaxed().m_h1;
		u32 var2 = (u16)queue->push1.read_relaxed().m_h5;

		if (queue->pop3.compare_and_swap_test(old, old))
		{
			if (var1 <= var2)
			{
				*size = var2 - var1;
			}
			else
			{
				*size = var2 - var1 + (u32)queue->m_depth * 2;
			}
			return CELL_OK;
		}
	}
}

s32 cellSyncLFQueueDepth(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> depth)
{
	cellSync.Log("cellSyncLFQueueDepth(queue=*0x%x, depth=*0x%x)", queue, depth);

	if (!queue || !depth)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*depth = queue->m_depth;
	return CELL_OK;
}

s32 _cellSyncLFQueueGetSignalAddress(vm::ptr<const CellSyncLFQueue> queue, vm::pptr<void> ppSignal)
{
	cellSync.Log("_cellSyncLFQueueGetSignalAddress(queue=*0x%x, ppSignal=**0x%x)", queue, ppSignal);

	if (!queue || !ppSignal)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*ppSignal = queue->m_eaSignal;

	return CELL_OK;
}

s32 cellSyncLFQueueGetDirection(vm::ptr<const CellSyncLFQueue> queue, vm::ptr<CellSyncQueueDirection> direction)
{
	cellSync.Log("cellSyncLFQueueGetDirection(queue=*0x%x, direction=*0x%x)", queue, direction);

	if (!queue || !direction)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*direction = queue->m_direction;
	return CELL_OK;
}

s32 cellSyncLFQueueGetEntrySize(vm::ptr<const CellSyncLFQueue> queue, vm::ptr<u32> entry_size)
{
	cellSync.Log("cellSyncLFQueueGetEntrySize(queue=*0x%x, entry_size=*0x%x)", queue, entry_size);

	if (!queue || !entry_size)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}
	if (queue.addr() % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*entry_size = queue->m_size;
	return CELL_OK;
}

s32 syncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.Todo("_cellSyncLFQueueAttachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	return syncLFQueueAttachLv2EventQueue(spus, num, queue);
}

s32 syncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	throw __FUNCTION__;
}

s32 _cellSyncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.Todo("_cellSyncLFQueueDetachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	return syncLFQueueDetachLv2EventQueue(spus, num, queue);
}

Module cellSync("cellSync", []()
{
	REG_FUNC(cellSync, cellSyncMutexInitialize);
	REG_FUNC(cellSync, cellSyncMutexLock);
	REG_FUNC(cellSync, cellSyncMutexTryLock);
	REG_FUNC(cellSync, cellSyncMutexUnlock);

	REG_FUNC(cellSync, cellSyncBarrierInitialize);
	REG_FUNC(cellSync, cellSyncBarrierNotify);
	REG_FUNC(cellSync, cellSyncBarrierTryNotify);
	REG_FUNC(cellSync, cellSyncBarrierWait);
	REG_FUNC(cellSync, cellSyncBarrierTryWait);

	REG_FUNC(cellSync, cellSyncRwmInitialize);
	REG_FUNC(cellSync, cellSyncRwmRead);
	REG_FUNC(cellSync, cellSyncRwmTryRead);
	REG_FUNC(cellSync, cellSyncRwmWrite);
	REG_FUNC(cellSync, cellSyncRwmTryWrite);

	REG_FUNC(cellSync, cellSyncQueueInitialize);
	REG_FUNC(cellSync, cellSyncQueuePush);
	REG_FUNC(cellSync, cellSyncQueueTryPush);
	REG_FUNC(cellSync, cellSyncQueuePop);
	REG_FUNC(cellSync, cellSyncQueueTryPop);
	REG_FUNC(cellSync, cellSyncQueuePeek);
	REG_FUNC(cellSync, cellSyncQueueTryPeek);
	REG_FUNC(cellSync, cellSyncQueueSize);
	REG_FUNC(cellSync, cellSyncQueueClear);

	REG_FUNC(cellSync, cellSyncLFQueueGetEntrySize);
	REG_FUNC(cellSync, cellSyncLFQueueSize);
	REG_FUNC(cellSync, cellSyncLFQueueClear);
	REG_FUNC(cellSync, _cellSyncLFQueueCompletePushPointer2);
	REG_FUNC(cellSync, _cellSyncLFQueueGetPopPointer2);
	REG_FUNC(cellSync, _cellSyncLFQueueCompletePushPointer);
	REG_FUNC(cellSync, _cellSyncLFQueueAttachLv2EventQueue);
	REG_FUNC(cellSync, _cellSyncLFQueueGetPushPointer2);
	REG_FUNC(cellSync, _cellSyncLFQueueGetPopPointer);
	REG_FUNC(cellSync, _cellSyncLFQueueCompletePopPointer2);
	REG_FUNC(cellSync, _cellSyncLFQueueDetachLv2EventQueue);
	REG_FUNC(cellSync, cellSyncLFQueueInitialize);
	REG_FUNC(cellSync, _cellSyncLFQueueGetSignalAddress);
	REG_FUNC(cellSync, _cellSyncLFQueuePushBody);
	REG_FUNC(cellSync, cellSyncLFQueueGetDirection);
	REG_FUNC(cellSync, cellSyncLFQueueDepth);
	REG_FUNC(cellSync, _cellSyncLFQueuePopBody);
	REG_FUNC(cellSync, _cellSyncLFQueueGetPushPointer);
	REG_FUNC(cellSync, _cellSyncLFQueueCompletePopPointer);
});
