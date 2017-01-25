#include "stdafx.h"
#include "Emu/System.h"
#include "CPUThread.h"

#include <mutex>

template<>
void fmt_class_string<cpu_flag>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cpu_flag f)
	{
		switch (f)
		{
		STR_CASE(cpu_flag::stop);
		STR_CASE(cpu_flag::exit);
		STR_CASE(cpu_flag::suspend);
		STR_CASE(cpu_flag::ret);
		STR_CASE(cpu_flag::signal);
		STR_CASE(cpu_flag::dbg_global_pause);
		STR_CASE(cpu_flag::dbg_global_stop);
		STR_CASE(cpu_flag::dbg_pause);
		STR_CASE(cpu_flag::dbg_step);
		case cpu_flag::__bitset_enum_max: break;
		}

		return unknown;
	});
}

template<>
void fmt_class_string<bs_t<cpu_flag>>::format(std::string& out, u64 arg)
{
	format_bitset(out, arg, "[", "|", "]", &fmt_class_string<cpu_flag>::format);
}

thread_local cpu_thread* g_tls_current_cpu_thread = nullptr;

void cpu_thread::on_task()
{
	state -= cpu_flag::exit;

	g_tls_current_cpu_thread = this;

	Emu.SendDbgCommand(DID_CREATE_THREAD, this);

	std::unique_lock<named_thread> lock(*this);

	// Check thread status
	while (!test(state & cpu_flag::exit))
	{
		CHECK_EMU_STATUS;

		// check stop status
		if (!test(state & cpu_flag::stop))
		{
			if (lock) lock.unlock();

			try
			{
				cpu_task();
			}
			catch (cpu_flag _s)
			{
				state += _s;
			}
			catch (const std::exception&)
			{
				LOG_NOTICE(GENERAL, "\n%s", dump());
				throw;
			}

			state -= cpu_flag::ret;
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
	state += cpu_flag::exit;
	lock_notify();
}

cpu_thread::~cpu_thread()
{
}

cpu_thread::cpu_thread(u32 id)
	: id(id)
{
}

bool cpu_thread::check_state()
{
	std::unique_lock<named_thread> lock(*this, std::defer_lock);

	while (true)
	{
		CHECK_EMU_STATUS; // check at least once

		if (test(state & cpu_flag::exit))
		{
			return true;
		}

		if (!test(state & cpu_state_pause))
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

	if (test(state_, cpu_flag::ret + cpu_flag::stop))
	{
		return true;
	}

	if (test(state_, cpu_flag::dbg_step))
	{
		state += cpu_flag::dbg_pause;
		state -= cpu_flag::dbg_step;
	}

	return false;
}

void cpu_thread::run()
{
	state -= cpu_flag::stop;
	lock_notify();
}

void cpu_thread::set_signal()
{
	verify("cpu_flag::signal" HERE), !state.test_and_set(cpu_flag::signal);
	notify();
}

std::string cpu_thread::dump() const
{
	return fmt::format("Type: %s\n" "State: %s\n", typeid(*this).name(), state.load());
}
