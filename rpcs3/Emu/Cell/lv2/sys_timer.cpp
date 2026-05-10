#include "stdafx.h"
#include "sys_timer.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/timers.hpp"

#include "util/asm.hpp"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "sys_event.h"
#include "sys_process.h"

#include <thread>
#include <deque>

LOG_CHANNEL(sys_timer);

struct lv2_timer_thread
{
	shared_mutex mutex;
	std::deque<shared_ptr<lv2_timer>> timers;

	lv2_timer_thread();
	void operator()();

	//SAVESTATE_INIT_POS(46); // FREE SAVESTATE_INIT_POS number

	static constexpr auto thread_name = "Timer Thread"sv;
};

lv2_timer::lv2_timer(utils::serial& ar)
	: lv2_obj(1)
	, state(ar)
	, port(lv2_event_queue::load_ptr(ar, port, "timer"))
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

u64 lv2_timer::check(u64 _now) noexcept
{
	while (true)
	{
		const u32 _state = +state;

		if (_state == SYS_TIMER_STATE_RUN)
		{
			u64 next = expire;

			// If aborting, perform the last accurate check for event
			if (_now >= next)
			{
				lv2_obj::notify_all_t notify;

				std::lock_guard lock(mutex);
				return check_unlocked(_now);
			}

			return (next - _now);
		}

		break;
	}

	return umax;
}

u64 lv2_timer::check_unlocked(u64 _now) noexcept
{
	const u64 next = expire;

	if (_now < next || state != SYS_TIMER_STATE_RUN)
	{
		return umax;
	}

	if (port)
	{
		port->send(source, data1, data2, next);
	}

	if (period)
	{
		// Set next expiration time and check again
		const u64 expire0 = utils::add_saturate<u64>(next, period);
		expire.release(expire0);
		return utils::sub_saturate<u64>(expire0, _now);
	}

	// Stop after oneshot
	state.release(SYS_TIMER_STATE_STOP);
	return umax;
}

lv2_timer_thread::lv2_timer_thread()
{
	Emu.PostponeInitCode([this]()
	{
		idm::select<lv2_obj, lv2_timer>([&](u32 id, lv2_timer&)
		{
			timers.emplace_back(idm::get_unlocked<lv2_obj, lv2_timer>(id));
		});
	});
}

void lv2_timer_thread::operator()()
{
	u64 sleep_time = 0;

	while (true)
	{
		if (sleep_time != umax)
		{
			// Scale time
			sleep_time = std::min(sleep_time, u64{umax} / 100) * 100 / g_cfg.core.clocks_scale;
		}

		thread_ctrl::wait_for(sleep_time);

		if (thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		sleep_time = umax;

		if (Emu.IsPausedOrReady())
		{
			sleep_time = 10000;
			continue;
		}

		const u64 _now = get_guest_system_time();

		reader_lock lock(mutex);

		for (const auto& timer : timers)
		{
			while (lv2_obj::check(timer))
			{
				if (thread_ctrl::state() == thread_state::aborting)
				{
					break;
				}

				if (const u64 advised_sleep_time = timer->check(_now))
				{
					if (sleep_time > advised_sleep_time)
					{
						sleep_time = advised_sleep_time;
					}

					break;
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

		ppu.check_state();
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
	const u64 now = get_guest_system_time();

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [&](lv2_timer& timer)
	{
		std::lock_guard lock(timer.mutex);

		timer.check_unlocked(now);
		timer.get_information(_info);
	});

	if (!timer)
	{
		return CELL_ESRCH;
	}

	ppu.check_state();
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

		// LV2 Disassembly: Simple nullptr check (assignment test, do not use lv2_obj::check here)
		if (!timer.port)
		{
			return CELL_ENOTCONN;
		}

		timer.check_unlocked(start_time);
		if (timer.state != SYS_TIMER_STATE_STOP)
		{
			return CELL_EBUSY;
		}

		if (!period && start_time >= base_time)
		{
			// Invalid oneshot
			return CELL_ETIMEDOUT;
		}

		const u64 expire = period == 0 ? base_time : // oneshot
			base_time == 0 ? utils::add_saturate(start_time, period) : // periodic timer with no base (using start time as base)
			start_time < utils::add_saturate(base_time, period) ? utils::add_saturate(base_time, period) : // periodic with base time over start time
			[&]() -> u64 // periodic timer base before start time (align to be at least a period over start time)
			{
				// Optimized from a loop in LV2:
				// do
				// {
				// 	  base_time += period;
				// }
				// while (base_time < start_time);

				const u64 start_time_with_base_time_reminder = utils::add_saturate(start_time - start_time % period, base_time % period);

				return utils::add_saturate(start_time_with_base_time_reminder, start_time_with_base_time_reminder < start_time ? period : 0);
			}();

		timer.expire = expire;
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

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [now = get_guest_system_time(), notify = lv2_obj::notify_all_t()](lv2_timer& timer)
	{
		std::lock_guard lock(timer.mutex);
		timer.check_unlocked(now);
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
		auto found = idm::get_unlocked<lv2_obj, lv2_event_queue>(queue_id);

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
		timer.port   = found;
		timer.source = name ? name : (u64{process_getpid() + 0u} << 32) | u64{timer_id};
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

	const auto timer = idm::check<lv2_obj, lv2_timer>(timer_id, [now = get_guest_system_time(), notify = lv2_obj::notify_all_t()](lv2_timer& timer) -> CellError
	{
		std::lock_guard lock(timer.mutex);

		timer.check_unlocked(now);
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

	sys_timer.trace("sys_timer_sleep(sleep_time=%d)", sleep_time);

	return sys_timer_usleep(ppu, sleep_time * u64{1000000});
}

error_code sys_timer_usleep(ppu_thread& ppu, u64 sleep_time)
{
	ppu.state += cpu_flag::wait;

	sys_timer.trace("sys_timer_usleep(sleep_time=0x%llx)", sleep_time);

	if (sleep_time)
	{
		const s64 add_time = g_cfg.core.usleep_addend;

		// Over/underflow checks
		if (add_time >= 0)
		{
			sleep_time = utils::add_saturate<u64>(sleep_time, add_time);
		}
		else
		{
			sleep_time = std::max<u64>(1, utils::sub_saturate<u64>(sleep_time, -add_time));
		}

		lv2_obj::sleep(ppu, g_cfg.core.sleep_timers_accuracy < sleep_timers_accuracy_level::_usleep ? sleep_time : 0);

		if (!lv2_obj::wait_timeout(sleep_time, &ppu, true, true))
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
