#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "sys_event.h"
#include "sys_process.h"
#include "sys_timer.h"

#include <thread>

logs::channel sys_timer("sys_timer", logs::level::notice);

extern u64 get_system_time();

extern std::mutex& get_current_thread_mutex();

void lv2_timer_t::on_task()
{
	std::unique_lock<std::mutex> lock(get_current_thread_mutex());

	while (state <= SYS_TIMER_STATE_RUN)
	{
		CHECK_EMU_STATUS;

		if (state == SYS_TIMER_STATE_RUN)
		{
			LV2_LOCK;

			while (get_system_time() >= expire)
			{
				const auto queue = port.lock();

				if (queue)
				{
					queue->push(lv2_lock, source, data1, data2, expire);
				}

				if (period && queue)
				{
					expire += period; // set next expiration time

					continue; // hack: check again
				}
				else
				{
					state = SYS_TIMER_STATE_STOP; // stop if oneshot or the event port was disconnected (TODO: is it correct?)

					break;
				}
			}

			continue;
		}

		get_current_thread_cv().wait_for(lock, 1ms);
	}
}

std::string lv2_timer_t::get_name() const
{
	return fmt::format("Timer Thread[0x%x]", id);
}

void lv2_timer_t::on_stop()
{
	// Signal thread using invalid state and join
	state = -1;
	(*this)->lock_notify();
	named_thread::on_stop();
}

s32 sys_timer_create(vm::ptr<u32> timer_id)
{
	sys_timer.warning("sys_timer_create(timer_id=*0x%x)", timer_id);

	*timer_id = idm::make<lv2_timer_t>();

	return CELL_OK;
}

s32 sys_timer_destroy(u32 timer_id)
{
	sys_timer.warning("sys_timer_destroy(timer_id=0x%x)", timer_id);

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (!timer->port.expired())
	{
		return CELL_EISCONN;
	}

	idm::remove<lv2_timer_t>(timer_id);

	return CELL_OK;
}

s32 sys_timer_get_information(u32 timer_id, vm::ptr<sys_timer_information_t> info)
{
	sys_timer.warning("sys_timer_get_information(timer_id=0x%x, info=*0x%x)", timer_id, info);

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);

	if (!timer)
	{
		return CELL_ESRCH;
	}

	info->next_expiration_time = timer->expire;

	info->period      = timer->period;
	info->timer_state = timer->state;

	return CELL_OK;
}

s32 _sys_timer_start(u32 timer_id, u64 base_time, u64 period)
{
	sys_timer.warning("_sys_timer_start(timer_id=0x%x, base_time=0x%llx, period=0x%llx)", timer_id, base_time, period);

	const u64 start_time = get_system_time();

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer->state != SYS_TIMER_STATE_STOP)
	{
		return CELL_EBUSY;
	}

	if (!period)
	{
		// oneshot timer (TODO: what will happen if both args are 0?)

		if (start_time >= base_time)
		{
			return CELL_ETIMEDOUT;
		}
	}
	else
	{
		// periodic timer

		if (period < 100)
		{
			return CELL_EINVAL;
		}
	}

	if (timer->port.expired())
	{
		return CELL_ENOTCONN;
	}

	// sys_timer_start_periodic() will use current time (TODO: is it correct?)
	timer->expire = base_time ? base_time : start_time + period;
	timer->period = period;
	timer->state  = SYS_TIMER_STATE_RUN;

	(*timer)->lock_notify();

	return CELL_OK;
}

s32 sys_timer_stop(u32 timer_id)
{
	sys_timer.warning("sys_timer_stop()");

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);

	if (!timer)
	{
		return CELL_ESRCH;
	}

	timer->state = SYS_TIMER_STATE_STOP; // stop timer

	return CELL_OK;
}

s32 sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2)
{
	sys_timer.warning("sys_timer_connect_event_queue(timer_id=0x%x, queue_id=0x%x, name=0x%llx, data1=0x%llx, data2=0x%llx)", timer_id, queue_id, name, data1, data2);

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);
	const auto queue = idm::get<lv2_event_queue_t>(queue_id);

	if (!timer || !queue)
	{
		return CELL_ESRCH;
	}

	if (!timer->port.expired())
	{
		return CELL_EISCONN;
	}

	timer->port   = queue; // connect event queue
	timer->source = name ? name : ((u64)process_getpid() << 32) | timer_id;
	timer->data1  = data1;
	timer->data2  = data2;

	return CELL_OK;
}

s32 sys_timer_disconnect_event_queue(u32 timer_id)
{
	sys_timer.warning("sys_timer_disconnect_event_queue(timer_id=0x%x)", timer_id);

	LV2_LOCK;

	const auto timer = idm::get<lv2_timer_t>(timer_id);

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer->port.expired())
	{
		return CELL_ENOTCONN;
	}

	timer->port.reset(); // disconnect event queue
	timer->state = SYS_TIMER_STATE_STOP; // stop timer

	return CELL_OK;
}

#include <thread>

s32 sys_timer_sleep(u32 sleep_time)
{
	sys_timer.trace("sys_timer_sleep(sleep_time=%d)", sleep_time);

	const u64 start_time = get_system_time();

	const u64 useconds = sleep_time * 1000000ull;

	u64 passed;

	while (useconds > (passed = get_system_time() - start_time) + 1000)
	{
		CHECK_EMU_STATUS;

		std::this_thread::sleep_for(1ms);
	}
	
	if (useconds > passed)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(useconds - passed));
	}

	CHECK_EMU_STATUS;
	return CELL_OK;
}

s32 sys_timer_usleep(const u64 sleep_time)
{
	sys_timer.trace("sys_timer_usleep(sleep_time=0x%llx)", sleep_time);

	const u64 start_time = get_system_time();

	u64 passed;

	while (sleep_time > (passed = get_system_time() - start_time) + 1000)
	{
		CHECK_EMU_STATUS;

		std::this_thread::sleep_for(1ms);
	}

	if (sleep_time > passed)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(sleep_time - passed));
	}

	CHECK_EMU_STATUS;
	return CELL_OK;
}
