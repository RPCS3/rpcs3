#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "cellSync.h"

#include "Emu/Memory/wait_engine.h"

LOG_CHANNEL(cellSync);

namespace _sync
{
	static inline be_t<u16> mutex_acquire(mutex& ctrl)
	{
		return ctrl.acq++;
	}

	static inline bool mutex_try_lock(mutex& ctrl)
	{
		return ctrl.acq++ == ctrl.rel;
	}

	static inline void mutex_unlock(mutex& ctrl)
	{
		ctrl.rel++;
	}
}

ppu_error_code cellSyncMutexInitialize(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexInitialize(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->ctrl.exchange({ 0, 0 });

	return CELL_OK;
}

ppu_error_code cellSyncMutexLock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexLock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// increase acq value and remember its old value
	const auto order = mutex->ctrl.atomic_op(_sync::mutex_acquire);

	// wait until rel value is equal to old acq value
	vm::wait_op(mutex.addr(), 4, WRAP_EXPR(mutex->ctrl.load().rel == order));

	_mm_mfence();

	return CELL_OK;
}

ppu_error_code cellSyncMutexTryLock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexTryLock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!mutex->ctrl.atomic_op(_sync::mutex_try_lock))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	return CELL_OK;
}

ppu_error_code cellSyncMutexUnlock(vm::ptr<CellSyncMutex> mutex)
{
	cellSync.trace("cellSyncMutexUnlock(mutex=*0x%x)", mutex);

	if (!mutex)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!mutex.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	mutex->ctrl.atomic_op(_sync::mutex_unlock);

	vm::notify_at(mutex.addr(), 4);

	return CELL_OK;
}

ppu_error_code cellSyncBarrierInitialize(vm::ptr<CellSyncBarrier> barrier, u16 total_count)
{
	cellSync.trace("cellSyncBarrierInitialize(barrier=*0x%x, total_count=%d)", barrier, total_count);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!total_count || total_count > 32767)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear current value, write total_count and sync
	barrier->ctrl.exchange({ 0, total_count });

	return CELL_OK;
}

ppu_error_code cellSyncBarrierNotify(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierNotify(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	vm::wait_op(barrier.addr(), 4, WRAP_EXPR(barrier->ctrl.atomic_op(_sync::barrier::try_notify)));

	vm::notify_at(barrier.addr(), 4);

	return CELL_OK;
}

ppu_error_code cellSyncBarrierTryNotify(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierTryNotify(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	_mm_mfence();

	if (!barrier->ctrl.atomic_op(_sync::barrier::try_notify))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	vm::notify_at(barrier.addr(), 4);

	return CELL_OK;
}

ppu_error_code cellSyncBarrierWait(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierWait(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	_mm_mfence();

	vm::wait_op(barrier.addr(), 4, WRAP_EXPR(barrier->ctrl.atomic_op(_sync::barrier::try_wait)));

	vm::notify_at(barrier.addr(), 4);

	return CELL_OK;
}

ppu_error_code cellSyncBarrierTryWait(vm::ptr<CellSyncBarrier> barrier)
{
	cellSync.trace("cellSyncBarrierTryWait(barrier=*0x%x)", barrier);

	if (!barrier)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!barrier.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	_mm_mfence();

	if (!barrier->ctrl.atomic_op(_sync::barrier::try_wait))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	vm::notify_at(barrier.addr(), 4);

	return CELL_OK;
}

ppu_error_code cellSyncRwmInitialize(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer, u32 buffer_size)
{
	cellSync.trace("cellSyncRwmInitialize(rwm=*0x%x, buffer=*0x%x, buffer_size=0x%x)", rwm, buffer, buffer_size);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned() || buffer % 128)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (buffer_size % 128 || buffer_size > 0x4000)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear readers and writers, write buffer_size, buffer addr and sync
	rwm->ctrl.store({ 0, 0 });
	rwm->size = buffer_size;
	rwm->buffer = buffer;

	_mm_mfence();

	return CELL_OK;
}

ppu_error_code cellSyncRwmRead(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncRwmRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// wait until `writers` is zero, increase `readers`
	vm::wait_op(rwm.addr(), 8, WRAP_EXPR(rwm->ctrl.atomic_op(_sync::rwlock::try_read_begin)));

	// copy data to buffer
	std::memcpy(buffer.get_ptr(), rwm->buffer.get_ptr(), rwm->size);

	// decrease `readers`, return error if already zero
	if (!rwm->ctrl.atomic_op(_sync::rwlock::try_read_end))
	{
		return CELL_SYNC_ERROR_ABORT;
	}

	vm::notify_at(rwm.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncRwmTryRead(vm::ptr<CellSyncRwm> rwm, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncRwmTryRead(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// increase `readers` if `writers` is zero
	if (!rwm->ctrl.atomic_op(_sync::rwlock::try_read_begin))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	// copy data to buffer
	std::memcpy(buffer.get_ptr(), rwm->buffer.get_ptr(), rwm->size);

	// decrease `readers`, return error if already zero
	if (!rwm->ctrl.atomic_op(_sync::rwlock::try_read_end))
	{
		return CELL_SYNC_ERROR_ABORT;
	}

	vm::notify_at(rwm.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncRwmWrite(vm::ptr<CellSyncRwm> rwm, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncRwmWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// wait until `writers` is zero, set to 1
	vm::wait_op(rwm.addr(), 8, WRAP_EXPR(rwm->ctrl.atomic_op(_sync::rwlock::try_write_begin)));

	// wait until `readers` is zero
	vm::wait_op(rwm.addr(), 8, WRAP_EXPR(!rwm->ctrl.load().readers));

	// copy data from buffer
	std::memcpy(rwm->buffer.get_ptr(), buffer.get_ptr(), rwm->size);

	// sync and clear `readers` and `writers`
	rwm->ctrl.exchange({ 0, 0 });

	vm::notify_at(rwm.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncRwmTryWrite(vm::ptr<CellSyncRwm> rwm, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncRwmTryWrite(rwm=*0x%x, buffer=*0x%x)", rwm, buffer);

	if (!rwm || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!rwm.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// set `writers` to 1 if `readers` and `writers` are zero
	if (!rwm->ctrl.compare_and_swap_test({ 0, 0 }, { 0, 1 }))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	// copy data from buffer
	std::memcpy(rwm->buffer.get_ptr(), buffer.get_ptr(), rwm->size);

	// sync and clear `readers` and `writers`
	rwm->ctrl.exchange({ 0, 0 });

	vm::notify_at(rwm.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueueInitialize(vm::ptr<CellSyncQueue> queue, vm::ptr<u8> buffer, u32 size, u32 depth)
{
	cellSync.trace("cellSyncQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x)", queue, buffer, size, depth);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (size && !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || buffer % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	if (!depth || size % 16)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	// clear sync var, write size, depth, buffer addr and sync
	queue->ctrl.store({ 0, 0 });
	queue->size = size;
	queue->depth = depth;
	queue->buffer = buffer;

	_mm_mfence();

	return CELL_OK;
}

ppu_error_code cellSyncQueuePush(vm::ptr<CellSyncQueue> queue, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	vm::wait_op(queue.addr(), 8, WRAP_EXPR(queue->ctrl.atomic_op(_sync::queue::try_push_begin, depth, &position)));

	// copy data from the buffer at the position
	std::memcpy(&queue->buffer[position * queue->size], buffer.get_ptr(), queue->size);

	// ...push_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._push = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueueTryPush(vm::ptr<CellSyncQueue> queue, vm::cptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPush(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	if (!queue->ctrl.atomic_op(_sync::queue::try_push_begin, depth, &position))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	// copy data from the buffer at the position
	std::memcpy(&queue->buffer[position * queue->size], buffer.get_ptr(), queue->size);

	// ...push_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._push = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueuePop(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();
	
	u32 position;

	vm::wait_op(queue.addr(), 8, WRAP_EXPR(queue->ctrl.atomic_op(_sync::queue::try_pop_begin, depth, &position)));

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	// ...pop_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._pop = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueueTryPop(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPop(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;
	
	if (!queue->ctrl.atomic_op(_sync::queue::try_pop_begin, depth, &position))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	// ...pop_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._pop = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueuePeek(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueuePeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	vm::wait_op(queue.addr(), 8, WRAP_EXPR(queue->ctrl.atomic_op(_sync::queue::try_peek_begin, depth, &position)));

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	// ...peek_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._pop = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueueTryPeek(vm::ptr<CellSyncQueue> queue, vm::ptr<void> buffer)
{
	cellSync.trace("cellSyncQueueTryPeek(queue=*0x%x, buffer=*0x%x)", queue, buffer);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	u32 position;

	if (!queue->ctrl.atomic_op(_sync::queue::try_peek_begin, depth, &position))
	{
		return NOT_AN_ERROR(CELL_SYNC_ERROR_BUSY);
	}

	// copy data at the position to the buffer
	std::memcpy(buffer.get_ptr(), &queue->buffer[position % depth * queue->size], queue->size);

	// ...peek_end
	queue->ctrl.atomic_op([](_sync::queue& ctrl) { ctrl._pop = 0; });

	vm::notify_at(queue.addr(), 8);

	return CELL_OK;
}

ppu_error_code cellSyncQueueSize(vm::ptr<CellSyncQueue> queue)
{
	cellSync.trace("cellSyncQueueSize(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	return NOT_AN_ERROR(queue->ctrl.load().count & 0xffffff);
}

ppu_error_code cellSyncQueueClear(vm::ptr<CellSyncQueue> queue)
{
	cellSync.trace("cellSyncQueueClear(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	const u32 depth = queue->check_depth();

	vm::wait_op(queue.addr(), 8, WRAP_EXPR(queue->ctrl.atomic_op(_sync::queue::try_clear_begin_1)));
	vm::wait_op(queue.addr(), 8, WRAP_EXPR(queue->ctrl.atomic_op(_sync::queue::try_clear_begin_2)));

	queue->ctrl.exchange({ 0, 0 });

	vm::notify_at(queue.addr(), 8);

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

ppu_error_code cellSyncLFQueueInitialize(vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 size, u32 depth, u32 direction, vm::ptr<void> eaSignal)
{
	cellSync.warning("cellSyncLFQueueInitialize(queue=*0x%x, buffer=*0x%x, size=0x%x, depth=0x%x, direction=%d, eaSignal=*0x%x)", queue, buffer, size, depth, direction, eaSignal);

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

	if (!depth || depth > 0x7fff || direction > 3)
	{
		return CELL_SYNC_ERROR_INVAL;
	}

	if (!queue.aligned() || buffer % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	// get sdk version of current process
	s32 sdk_ver;

	if (s32 ret = process_get_sdk_version(process_getpid(), sdk_ver))
	{
		return NOT_AN_ERROR(ret);
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
				for (const auto& data : vm::_ref<u64[16]>(queue.addr()))
				{
					if (data)
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
			if (queue->m_eaSignal != eaSignal || queue->m_direction != direction)
			{
				return CELL_SYNC_ERROR_INVAL;
			}
		}

		_mm_mfence();
	}
	else
	{
		syncLFQueueInitialize(queue, buffer, size, depth, direction, eaSignal);

		queue->init.exchange(0);
	}

	return CELL_OK;
}

ppu_error_code _cellSyncLFQueueGetPushPointer(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	cellSync.warning("_cellSyncLFQueueGetPushPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

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
			CHECK_EMU_STATUS;

			const auto old = queue->push1.load(); _mm_lfence();
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
			if (useEventQueue && ((s32)push.m_h5 != var2 || push.m_h7))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 -= (s32)(u16)queue->pop1.load().m_h1;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 < depth)
				{
					const s32 _pointer = (s16)push.m_h8;
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
					return NOT_AN_ERROR(res);
				}
				break;
			}
		}

		ASSERT(sys_event_queue_receive(ppu, queue->m_eq_id, vm::null, 0) == CELL_OK);
		var1 = 1;
	}
}

ppu_error_code _cellSyncLFQueueGetPushPointer2(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPushPointer
	cellSync.todo("_cellSyncLFQueueGetPushPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	throw EXCEPTION("");
}

ppu_error_code _cellSyncLFQueueCompletePushPointer(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	cellSync.warning("_cellSyncLFQueueCompletePushPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	if (queue->m_direction != CELL_SYNC_QUEUE_PPU2SPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->push2.load(); _mm_lfence();
		auto push2 = old;

		const auto old2 = queue->push3.load();
		auto push3 = old2;

		s32 var1 = pointer - (u16)push3.m_h5;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = (s32)(s16)queue->pop1.load().m_h4 - (s32)(u16)queue->pop1.load().m_h1;
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
				ASSERT(16 - var2 <= 1);
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
			ASSERT(var2 + var4 < 16);
			if (var6 != -1)
			{
				ASSERT(queue->push3.compare_and_swap_test(old2, push3));
				ASSERT(fpSendSignal);
				return NOT_AN_ERROR(fpSendSignal(ppu, (u32)queue->m_eaSignal.addr(), var6));
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

ppu_error_code _cellSyncLFQueueCompletePushPointer2(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer
	cellSync.todo("_cellSyncLFQueueCompletePushPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x)", queue, pointer, fpSendSignal);

	throw EXCEPTION("");
}

ppu_error_code _cellSyncLFQueuePushBody(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::cptr<void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePush has 1 in isBlocking param, cellSyncLFQueueTryPush has 0
	cellSync.warning("_cellSyncLFQueuePushBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || buffer % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	vm::var<s32> position;

	while (true)
	{
		CHECK_EMU_STATUS;

		s32 res;

		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = _cellSyncLFQueueGetPushPointer(ppu, queue, position, isBlocking, 0);
		}
		else
		{
			res = _cellSyncLFQueueGetPushPointer2(ppu, queue, position, isBlocking, 0);
		}

		if (!isBlocking || res != CELL_SYNC_ERROR_AGAIN)
		{
			if (res) return NOT_AN_ERROR(res);

			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const s32 pos = *position;
	const u32 addr = vm::cast((u64)((queue->m_buffer.addr() & ~1ull) + size * (pos >= depth ? pos - depth : pos)), HERE);
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

ppu_error_code _cellSyncLFQueueGetPopPointer(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 arg4, u32 useEventQueue)
{
	cellSync.warning("_cellSyncLFQueueGetPopPointer(queue=*0x%x, pointer=*0x%x, isBlocking=%d, arg4=%d, useEventQueue=%d)", queue, pointer, isBlocking, arg4, useEventQueue);

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
			CHECK_EMU_STATUS;

			const auto old = queue->pop1.load(); _mm_lfence();
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
			if (useEventQueue && ((s32)(u16)pop.m_h1 != var2 || pop.m_h3))
			{
				res = CELL_SYNC_ERROR_BUSY;
			}
			else
			{
				var2 = (s32)(u16)queue->push1.load().m_h5 - var2;
				if (var2 < 0)
				{
					var2 += depth * 2;
				}

				if (var2 > 0)
				{
					const s32 _pointer = (s16)pop.m_h4;
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
					return NOT_AN_ERROR(res);
				}
				break;
			}
		}

		ASSERT(sys_event_queue_receive(ppu, queue->m_eq_id, vm::null, 0) == CELL_OK);
		var1 = 1;
	}
}

ppu_error_code _cellSyncLFQueueGetPopPointer2(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<s32> pointer, u32 isBlocking, u32 useEventQueue)
{
	// arguments copied from _cellSyncLFQueueGetPopPointer
	cellSync.todo("_cellSyncLFQueueGetPopPointer2(queue=*0x%x, pointer=*0x%x, isBlocking=%d, useEventQueue=%d)", queue, pointer, isBlocking, useEventQueue);

	throw EXCEPTION("");
}

ppu_error_code _cellSyncLFQueueCompletePopPointer(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePushPointer + unknown argument (noQueueFull taken from LFQueue2CompletePopPointer)
	cellSync.warning("_cellSyncLFQueueCompletePopPointer(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	if (queue->m_direction != CELL_SYNC_QUEUE_SPU2PPU)
	{
		return CELL_SYNC_ERROR_PERM;
	}

	const s32 depth = queue->m_depth;

	while (true)
	{
		const auto old = queue->pop2.load(); _mm_lfence();
		auto pop2 = old;

		const auto old2 = queue->pop3.load();
		auto pop3 = old2;

		s32 var1 = pointer - (u16)pop3.m_h1;
		if (var1 < 0)
		{
			var1 += depth * 2;
		}

		s32 var2 = (s32)(s16)queue->push1.load().m_h8 - (s32)(u16)queue->push1.load().m_h5;
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
				ASSERT(16 - var2 <= 1);
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
				ASSERT(queue->pop3.compare_and_swap_test(old2, pop3));
				ASSERT(fpSendSignal);
				return NOT_AN_ERROR(fpSendSignal(ppu, (u32)queue->m_eaSignal.addr(), var6));
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

ppu_error_code _cellSyncLFQueueCompletePopPointer2(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, s32 pointer, vm::ptr<s32(u32 addr, u32 arg)> fpSendSignal, u32 noQueueFull)
{
	// arguments copied from _cellSyncLFQueueCompletePopPointer
	cellSync.todo("_cellSyncLFQueueCompletePopPointer2(queue=*0x%x, pointer=%d, fpSendSignal=*0x%x, noQueueFull=%d)", queue, pointer, fpSendSignal, noQueueFull);

	throw EXCEPTION("");
}

ppu_error_code _cellSyncLFQueuePopBody(PPUThread& ppu, vm::ptr<CellSyncLFQueue> queue, vm::ptr<void> buffer, u32 isBlocking)
{
	// cellSyncLFQueuePop has 1 in isBlocking param, cellSyncLFQueueTryPop has 0
	cellSync.warning("_cellSyncLFQueuePopBody(queue=*0x%x, buffer=*0x%x, isBlocking=%d)", queue, buffer, isBlocking);

	if (!queue || !buffer)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned() || buffer % 16)
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	vm::var<s32> position;

	while (true)
	{
		CHECK_EMU_STATUS;

		s32 res;

		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			res = _cellSyncLFQueueGetPopPointer(ppu, queue, position, isBlocking, 0, 0);
		}
		else
		{
			res = _cellSyncLFQueueGetPopPointer2(ppu, queue, position, isBlocking, 0);
		}

		if (!isBlocking || res != CELL_SYNC_ERROR_AGAIN)
		{
			if (res) return NOT_AN_ERROR(res);

			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	const s32 depth = queue->m_depth;
	const s32 size = queue->m_size;
	const s32 pos = *position;
	const u32 addr = vm::cast((u64)((queue->m_buffer.addr() & ~1) + size * (pos >= depth ? pos - depth : pos)), HERE);
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

ppu_error_code cellSyncLFQueueClear(vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.warning("cellSyncLFQueueClear(queue=*0x%x)", queue);

	if (!queue)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop1.load(); _mm_lfence();
		auto pop = old;

		const auto push = queue->push1.load();

		s32 var1, var2;
		if (queue->m_direction != CELL_SYNC_QUEUE_ANY2ANY)
		{
			var1 = var2 = (u16)queue->pop2.load().pack;
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

ppu_error_code cellSyncLFQueueSize(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> size)
{
	cellSync.warning("cellSyncLFQueueSize(queue=*0x%x, size=*0x%x)", queue, size);

	if (!queue || !size)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	while (true)
	{
		const auto old = queue->pop3.load(); _mm_lfence();

		u32 var1 = (u16)queue->pop1.load().m_h1;
		u32 var2 = (u16)queue->push1.load().m_h5;

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

ppu_error_code cellSyncLFQueueDepth(vm::ptr<CellSyncLFQueue> queue, vm::ptr<u32> depth)
{
	cellSync.trace("cellSyncLFQueueDepth(queue=*0x%x, depth=*0x%x)", queue, depth);

	if (!queue || !depth)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*depth = queue->m_depth;

	return CELL_OK;
}

ppu_error_code _cellSyncLFQueueGetSignalAddress(vm::cptr<CellSyncLFQueue> queue, vm::pptr<void> ppSignal)
{
	cellSync.trace("_cellSyncLFQueueGetSignalAddress(queue=*0x%x, ppSignal=**0x%x)", queue, ppSignal);

	if (!queue || !ppSignal)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*ppSignal = queue->m_eaSignal;

	return CELL_OK;
}

ppu_error_code cellSyncLFQueueGetDirection(vm::cptr<CellSyncLFQueue> queue, vm::ptr<u32> direction)
{
	cellSync.trace("cellSyncLFQueueGetDirection(queue=*0x%x, direction=*0x%x)", queue, direction);

	if (!queue || !direction)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*direction = queue->m_direction;

	return CELL_OK;
}

ppu_error_code cellSyncLFQueueGetEntrySize(vm::cptr<CellSyncLFQueue> queue, vm::ptr<u32> entry_size)
{
	cellSync.trace("cellSyncLFQueueGetEntrySize(queue=*0x%x, entry_size=*0x%x)", queue, entry_size);

	if (!queue || !entry_size)
	{
		return CELL_SYNC_ERROR_NULL_POINTER;
	}

	if (!queue.aligned())
	{
		return CELL_SYNC_ERROR_ALIGN;
	}

	*entry_size = queue->m_size;

	return CELL_OK;
}

ppu_error_code _cellSyncLFQueueAttachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.todo("_cellSyncLFQueueAttachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	throw EXCEPTION("");
}

ppu_error_code _cellSyncLFQueueDetachLv2EventQueue(vm::ptr<u32> spus, u32 num, vm::ptr<CellSyncLFQueue> queue)
{
	cellSync.todo("_cellSyncLFQueueDetachLv2EventQueue(spus=*0x%x, num=%d, queue=*0x%x)", spus, num, queue);

	throw EXCEPTION("");
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
