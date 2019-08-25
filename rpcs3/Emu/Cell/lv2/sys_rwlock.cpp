#include "stdafx.h"
#include "sys_rwlock.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_rwlock);

template<> DECLARE(ipc_manager<lv2_rwlock, u64>::g_ipc) {};

error_code sys_rwlock_create(ppu_thread& ppu, vm::ptr<u32> rw_lock_id, vm::ptr<sys_rwlock_attribute_t> attr)
{
	vm::temporary_unlock(ppu);

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

	if (auto error = lv2_obj::create<lv2_rwlock>(attr->pshared, attr->ipc_key, attr->flags, [&]
	{
		return std::make_shared<lv2_rwlock>(protocol, attr->pshared, attr->ipc_key, attr->flags, attr->name_u64);
	}))
	{
		return error;
	}

	*rw_lock_id = idm::last_id();
	return CELL_OK;
}

error_code sys_rwlock_destroy(ppu_thread& ppu, u32 rw_lock_id)
{
	vm::temporary_unlock(ppu);

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
	vm::temporary_unlock(ppu);

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

		std::lock_guard lock(rwlock.mutex);

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
		if (ppu.is_stopped())
		{
			return 0;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				ppu.check_state();

				std::lock_guard lock(rwlock->mutex);

				if (!rwlock->unqueue(rwlock->rq, &ppu))
				{
					timeout = 0;
					continue;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}
		}
		else
		{
			thread_ctrl::wait();
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_rwlock_tryrlock(ppu_thread& ppu, u32 rw_lock_id)
{
	vm::temporary_unlock(ppu);

	sys_rwlock.trace("sys_rwlock_tryrlock(rw_lock_id=0x%x)", rw_lock_id);

	const auto rwlock = idm::check<lv2_obj, lv2_rwlock>(rw_lock_id, [](lv2_rwlock& rwlock)
	{
		auto [_, ok] = rwlock.owner.fetch_op([](s64& val)
		{
			if (val <= 0 && !(val & 1))
			{
				val -= 2;
				return true;
			}

			return false;
		});

		return ok;
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
	vm::temporary_unlock(ppu);

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
		std::lock_guard lock(rwlock->mutex);

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
				rwlock->owner = cpu->id << 1 | !rwlock->wq.empty() | !rwlock->rq.empty();

				rwlock->awake(cpu);
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
	vm::temporary_unlock(ppu);

	sys_rwlock.trace("sys_rwlock_wlock(rw_lock_id=0x%x, timeout=0x%llx)", rw_lock_id, timeout);

	const auto rwlock = idm::get<lv2_obj, lv2_rwlock>(rw_lock_id, [&](lv2_rwlock& rwlock) -> s64
	{
		const s64 val = rwlock.owner;

		if (val == 0)
		{
			if (rwlock.owner.compare_and_swap_test(0, ppu.id << 1))
			{
				return 0;
			}
		}
		else if (val >> 1 == ppu.id)
		{
			return val;
		}

		std::lock_guard lock(rwlock.mutex);

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
		}

		return _old;
	});

	if (!rwlock)
	{
		return CELL_ESRCH;
	}

	if (rwlock.ret == 0)
	{
		return CELL_OK;
	}

	if (rwlock.ret >> 1 == ppu.id)
	{
		return CELL_EDEADLK;
	}

	ppu.gpr[3] = CELL_OK;

	while (!ppu.state.test_and_reset(cpu_flag::signal))
	{
		if (ppu.is_stopped())
		{
			return 0;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				ppu.check_state();

				std::lock_guard lock(rwlock->mutex);

				if (!rwlock->unqueue(rwlock->wq, &ppu))
				{
					timeout = 0;
					continue;
				}

				// If the last waiter quit the writer sleep queue, wake blocked readers
				if (!rwlock->rq.empty() && rwlock->wq.empty() && rwlock->owner < 0)
				{
					rwlock->owner.atomic_op([&](s64& owner)
					{
						owner -= -2 * static_cast<s64>(rwlock->rq.size()); // Add readers to value
						owner &= -2; // Clear wait bit
					});

					// Protocol doesn't matter here since they are all enqueued anyways
					while (auto cpu = rwlock->schedule<ppu_thread>(rwlock->rq, SYS_SYNC_FIFO))
					{
						rwlock->append(cpu);
					}

					lv2_obj::awake_all();
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}
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
	vm::temporary_unlock(ppu);

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
	vm::temporary_unlock(ppu);

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
		std::lock_guard lock(rwlock->mutex);

		if (auto cpu = rwlock->schedule<ppu_thread>(rwlock->wq, rwlock->protocol))
		{
			rwlock->owner = cpu->id << 1 | !rwlock->wq.empty() | !rwlock->rq.empty();

			rwlock->awake(cpu);
		}
		else if (auto readers = rwlock->rq.size())
		{
			while (auto cpu = rwlock->schedule<ppu_thread>(rwlock->rq, SYS_SYNC_FIFO))
			{
				rwlock->append(cpu);
			}

			rwlock->owner.release(-2 * static_cast<s64>(readers));
			lv2_obj::awake_all();
		}
		else
		{
			rwlock->owner = 0;
		}
	}

	return CELL_OK;
}
