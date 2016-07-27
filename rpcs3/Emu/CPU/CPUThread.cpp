#include "stdafx.h"
#include "Emu/System.h"
#include "CPUThread.h"

#include <mutex>

thread_local cpu_thread* g_tls_current_cpu_thread = nullptr;

void cpu_thread::on_task()
{
	state -= cpu_state::exit;

	g_tls_current_cpu_thread = this;

	Emu.SendDbgCommand(DID_CREATE_THREAD, this);

	std::unique_lock<named_thread> lock(*this);

	// Check thread status
	while (!(state & cpu_state::exit))
	{
		CHECK_EMU_STATUS;

		// check stop status
		if (!(state & cpu_state::stop))
		{
			if (lock) lock.unlock();

			try
			{
				cpu_task();
			}
			catch (cpu_state _s)
			{
				state += _s;
			}
			catch (const std::exception&)
			{
				LOG_NOTICE(GENERAL, "\n%s", dump());
				throw;
			}

			state -= cpu_state::ret;
			continue;
		}

		if (!lock)
		{
			lock.lock();
			continue;
		}

		thread_ctrl::wait();
	}
}

void cpu_thread::on_stop()
{
	state += cpu_state::exit;
	lock_notify();
}

cpu_thread::~cpu_thread()
{
}

cpu_thread::cpu_thread(cpu_type type)
	: type(type)
{
}

bool cpu_thread::check_state()
{
	std::unique_lock<named_thread> lock(*this, std::defer_lock);

	while (true)
	{
		CHECK_EMU_STATUS; // check at least once

		if (state & cpu_state::exit)
		{
			return true;
		}

		if (!state.test(cpu_state_pause))
		{
			break;
		}

		if (!lock)
		{
			lock.lock();
			continue;
		}

		thread_ctrl::wait();
	}

	const auto state_ = state.load();

	if (state_ & make_bitset(cpu_state::ret, cpu_state::stop))
	{
		return true;
	}

	if (state_ & cpu_state::dbg_step)
	{
		state += cpu_state::dbg_pause;
		state -= cpu_state::dbg_step;
	}

	return false;
}

void cpu_thread::run()
{
	state -= cpu_state::stop;
	lock_notify();
}
