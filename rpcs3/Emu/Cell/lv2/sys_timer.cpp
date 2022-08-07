#include "stdafx.h"
#include "sys_timer.h"

#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/System.h"
#include "sys_event.h"
#include "sys_process.h"

#include <thread>
#include <deque>

LOG_CHANNEL(sys_timer);

struct lv2_timer_thread
{
	shared_mutex mutex;
	std::deque<std::shared_ptr<lv2_timer>> timers;

	lv2_timer_thread();
	void operator()();

	SAVESTATE_INIT_POS(46); // Dependency ion LV2 objects (lv2_timer)

	static constexpr auto thread_name = "Timer Thread"sv;
};

lv2_timer::lv2_timer(utils::serial& ar)
	: lv2_obj{1}
	, state(ar)
	, port(lv2_event_queue::load_ptr(ar, port))
	, source(ar)
	, data1(ar)
	, data2(ar)
	, expire(ar)
	, period(ar)
{
}

void lv2_timer::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(lv2_sync);
	ar(state), lv2_event_queue::save_ptr(ar, port.get()), ar(source, data1, data2, expire, period);
}

u64 lv2_timer::check()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		const u32 _state = +state;

		if (_state == SYS_TIMER_STATE_RUN)
		{
			const u64 _now = get_guest_system_time();
			u64 next = expire;

			// If aborting, perform the last accurate check for event
			if (_now >= next)
			{
				std::lock_guard lock(mutex);

				if (next = expire; _now < next)
				{
					// expire was updated in the middle, don't send an event
					continue;
				}

				if (port)
				{
					port->send(source, data1, data2, next);
				}

				if (period)
				{
					// Set next expiration time and check again
					const u64 _next = next + period;
					expire.release(_next > next ? _next : umax);
					continue;
				}

				// Stop after oneshot
				state.release(SYS_TIMER_STATE_STOP);
				break;
			}

			return (next - _now);
		}

		break;
	}

	return umax;
}

lv2_timer_thread::lv2_timer_thread()
{
	idm::select<lv2_obj, lv2_timer>([&](u32 id, lv2_timer&)
	{
		timers.emplace_back(idm::get_unlocked<lv2_obj, lv2_timer>(id));
	});
}

void lv2_timer_thread::operator()()
{
	u64 sleep_time = 0;

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (sleep_time != umax)
		{
			// Scale time
			sleep_time = std::min(sleep_time, u64{umax} / 100) * 100 / g_cfg.core.clocks_scale;
		}

		thread_ctrl::wait_for(sleep_time);

		sleep_time = umax;

		if (Emu.IsPaused())
		{
			sleep_time = 10000;
			continue;
		}

		reader_lock lock(mutex);

		for (const auto& timer : timers)
		{
			if (lv2_obj::check(timer))
			{
				const u64 adviced_sleep_time = timer->check();

				if (sleep_time > adviced_sleep_time)
				{
					sleep_time = adviced_sleep_time;
				}
			}
		}
	}
}

error_code sys_timer_create(ppu_thread& ppu, vm::ptr<u32> timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_create(timer_id=*0x%x)", timer_id);

	if (auto ptr = idm::make_ptr<lv2_obj, lv2_timer>())
	{
		auto& thread = g_fxo->get<named_thread<lv2_timer_thread>>();
		{
			std::lock_guard lock(thread.mutex);

			// Theoretically could have been destroyed by sys_timer_destroy by now
			if (auto it = std::find(thread.timers.begin(), thread.timers.end(), ptr); it == thread.timers.end())
			{
				thread.timers.emplace_back(std::move(ptr));
			}
		}

		*timer_id = idm::last_id();
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_timer_destroy(ppu_thread& ppu, u32 timer_id)
{
	ppu.state += cpu_flag::wait;

	sys_timer.warning("sys_timer_destroy(timer_id=0x%x)", timer_id);

	auto timer = idm::withdraw<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		if (reader_lock lock(timer.mutex); lv2_obj::check(timer.port))
		{
			return CELL_EISCONN;
		}

		timer.exists--;
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

	auto& thread = g_fxo->get<named_thread<lv2_timer_thread>>();
	std::lock_guard lock(thread.mutex);

	if (auto it = std::find(thread.timers.begin(), thread.timers.end(), timer.ptr); it != thread.timers.end())
	{
		thread.timers.erase(it);
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

	(period ? sys_timer.warning : sys_timer.trace)("_sys_timer_start(timer_id=0x%x, base_time=0x%llx, period=0x%llx)", timer_id, base_time, period);

	const u64 start_time = get_guest_system_time();

	if (period && period < 100)
	{
		// Invalid periodic timer
		return CELL_EINVAL;
	}

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer) -> CellError
	{
		std::lock_guard lock(timer.mutex);

		if (!lv2_obj::check(timer.port))
		{
			return CELL_ENOTCONN;
		}

		if (timer.state != SYS_TIMER_STATE_STOP)
		{
			return CELL_EBUSY;
		}

		if (!period && start_time >= base_time)
		{
			// Invalid oneshot
			return CELL_ETIMEDOUT;
		}

		// sys_timer_start_periodic() will use current time (TODO: is it correct?)
		const u64 expire = base_time ? base_time : start_time + period;
		timer.expire = expire > start_time ? expire : umax;
		timer.period = period;
		timer.state  = SYS_TIMER_STATE_RUN;
		return {};
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	if (timer.ret)
	{
		if (timer.ret == CELL_ETIMEDOUT)
		{
			return not_an_error(timer.ret);
		}

		return timer.ret;
	}

	g_fxo->get<named_thread<lv2_timer_thread>>()([]{});

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

		if (lv2_obj::check(timer.port))
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

		if (!lv2_obj::check(timer.port))
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

	sys_timer.warning("sys_timer_sleep(sleep_time=%d)", sleep_time);

	return sys_timer_usleep(ppu, sleep_time * u64{1000000});
}

error_code sys_timer_usleep(ppu_thread& ppu, u64 sleep_time)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_usleep(sleep_time=0x%llx)", sleep_time);

	if (sleep_time)
	{
		lv2_obj::sleep(ppu, g_cfg.core.sleep_timers_accuracy < sleep_timers_accuracy_level::_usleep ? sleep_time : 0);

		if (!lv2_obj::wait_timeout<true>(sleep_time))
		{
			ppu.state += cpu_flag::again;
		}
	}
	else
	{
		std::this_thread::yield();
	}

	return CELL_OK;
}
