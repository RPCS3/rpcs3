#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_event.h"
#include "sys_process.h"
#include "sys_timer.h"

#include <thread>



logs::channel sys_timer("sys_timer");

extern u64 get_system_time();

void lv2_timer::on_task()
{
	while (!Emu.IsStopped())
	{
		const u32 _state = state;

		if (_state == SYS_TIMER_STATE_RUN)
		{
			const u64 _now = get_system_time();
			const u64 next = expire;

			if (_now >= next)
			{
				semaphore_lock lock(mutex);

				if (const auto queue = port.lock())
				{
					queue->send(source, data1, data2, next);

					if (period)
					{
						// Set next expiration time and check again (HACK)
						expire += period;
						continue;
					}
				}

				// Stop: oneshot or the event port was disconnected (TODO: is it correct?)
				state = SYS_TIMER_STATE_STOP;
				continue;
			}

			// TODO: use single global dedicated thread for busy waiting, no timer threads
			lv2_obj::sleep_timeout(*this, next - _now);
			thread_ctrl::wait_for(next - _now);
		}
		else if (_state == SYS_TIMER_STATE_STOP)
		{
			thread_ctrl::wait_for(10000);
		}
		else
		{
			break;
		}
	}
}

void lv2_timer::on_stop()
{
	// Signal thread using invalid state
	state = -1;
	notify();
	join();
}

error_code sys_timer_create(vm::ptr<u32> timer_id)
{
	sys_timer.warning("sys_timer_create(timer_id=*0x%x)", timer_id);

	if (const u32 id = idm::make<lv2_obj, lv2_timer>())
	{
		*timer_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_timer_destroy(u32 timer_id)
{
	sys_timer.warning("sys_timer_destroy(timer_id=0x%x)", timer_id);

	const auto timer = idm::withdraw<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		semaphore_lock lock(timer.mutex);

		if (!timer.port.expired())
		{
			return CELL_EISCONN;
		}

		return {};
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer.ret)
	{
		return timer.ret;
	}

	return CELL_OK;
}

error_code sys_timer_get_information(u32 timer_id, vm::ptr<sys_timer_information_t> info)
{
	sys_timer.trace("sys_timer_get_information(timer_id=0x%x, info=*0x%x)", timer_id, info);

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer)
	{
		semaphore_lock lock(timer.mutex);

		info->next_expire = timer.expire;
		info->period      = timer.period;
		info->timer_state = timer.state;
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code _sys_timer_start(u32 timer_id, u64 base_time, u64 period)
{
	sys_timer.trace("_sys_timer_start(timer_id=0x%x, base_time=0x%llx, period=0x%llx)", timer_id, base_time, period);

	const u64 start_time = get_system_time();

	if (!period && start_time >= base_time)
	{
		// Invalid oneshot (TODO: what will happen if both args are 0?)
		return not_an_error(CELL_ETIMEDOUT);
	}

	if (period && period < 100)
	{
		// Invalid periodic timer
		return CELL_EINVAL;
	}

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		semaphore_lock lock(timer.mutex);

		if (timer.state != SYS_TIMER_STATE_STOP)
		{
			return CELL_EBUSY;
		}

		if (timer.port.expired())
		{
			return CELL_ENOTCONN;
		}

		// sys_timer_start_periodic() will use current time (TODO: is it correct?)
		timer.expire = base_time ? base_time : start_time + period;
		timer.period = period;
		timer.state  = SYS_TIMER_STATE_RUN;
		timer.notify();
		return {};
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer.ret)
	{
		return timer.ret;
	}

	return CELL_OK;
}

error_code sys_timer_stop(u32 timer_id)
{
	sys_timer.trace("sys_timer_stop()");

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [](lv2_timer& timer)
	{
		semaphore_lock lock(timer.mutex);

		timer.state = SYS_TIMER_STATE_STOP;
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2)
{
	sys_timer.warning("sys_timer_connect_event_queue(timer_id=0x%x, queue_id=0x%x, name=0x%llx, data1=0x%llx, data2=0x%llx)", timer_id, queue_id, name, data1, data2);

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		const auto found = idm::find_unlocked<lv2_obj, lv2_event_queue>(queue_id);

		if (!found)
		{
			return CELL_ESRCH;
		}

		semaphore_lock lock(timer.mutex);

		if (!timer.port.expired())
		{
			return CELL_EISCONN;
		}

		// Connect event queue
		timer.port   = std::static_pointer_cast<lv2_event_queue>(found->second);
		timer.source = name ? name : ((u64)process_getpid() << 32) | timer_id;
		timer.data1  = data1;
		timer.data2  = data2;
		return {};
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer.ret)
	{
		return timer.ret;
	}

	return CELL_OK;
}

error_code sys_timer_disconnect_event_queue(u32 timer_id)
{
	sys_timer.warning("sys_timer_disconnect_event_queue(timer_id=0x%x)", timer_id);

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [](lv2_timer& timer) -> CellError
	{
		semaphore_lock lock(timer.mutex);

		if (timer.port.expired())
		{
			return CELL_ENOTCONN;
		}

		timer.state = SYS_TIMER_STATE_STOP;
		timer.port.reset();
		return {};
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer.ret)
	{
		return timer.ret;
	}

	return CELL_OK;
}

error_code sys_timer_sleep(ppu_thread& ppu, u32 sleep_time)
{
	vm::temporary_unlock(ppu);

	sys_timer.trace("sys_timer_sleep(sleep_time=%d) -> sys_timer_usleep()", sleep_time);

	return sys_timer_usleep(ppu, sleep_time * u64{1000000});
}

error_code sys_timer_usleep(ppu_thread& ppu, u64 sleep_time)
{
	vm::temporary_unlock(ppu);

	sys_timer.trace("sys_timer_usleep(sleep_time=0x%llx)", sleep_time);

	if (sleep_time)
	{
#ifdef __linux__
		constexpr u32 host_min_quantum = 100;
#else
		// Host scheduler quantum for windows (worst case)
		// NOTE: On ps3 this function has very high accuracy
		constexpr u32 host_min_quantum = 500;
#endif

		u64 passed = 0;
		u64 remaining;

		lv2_obj::sleep(ppu, sleep_time);

		while (sleep_time >= passed)
		{
			remaining = sleep_time - passed;

			if (remaining > host_min_quantum)
			{
				// Wait on multiple of min quantum for large durations
				thread_ctrl::wait_for(remaining - (remaining % host_min_quantum));
			}
			else
			{
				// Try yielding. May cause long wake latency but helps weaker CPUs a lot by alleviating resource pressure
				std::this_thread::yield();
			}

			passed = (get_system_time() - ppu.start_time);
		}
	}
	else
	{
		lv2_obj::yield(ppu);
	}

	return CELL_OK;
}
