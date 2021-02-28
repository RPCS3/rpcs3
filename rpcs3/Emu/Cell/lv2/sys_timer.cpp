#include "stdafx.h"
#include "sys_timer.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_event.h"
#include "sys_process.h"

#include <thread>

LOG_CHANNEL(sys_timer);

extern u64 get_guest_system_time();

void lv2_timer_context::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		const u32 _state = +state;

		if (_state == SYS_TIMER_STATE_RUN)
		{
			const u64 _now = get_guest_system_time();
			u64 next = expire;

			if (_now >= next)
			{
				std::lock_guard lock(mutex);

				if (next = expire; _now < next)
				{
					// expire was updated in the middle, don't send an event
					continue;
				}

				if (const auto queue = port.lock())
				{
					queue->send(source, data1, data2, next);
				}

				if (period)
				{
					// Set next expiration time and check again
					expire += period;
					continue;
				}

				// Stop after oneshot
				state.release(SYS_TIMER_STATE_STOP);
				continue;
			}

			// TODO: use single global dedicated thread for busy waiting, no timer threads
			lv2_obj::wait_timeout(next - _now);
			continue;
		}

		thread_ctrl::wait_on(state, _state);
	}
}

error_code sys_timer_create(ppu_thread& ppu, vm::ptr<u32> timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_create(timer_id=*0x%x)", timer_id);

	if (const u32 id = idm::make<lv2_obj, lv2_timer>("Timer Thread"))
	{
		*timer_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_timer_destroy(ppu_thread& ppu, u32 timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_destroy(timer_id=0x%x)", timer_id);

	const auto timer = idm::withdraw<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		if (reader_lock lock(timer.mutex); lv2_event_queue::check(timer.port))
		{
			return CELL_EISCONN;
		}

		timer = thread_state::aborting;
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

error_code sys_timer_get_information(ppu_thread& ppu, u32 timer_id, vm::ptr<sys_timer_information_t> info)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_get_information(timer_id=0x%x, info=*0x%x)", timer_id, info);

	sys_timer_information_t _info{};

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer)
	{
		timer.get_information(_info);
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	std::memcpy(info.get_ptr(), &_info, info.size());
	return CELL_OK;
}

error_code _sys_timer_start(ppu_thread& ppu, u32 timer_id, u64 base_time, u64 period)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("_sys_timer_start(timer_id=0x%x, base_time=0x%llx, period=0x%llx)", timer_id, base_time, period);

	const u64 start_time = get_guest_system_time();

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
		std::unique_lock lock(timer.mutex);

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

		lock.unlock();
		timer.state.notify_one();
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

error_code sys_timer_stop(ppu_thread& ppu, u32 timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_stop()");

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [](lv2_timer& timer)
	{
		std::lock_guard lock(timer.mutex);

		timer.state = SYS_TIMER_STATE_STOP;
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_timer_connect_event_queue(ppu_thread& ppu, u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_connect_event_queue(timer_id=0x%x, queue_id=0x%x, name=0x%llx, data1=0x%llx, data2=0x%llx)", timer_id, queue_id, name, data1, data2);

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		const auto found = idm::find_unlocked<lv2_obj, lv2_event_queue>(queue_id);

		if (!found)
		{
			return CELL_ESRCH;
		}

		std::lock_guard lock(timer.mutex);

		if (lv2_event_queue::check(timer.port))
		{
			return CELL_EISCONN;
		}

		// Connect event queue
		timer.port   = std::static_pointer_cast<lv2_event_queue>(found->second);
		timer.source = name ? name : (s64{process_getpid()} << 32) | u64{timer_id};
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

error_code sys_timer_disconnect_event_queue(ppu_thread& ppu, u32 timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_disconnect_event_queue(timer_id=0x%x)", timer_id);

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [](lv2_timer& timer) -> CellError
	{
		std::lock_guard lock(timer.mutex);

		timer.state = SYS_TIMER_STATE_STOP;

		if (!lv2_event_queue::check(timer.port))
		{
			return CELL_ENOTCONN;
		}

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
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_sleep(sleep_time=%d) -> sys_timer_usleep()", sleep_time);

	return sys_timer_usleep(ppu, sleep_time * u64{1000000});
}

error_code sys_timer_usleep(ppu_thread& ppu, u64 sleep_time)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_usleep(sleep_time=0x%llx)", sleep_time);

	if (sleep_time)
	{
		lv2_obj::sleep(ppu, sleep_time);

		lv2_obj::wait_timeout<true>(sleep_time);
	}
	else
	{
		std::this_thread::yield();
	}

	return CELL_OK;
}
