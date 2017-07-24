#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_rwlock.h"

namespace vm { using namespace ps3; }

logs::channel sys_rwlock("sys_rwlock");

extern u64 get_system_time();

error_code sys_rwlock_create(vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr)
{
	sys_rwlock.warning("sys_rwlock_create(rw_lock_id=*0x%x, attr=*0x%x)", rw_lock_id, attr);

	if (!rw_lock_id || !attr)
	{
		return CELL_EFAULT;
	}

	const u32 protocol = attr->protocol;

	if (protocol == SYS_SYNC_PRIORITY_INHERIT)
		sys_rwlock.todo("sys_rwlock_create(): SYS_SYNC_PRIORITY_INHERIT");

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY && protocol != SYS_SYNC_PRIORITY_INHERIT)
	{
		sys_rwlock.error("sys_rwlock_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	if (attr->pshared != SYS_SYNC_NOT_PROCESS_SHARED || attr->ipc_key || attr->flags)
	{
		sys_rwlock.error("sys_rwlock_create(): unknown attributes (pshared=0x%x, ipc_key=0x%llx, flags=0x%x)", attr->pshared, attr->ipc_key, attr->flags);
		return CELL_EINVAL;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_rwlock>(protocol, attr->name_u64))
	{
		*rw_lock_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_rwlock_destroy(u32 rw_lock_id)
{
	sys_rwlock.warning("sys_rwlock_destroy(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::withdraw<lv2_obj, lv2_rwlock>(rw_lock_id, [](lv2_rwlock& rw) -> CellError
	{
		if (rw.owner)
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret)
	{
		return rwlock.ret;
	}

	return CELL_OK;
}

error_code sys_rwlock_rlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.trace("sys_rwlock_rlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const auto rwlock = idm::get<lv2_obj, lv2_rwlock>(rw_lock_id, [&](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		if (val <= 0 && !(val & 1))
		{
			if (rwlock.owner.compare_and_swap_test(val, val - 2))
			{
				return true;
			}
		}

		semaphore_lock lock(rwlock.mutex);

		const s64 _old = rwlock.owner.fetch_op([&](s64& val)
		{
			if (val <= 0 && !(val & 1))
			{
				val -= 2;
			}
			else
			{
				val |= 1;
			}
		});

		if (_old > 0 || _old & 1)
		{
			rwlock.rq.emplace_back(&ppu);
			rwlock.sleep(ppu, timeout);
			return false;
		}

		return true;
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret)
	{
		return CELL_OK;
	}

	ppu.gpr[3] = CELL_OK;

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock lock(rwlock->mutex);

				if (!rwlock->unqueue(rwlock->rq, &ppu))
				{
					timeout = 0;
					continue;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}

			thread_ctrl::wait_for(timeout - passed);
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_rwlock_tryrlock(u32 rw_lock_id)
{
	sys_rwlock.trace("sys_rwlock_tryrlock(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::check<lv2_obj, lv2_rwlock>(rw_lock_id, [](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		if (val <= 0 && !(val & 1))
		{
			if (rwlock.owner.compare_and_swap_test(val, val - 2))
			{
				return true;
			}
		}

		return false;
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (!rwlock.ret)
	{
		return not_an_error(CELL_EBUSY);
	}

	return CELL_OK;
}

error_code sys_rwlock_runlock(ppu_thread& ppu, u32 rw_lock_id)
{
	sys_rwlock.trace("sys_rwlock_runlock(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::get<lv2_obj, lv2_rwlock>(rw_lock_id, [](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		if (val < 0 && !(val & 1))
		{
			if (rwlock.owner.compare_and_swap_test(val, val + 2))
			{
				return true;
			}
		}

		return false;
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret)
	{
		return CELL_OK;
	}
	else
	{
		semaphore_lock lock(rwlock->mutex);

		// Remove one reader
		const s64 _old = rwlock->owner.fetch_op([](s64& val)
		{
			if (val < -1)
			{
				val += 2;
			}
		});

		if (_old >= 0)
		{
			return CELL_EPERM;
		}

		if (_old == -1)
		{
			if (const auto cpu = rwlock->schedule<ppu_thread>(rwlock->wq, rwlock->protocol))
			{
				rwlock->owner = cpu->id << 1 | !rwlock->wq.empty();

				rwlock->awake(*cpu);
			}
			else
			{
				rwlock->owner = 0;

				verify(HERE), rwlock->rq.empty();
			}
		}
	}

	return CELL_OK;
}

error_code sys_rwlock_wlock(ppu_thread& ppu, u32 rw_lock_id, u64 timeout)
{
	sys_rwlock.trace("sys_rwlock_wlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const auto rwlock = idm::get<lv2_obj, lv2_rwlock>(rw_lock_id, [&](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		if (val == 0)
		{
			if (rwlock.owner.compare_and_swap_test(0, ppu.id << 1))
			{
				return true;
			}
		}
		else if (val >> 1 == ppu.id)
		{
			return false;
		}

		semaphore_lock lock(rwlock.mutex);

		const s64 _old = rwlock.owner.fetch_op([&](s64& val)
		{
			if (val == 0)
			{
				val = ppu.id << 1;
			}
			else
			{
				val |= 1;
			}
		});

		if (_old != 0)
		{
			rwlock.wq.emplace_back(&ppu);
			rwlock.sleep(ppu, timeout);
			return false;
		}

		return true;
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret)
	{
		return CELL_OK;
	}

	if (rwlock->owner >> 1 == ppu.id)
	{
		return CELL_EDEADLK;
	}

	ppu.gpr[3] = CELL_OK;

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (timeout)
		{
			const u64 passed = get_system_time() - ppu.start_time;

			if (passed >= timeout)
			{
				semaphore_lock lock(rwlock->mutex);

				if (!rwlock->unqueue(rwlock->wq, &ppu))
				{
					timeout = 0;
					continue;
				}

				// If the last waiter quit the writer sleep queue, readers must acquire the lock
				if (!rwlock->rq.empty() && rwlock->wq.empty())
				{
					rwlock->owner = (s64{-2} * rwlock->rq.size()) | 1;

					while (auto cpu = rwlock->schedule<ppu_thread>(rwlock->rq, SYS_SYNC_PRIORITY))
					{
						rwlock->awake(*cpu);
					}

					rwlock->owner &= ~1;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}

			thread_ctrl::wait_for(timeout - passed);
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_rwlock_trywlock(ppu_thread& ppu, u32 rw_lock_id)
{
	sys_rwlock.trace("sys_rwlock_trywlock(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::check<lv2_obj, lv2_rwlock>(rw_lock_id, [&](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		// Return previous value
		return val ? val : rwlock.owner.compare_and_swap(0, ppu.id << 1);
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret != 0)
	{
		if (rwlock.ret >> 1 == ppu.id)
		{
			return CELL_EDEADLK;
		}
		
		return not_an_error(CELL_EBUSY);
	}

	return CELL_OK;
}

error_code sys_rwlock_wunlock(ppu_thread& ppu, u32 rw_lock_id)
{
	sys_rwlock.trace("sys_rwlock_wunlock(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::get<lv2_obj, lv2_rwlock>(rw_lock_id, [&](lv2_rwlock& rwlock)
	{
		const s64 val = rwlock.owner;

		// Return previous value
		return val != ppu.id << 1 ? val : rwlock.owner.compare_and_swap(val, 0);
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret >> 1 != ppu.id)
	{
		return CELL_EPERM;
	}

	if (rwlock.ret & 1)
	{
		semaphore_lock lock(rwlock->mutex);

		if (auto cpu = rwlock->schedule<ppu_thread>(rwlock->wq, rwlock->protocol))
		{
			rwlock->owner = cpu->id << 1 | !rwlock->wq.empty();

			rwlock->awake(*cpu);
		}
		else if (auto readers = rwlock->rq.size())
		{
			rwlock->owner = (s64{-2} * readers) | 1;

			while (auto cpu = rwlock->schedule<ppu_thread>(rwlock->rq, SYS_SYNC_PRIORITY))
			{
				rwlock->awake(*cpu);
			}

			rwlock->owner &= ~1;
		}
		else
		{
			rwlock->owner = 0;
		}
	}

	return CELL_OK;
}
