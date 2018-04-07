#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "CPUThread.h"
#include "Emu/IdManager.h"
#include "Utilities/GDBDebugServer.h"
#include <typeinfo>

#ifdef _WIN32
#include <Windows.h>
#endif

DECLARE(cpu_thread::g_threads_created){0};
DECLARE(cpu_thread::g_threads_deleted){0};

template <>
void fmt_class_string<cpu_flag>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cpu_flag f)
	{
		switch (f)
		{
		case cpu_flag::stop: return "STOP";
		case cpu_flag::exit: return "EXIT";
		case cpu_flag::suspend: return "s";
		case cpu_flag::ret: return "ret";
		case cpu_flag::signal: return "sig";
		case cpu_flag::memory: return "mem";
		case cpu_flag::dbg_global_pause: return "G-PAUSE";
		case cpu_flag::dbg_global_stop: return "G-EXIT";
		case cpu_flag::dbg_pause: return "PAUSE";
		case cpu_flag::dbg_step: return "STEP";
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

	// Check thread status
	while (!test(state, cpu_flag::exit + cpu_flag::dbg_global_stop))
	{
		// Check stop status
		if (!test(state & cpu_flag::stop))
		{
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

		thread_ctrl::wait();
	}
}

void cpu_thread::on_stop()
{
	state += cpu_flag::exit;
	notify();
}

cpu_thread::~cpu_thread()
{
	vm::cleanup_unlock(*this);
	g_threads_deleted++;
}

cpu_thread::cpu_thread(u32 id)
	: id(id)
{
	g_threads_created++;
}

bool cpu_thread::check_state()
{
#ifdef WITH_GDB_DEBUGGER
	if (test(state, cpu_flag::dbg_pause)) {
		fxm::get<GDBDebugServer>()->pause_from(this);
	}
#endif

	bool cpu_sleep_called = false;
	bool cpu_flag_memory = false;

	while (true)
	{
		if (test(state, cpu_flag::memory) && state.test_and_reset(cpu_flag::memory))
		{
			cpu_flag_memory = true;

			if (auto& ptr = vm::g_tls_locked)
			{
				ptr->compare_and_swap(this, nullptr);
				ptr = nullptr;
			}
		}

		if (test(state, cpu_flag::exit + cpu_flag::dbg_global_stop))
		{
			return true;
		}

		if (test(state & cpu_flag::signal) && state.test_and_reset(cpu_flag::signal))
		{
			cpu_sleep_called = false;
		}

		if (!test(state, cpu_state_pause))
		{
			if (cpu_flag_memory)
			{
				cpu_mem();
			}

			break;
		}
		else if (!cpu_sleep_called && test(state, cpu_flag::suspend))
		{
			cpu_sleep();
			cpu_sleep_called = true;
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

void cpu_thread::test_state()
{
	if (UNLIKELY(test(state)))
	{
		if (check_state())
		{
			throw cpu_flag::ret;
		}
	}
}

void cpu_thread::run()
{
	state -= cpu_flag::stop;
	notify();
}

std::string cpu_thread::dump() const
{
	return fmt::format("Type: %s\n" "State: %s\n", typeid(*this).name(), state.load());
}
