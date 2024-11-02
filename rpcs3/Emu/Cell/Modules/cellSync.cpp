#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "cellSync.h"

LOG_CHANNEL(cellSync);

template<>
void fmt_class_string<CellSyncError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_SYNC_ERROR_AGAIN);
		STR_CASE(CELL_SYNC_ERROR_INVAL);
		STR_CASE(CELL_SYNC_ERROR_NOSYS);
		STR_CASE(CELL_SYNC_ERROR_NOMEM);
		STR_CASE(CELL_SYNC_ERROR_SRCH);
		STR_CASE(CELL_SYNC_ERROR_NOENT);
		STR_CASE(CELL_SYNC_ERROR_NOEXEC);
		STR_CASE(CELL_SYNC_ERROR_DEADLK);
		STR_CASE(CELL_SYNC_ERROR_PERM);
		STR_CASE(CELL_SYNC_ERROR_BUSY);
		STR_CASE(CELL_SYNC_ERROR_ABORT);
		STR_CASE(CELL_SYNC_ERROR_FAULT);
		STR_CASE(CELL_SYNC_ERROR_CHILD);
		STR_CASE(CELL_SYNC_ERROR_STAT);
		STR_CASE(CELL_SYNC_ERROR_ALIGN);
		STR_CASE(CELL_SYNC_ERROR_NULL_POINTER);
		STR_CASE(CELL_SYNC_ERROR_NOT_SUPPORTED_THREAD);
		STR_CASE(CELL_SYNC_ERROR_NO_NOTIFIER);
		STR_CASE(CELL_SYNC_ERROR_NO_SPU_CONTEXT_STORAGE);
		}

		return unknown;
	});
}

error_code cellSyncMutexInitialize(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexInitialize(mutex=*0x%x)", mutex);

	if (!mutex) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->ctrl.exchange({0, 0});

	return CELL_OK;
}

error_code cellSyncMutexLock(ppu_thread& ppu, vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexLock(mutex=*0x%x)", mutex);

	if (!mutex) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// Increase acq value and remember its old value
	const auto order = mutex->ctrl.atomic_op(&CellSyncMutex::Counter::lock_begin);

	// Wait until rel value is equal to old acq value
	while (mutex->ctrl.load().rel != order)
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	atomic_fence_acq_rel();
	return CELL_OK;
}

error_code cellSyncMutexTryLock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexTryLock(mutex=*0x%x)", mutex);

	if (!mutex) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!mutex->ctrl.atomic_op(&CellSyncMutex::Counter::try_lock))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	return CELL_OK;
}

error_code cellSyncMutexUnlock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexUnlock(mutex=*0x%x)", mutex);

	if (!mutex) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->ctrl.atomic_op(&CellSyncMutex::Counter::unlock);

	return CELL_OK;
}

error_code cellSyncBarrierInitialize(vm::ptr<CellSyncBarrier> barrier, u16 total_count)
{
	cellSync.trace("cellSyncBarrierInitialize(barrier=*0x%x, total_count=%d)", barrier, total_count);

	if (!barrier) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!total_count || total_count > 32767) [[unlikely]]
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear current value, write total_count and sync
	barrier->ctrl.exchange({0, total_count});

	return CELL_OK;
}

error_code cellSyncBarrierNotify(ppu_thread& ppu, vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierNotify(barrier=*0x%x)", barrier);

	if (!barrier) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (!barrier->ctrl.atomic_op(&CellSyncBarrier::try_notify))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	return CELL_OK;
}

error_code cellSyncBarrierTryNotify(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierTryNotify(barrier=*0x%x)", barrier);

	if (!barrier) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	atomic_fence_acq_rel();

	if (!barrier->ctrl.atomic_op(&CellSyncBarrier::try_notify))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	return CELL_OK;
}

error_code cellSyncBarrierWait(ppu_thread& ppu, vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierWait(barrier=*0x%x)", barrier);

	if (!barrier) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	atomic_fence_acq_rel();

	while (!barrier->ctrl.atomic_op(&CellSyncBarrier::try_wait))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	return CELL_OK;
}

error_code cellSyncBarrierTryWait(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierTryWait(barrier=*0x%x)", barrier);

	if (!barrier) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	atomic_fence_acq_rel();

	if (!barrier->ctrl.atomic_op(&CellSyncBarrier::try_wait))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	return CELL_OK;
}

error_code cellSyncRwmInitialize(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer, u32 buffer_size)
{
	cellSync.trace("cellSyncRwmInitialize(rwm=*0x%x, buffer=*0x%x, buffer_size=0x%x)", rwm, buffer, buffer_size);

	if (!rwm || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned() || !buffer.aligned(128)) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (buffer_size % 128 || buffer_size > 0x4000) [[unlikely]]
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear readers and writers, write buffer_size, buffer addr and sync
	rwm->ctrl.store({ 0, 0 });
	rwm->size = buffer_size;
	rwm->buffer = buffer;

	atomic_fence_acq_rel();

	return CELL_OK;
}

error_code cellSyncRwmRead(ppu_thread& ppu, vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncRwmRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// wait until `writers` is zero, increase `readers`
	while (!rwm->ctrl.atomic_op(&CellSyncRwm::try_read_begin))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// copy data to buffer
	std::memcpy(buffer.get_ptr(), rwm->buffer.get_ptr(), rwm->size);

	// decrease `readers`, return error if already zero
	if (!rwm->ctrl.atomic_op(&CellSyncRwm::try_read_end))
	{
		return CELL_SYNC_ERROR_ABORT;
	}

	return CELL_OK;
}

error_code cellSyncRwmTryRead(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncRwmTryRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// increase `readers` if `writers` is zero
	if (!rwm->ctrl.atomic_op(&CellSyncRwm::try_read_begin))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	// copy data to buffer
	std::memcpy(buffer.get_ptr(), rwm->buffer.get_ptr(), rwm->size);

	// decrease `readers`, return error if already zero
	if (!rwm->ctrl.atomic_op(&CellSyncRwm::try_read_end))
	{
		return CELL_SYNC_ERROR_ABORT;
	}

	return CELL_OK;
}

error_code cellSyncRwmWrite(ppu_thread& ppu, vm::ptr<CellSyncRwm> rwm, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncRwmWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// wait until `writers` is zero, set to 1
	while (!rwm->ctrl.atomic_op(&CellSyncRwm::try_write_begin))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// wait until `readers` is zero
	while (rwm->ctrl.load().readers != 0)
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// copy data from buffer
	std::memcpy(rwm->buffer.get_ptr(), buffer.get_ptr(), rwm->size);

	// sync and clear `readers` and `writers`
	rwm->ctrl.exchange({ 0, 0 });

	return CELL_OK;
}

error_code cellSyncRwmTryWrite(vm::ptr<CellSyncRwm> rwm, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncRwmTryWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// set `writers` to 1 if `readers` and `writers` are zero
	if (!rwm->ctrl.compare_and_swap_test({ 0, 0 }, { 0, 1 }))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	// copy data from buffer
	std::memcpy(rwm->buffer.get_ptr(), buffer.get_ptr(), rwm->size);

	// sync and clear `readers` and `writers`
	rwm->ctrl.exchange({ 0, 0 });

	return CELL_OK;
}

error_code cellSyncQueueInitialize(vm::ptr<CellSyncQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth)
{
	cellSync.trace("cellSyncQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x)", queue, buffer, size, depth);

	if (!queue) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (size && !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || !buffer.aligned(16)) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!depth || size % 16) [[unlikely]]
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear sync var, write size, depth, buffer addr and sync
	queue->ctrl.store({});
	queue->size = size;
	queue->depth = depth;
	queue->buffer = buffer;

	atomic_fence_acq_rel();

	return CELL_OK;
}

error_code cellSyncQueuePush(ppu_thread& ppu, vm::ptr<CellSyncQueue> queue, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_push_begin(ctrl, depth, &position);
	}))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// copy data from the buffer at the position
	std::memcpy(&queue->buffer[position * queue->size], buffer.get_ptr(), queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::push_end);

	return CELL_OK;
}

error_code cellSyncQueueTryPush(vm::ptr<CellSyncQueue> queue, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_push_begin(ctrl, depth, &position);
	}))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	// copy data from the buffer at the position
	std::memcpy(&queue->buffer[position * queue->size], buffer.get_ptr(), queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::push_end);

	return CELL_OK;
}

error_code cellSyncQueuePop(ppu_thread& ppu, vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_pop_begin(ctrl, depth, &position);
	}))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::pop_end);

	return CELL_OK;
}

error_code cellSyncQueueTryPop(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_pop_begin(ctrl, depth, &position);
	}))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::pop_end);

	return CELL_OK;
}

error_code cellSyncQueuePeek(ppu_thread& ppu, vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_peek_begin(ctrl, depth, &position);
	}))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::pop_end);

	return CELL_OK;
}

error_code cellSyncQueueTryPeek(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	while (!queue->ctrl.atomic_op([&](CellSyncQueue::ctrl_t& ctrl)
	{
		return CellSyncQueue::try_peek_begin(ctrl, depth, &position);
	}))
	{
		return not_an_error(CELL_SYNC_ERROR_BUSY);
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	queue->ctrl.atomic_op(&CellSyncQueue::pop_end);

	return CELL_OK;
}

error_code cellSyncQueueSize(vm::ptr<CellSyncQueue> queue)
{
	cellSync.trace("cellSyncQueueSize(queue=*0x%x)", queue);

	if (!queue) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	queue->check_depth();

	return not_an_error(queue->ctrl.load().count & 0xffffff);
}

error_code cellSyncQueueClear(ppu_thread& ppu, vm::ptr<CellSyncQueue> queue)
{
	cellSync.trace("cellSyncQueueClear(queue=*0x%x)", queue);

	if (!queue) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	queue->check_depth();

	while (!queue->ctrl.atomic_op(&CellSyncQueue::try_clear_begin_1))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	while (!queue->ctrl.atomic_op(&CellSyncQueue::try_clear_begin_2))
	{
		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	queue->ctrl.store({});
	return CELL_OK;
}

// LFQueue functions

void syncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction, vm::ptr<void> eaSignal)
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
		queue->pop1.store({});
		queue->push1.store({});
		queue->m_buffer.set(queue->m_buffer.addr() | 1);
		queue->m_bs[0] = -1;
		queue->m_bs[1] = -1;
		//m_bs[2]
		//m_bs[3]
		queue->m_v1 = -1;
		queue->push2.store({ 0xffff });
		queue->pop2.store({ 0xffff });
	}
	else
	{
		queue->pop1.store({ 0, 0, queue->pop1.load().m_h3, 0});
		queue->push1.store({ 0, 0, queue->push1.load().m_h7, 0 });
		queue->m_bs[0] = -1; // written as u32
		queue->m_bs[1] = -1;
		queue->m_bs[2] = -1;
		queue->m_bs[3] = -1;
		queue->m_v1 = 0;
		queue->push2.store({});
		queue->pop2.store({});
	}

	queue->m_v2 = 0;
	queue->m_eq_id = 0;
}

error_code cellSyncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction, vm::ptr<void> eaSignal)
{
	cellSync.warning("cellSyncLFQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x, direction=%d, eaSignal=*0x%x)", queue, buffer, size, depth, direction, eaSignal);

	if (!queue) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (size)
	{
		if (!buffer) [[unlikely]]
		{
			return CELL_SYNC_ERROR_NULL_POINTER;
		}

		if (size > 0x4000 || size % 16) [[unlikely]]
		{
			return CELL_SYNC_ERROR_INVAL;
		}
	}

	if (!depth || depth > 0x7fff || direction > 3) [[unlikely]]
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	if (!queue.aligned() || !buffer.aligned(16)) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// get sdk version of current process
	s32 sdk_ver;

	if (s32 ret = process_get_sdk_version(process_getpid(), sdk_ver))
	{
		return not_an_error(ret);
	}

	if (sdk_ver == -1)
	{
		sdk_ver = 0x460000;
	}

	// reserve `init`
	u32 old_value;

	while (true)
	{
		const auto old = queue->init.load();
		auto init = old;

		if (old)
		{
			if (sdk_ver > 0x17ffff && old != 2) [[unlikely]]
			{
				return CELL_SYNC_ERROR_STAT;
			}

			old_value = old;
		}
		else
		{
			if (sdk_ver > 0x17ffff)
			{
				for (const auto& data : vm::_ref<u64[16]>(queue.addr()))
				{
					if (data) [[unlikely]]
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
		if (queue->m_size != size || queue->m_depth != depth || queue->m_buffer != buffer) [[unlikely]]
		{
			return CELL_SYNC_ERROR_INVAL;
		}

		if (sdk_ver > 0x17ffff)
		{
			if (queue->m_eaSignal != eaSignal || queue->m_direction != direction) [[unlikely]]
			{
				return CELL_SYNC_ERROR_INVAL;
			}
		}

		atomic_fence_acq_rel();
	}
	else
	{
		syncLFQueueInitialize(queue, buffer, size, depth, direction, eaSignal);

		queue->init.exchange(0);
	}

	return CELL_OK;
}

error_code _cellSyncLFQueueGetPushPointer(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	cellSync.warning("_cellSyncLFQueueGetPushPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	if (queue->m_direction != CELL_SYNC_QUEUE_PPU2SPU) [[unlikely]]
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	u32 var1 = 0;

	while (true)
	{
		while (true)
		{
			const auto old = queue->push1.load();
			auto push = old;

			if (var1)
			{
				push.m_h7 = 0;
			}
			if (isBlocking && useEventQueue && std::bit_cast<s32>(queue->m_bs) == -1)
			{
				return CELL_SYNC_ERROR_STAT;
			}

			s32 var2 = static_cast<s16>(push.m_h8);
			s32 res;
			if (useEventQueue && (+push.m_h5 != var2 || push.m_h7))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 -= queue->pop1.load().m_h1;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 < depth)
				{
					const s32 _pointer = static_cast<s16>(push.m_h8);
					*pointer = _pointer;
					if (_pointer + 1 >= depth * 2)
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
				if (!push.m_h7 || res)
				{
					return not_an_error(res);
				}
				break;
			}
		}

		ensure(sys_event_queue_receive(ppu, queue->m_eq_id, vm::null, 0) == CELL_OK);
		var1 = 1;
	}
}

error_code _cellSyncLFQueueGetPushPointer2(ppu_thread& /*ppu*/, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPushPointer
	cellSync.todo("_cellSyncLFQueueGetPushPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	return CELL_OK;
}

error_code _cellSyncLFQueueCompletePushPointer(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	cellSync.warning("_cellSyncLFQueueCompletePushPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	if (queue->m_direction != CELL_SYNC_QUEUE_PPU2SPU) [[unlikely]]
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->push2.load();
		auto push2 = old;

		// Loads must be in this order
		const auto old2 = queue->push3.load();
		auto push3 = old2;

		s32 var1 = pointer - push3.m_h5;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = static_cast<s16>(queue->pop1.load().m_h4) - queue->pop1.load().m_h1;
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
		s32 var9 = std::countl_zero<u32>(static_cast<u16>(~(var9_ | push3.m_h6))) - 16; // count leading zeros in u16

		s32 var5 = push3.m_h6 | var9_;
		if (var9 & 0x30)
		{
			var5 = 0;
		}
		else
		{
			var5 <<= var9;
		}

		s32 var3 = push3.m_h5 + var9;
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

			if (var9 > 1 && static_cast<u32>(var8) > 1)
			{
				ensure((16 - var2 <= 1));
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
			var6 = queue->m_hs1[var11];
		}
		else
		{
			var6 = -1;
		}

		push3.m_h5 = static_cast<u16>(var3);
		push3.m_h6 = static_cast<u16>(var5);

		if (queue->push2.compare_and_swap_test(old, push2))
		{
			ensure((var2 + var4 < 16));
			if (var6 != umax)
			{
				ensure((queue->push3.compare_and_swap_test(old2, push3)));
				ensure((fpSendSignal));
				return not_an_error(fpSendSignal(ppu, vm::cast(queue->m_eaSignal.addr()), var6));
			}
			else
			{
				pack = queue->push2.load().pack;
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

error_code _cellSyncLFQueueCompletePushPointer2(ppu_thread&, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer
	cellSync.todo("_cellSyncLFQueueCompletePushPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	return CELL_OK;
}

error_code _cellSyncLFQueuePushBody(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePush has 1 in isBlocking param, cellSyncLFQueueTryPush has 0
	cellSync.warning("_cellSyncLFQueuePushBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || !buffer.aligned(16)) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	vm::var<s32> position;

	while (true)
	{
		s32 res;

		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = _cellSyncLFQueueGetPushPointer(ppu, queue, position, isBlocking, 0);
		}
		else
		{
			res = _cellSyncLFQueueGetPushPointer2(ppu, queue, position, isBlocking, 0);
		}

		if (!isBlocking || res + 0u != CELL_SYNC_ERROR_AGAIN)
		{
			if (res) return not_an_error(res);

			break;
		}

		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const s32 pos = *position;
	const u32 addr = vm::cast<u64>((queue->m_buffer.addr() & ~1ull) + size * (pos >= depth ? pos - depth : pos));
	std::memcpy(vm::base(addr), buffer.get_ptr(), size);

	if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
	{
		return _cellSyncLFQueueCompletePushPointer(ppu, queue, pos, vm::null);
	}
	else
	{
		return _cellSyncLFQueueCompletePushPointer2(ppu, queue, pos, vm::null);
	}
}

error_code _cellSyncLFQueueGetPopPointer(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 arg4, u32 useEventQueue)
{
	cellSync.warning("_cellSyncLFQueueGetPopPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, arg4=%d, useEventQueue=%d)", queue, pointer, isBlocking, arg4, useEventQueue);

	if (queue->m_direction != CELL_SYNC_QUEUE_SPU2PPU) [[unlikely]]
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	u32 var1 = 0;

	while (true)
	{
		while (true)
		{
			const auto old = queue->pop1.load();
			auto pop = old;

			if (var1)
			{
				pop.m_h3 = 0;
			}
			if (isBlocking && useEventQueue && std::bit_cast<s32>(queue->m_bs) == -1)
			{
				return CELL_SYNC_ERROR_STAT;
			}

			s32 var2 = static_cast<s16>(pop.m_h4);
			s32 res;
			if (useEventQueue && (static_cast<s32>(pop.m_h1) != var2 || pop.m_h3))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 = queue->push1.load().m_h5 - var2;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 > 0)
				{
					const s32 _pointer = static_cast<s16>(pop.m_h4);
					*pointer = _pointer;
					if (_pointer + 1 >= depth * 2)
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
				if (!pop.m_h3 || res)
				{
					return not_an_error(res);
				}
				break;
			}
		}

		ensure((sys_event_queue_receive(ppu, queue->m_eq_id, vm::null, 0) == CELL_OK));
		var1 = 1;
	}
}

error_code _cellSyncLFQueueGetPopPointer2(ppu_thread&, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPopPointer
	cellSync.todo("_cellSyncLFQueueGetPopPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	return CELL_OK;
}

error_code _cellSyncLFQueueCompletePopPointer(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer + unknown argument (noQueueFull taken from LFQueue2CompletePopPointer)
	cellSync.warning("_cellSyncLFQueueCompletePopPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	if (queue->m_direction != CELL_SYNC_QUEUE_SPU2PPU) [[unlikely]]
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->pop2.load();
		auto pop2 = old;

		// Loads must be in this order
		const auto old2 = queue->pop3.load();
		auto pop3 = old2;

		s32 var1 = pointer - pop3.m_h1;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = static_cast<s16>(queue->push1.load().m_h8) - queue->push1.load().m_h5;
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

		s32 var9 = std::countl_zero<u32>(static_cast<u16>(~(var9_ | pop3.m_h2))) - 16; // count leading zeros in u16

		s32 var5 = pop3.m_h2 | var9_;
		if (var9 & 0x30)
		{
			var5 = 0;
		}
		else
		{
			var5 <<= var9;
		}

		s32 var3 = pop3.m_h1 + var9;
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

			if (var9 > 1 && static_cast<u32>(var8) > 1)
			{
				ensure((16 - var2 <= 1));
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
			var6 = queue->m_hs2[var11];
		}

		pop3.m_h1 = static_cast<u16>(var3);
		pop3.m_h2 = static_cast<u16>(var5);

		if (queue->pop2.compare_and_swap_test(old, pop2))
		{
			if (var6 != umax)
			{
				ensure((queue->pop3.compare_and_swap_test(old2, pop3)));
				ensure((fpSendSignal));
				return not_an_error(fpSendSignal(ppu, vm::cast(queue->m_eaSignal.addr()), var6));
			}
			else
			{
				pack = queue->pop2.load().pack;
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

error_code _cellSyncLFQueueCompletePopPointer2(ppu_thread&, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePopPointer
	cellSync.todo("_cellSyncLFQueueCompletePopPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	return CELL_OK;
}

error_code _cellSyncLFQueuePopBody(ppu_thread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePop has 1 in isBlocking param, cellSyncLFQueueTryPop has 0
	cellSync.warning("_cellSyncLFQueuePopBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || !buffer.aligned(16)) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	vm::var<s32> position;

	while (true)
	{
		s32 res;

		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = _cellSyncLFQueueGetPopPointer(ppu, queue, position, isBlocking, 0, 0);
		}
		else
		{
			res = _cellSyncLFQueueGetPopPointer2(ppu, queue, position, isBlocking, 0);
		}

		if (!isBlocking || res + 0u != CELL_SYNC_ERROR_AGAIN)
		{
			if (res) return not_an_error(res);

			break;
		}

		if (ppu.test_stopped())
		{
			return 0;
		}
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const s32 pos = *position;
	const u32 addr = vm::cast<u64>((queue->m_buffer.addr() & ~1) + size * (pos >= depth ? pos - depth : pos));
	std::memcpy(buffer.get_ptr(), vm::base(addr), size);

	if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
	{
		return _cellSyncLFQueueCompletePopPointer(ppu, queue, pos, vm::null, 0);
	}
	else
	{
		return _cellSyncLFQueueCompletePopPointer2(ppu, queue, pos, vm::null, 0);
	}
}

error_code cellSyncLFQueueClear(vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.warning("cellSyncLFQueueClear(queue=*0x%x)", queue);

	if (!queue) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop1.load();
		auto pop = old;

		// Loads must be in this order
		const auto push = queue->push1.load();

		s32 var1, var2;
		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			var1 = var2 = queue->pop2.load().pack;
		}
		else
		{
			var1 = push.m_h7;
			var2 = pop.m_h3;
		}

		if (static_cast<s16>(pop.m_h4) != +pop.m_h1 ||
			static_cast<s16>(push.m_h8) != +push.m_h5 ||
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

error_code cellSyncLFQueueSize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> size)
{
	cellSync.warning("cellSyncLFQueueSize(queue=*0x%x, size=*0x%x)", queue, size);

	if (!queue || !size) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop3.load();

		// Loads must be in this order
		u32 var1 = queue->pop1.load().m_h1;
		u32 var2 = queue->push1.load().m_h5;

		if (queue->pop3.compare_and_swap_test(old, old))
		{
			if (var1 <= var2)
			{
				*size = var2 - var1;
			}
			else
			{
				*size = var2 - var1 + queue->m_depth * 2;
			}

			return CELL_OK;
		}
	}
}

error_code cellSyncLFQueueDepth(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> depth)
{
	cellSync.trace("cellSyncLFQueueDepth(queue=*0x%x, depth=*0x%x)", queue, depth);

	if (!queue || !depth) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*depth = queue->m_depth;

	return CELL_OK;
}

error_code _cellSyncLFQueueGetSignalAddress(vm::cptr<CellSyncLFQueue> queue, vm::pptr<void> ppSignal)
{
	cellSync.trace("_cellSyncLFQueueGetSignalAddress(queue=*0x%x, ppSignal=**0x%x)", queue, ppSignal);

	if (!queue || !ppSignal) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*ppSignal = queue->m_eaSignal;

	return CELL_OK;
}

error_code cellSyncLFQueueGetDirection(vm::cptr<CellSyncLFQueue> queue, vm::ptr<u32> direction)
{
	cellSync.trace("cellSyncLFQueueGetDirection(queue=*0x%x, direction=*0x%x)", queue, direction);

	if (!queue || !direction) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*direction = queue->m_direction;

	return CELL_OK;
}

error_code cellSyncLFQueueGetEntrySize(vm::cptr<CellSyncLFQueue> queue, vm::ptr<u32> entry_size)
{
	cellSync.trace("cellSyncLFQueueGetEntrySize(queue=*0x%x, entry_size=*0x%x)", queue, entry_size);

	if (!queue || !entry_size) [[unlikely]]
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned()) [[unlikely]]
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*entry_size = queue->m_size;

	return CELL_OK;
}

error_code _cellSyncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.todo("_cellSyncLFQueueAttachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	return CELL_OK;
}

error_code _cellSyncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.todo("_cellSyncLFQueueDetachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSync)("cellSync", []()
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
