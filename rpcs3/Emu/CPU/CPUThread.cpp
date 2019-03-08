#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "CPUThread.h"
#include "Emu/IdManager.h"
#include "Utilities/GDBDebugServer.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

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

void cpu_thread::operator()()
{
	state -= cpu_flag::exit;

	g_tls_current_cpu_thread = this;

	if (g_cfg.core.thread_scheduler_enabled)
	{
		thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(id_type() == 1 ? thread_class::ppu : thread_class::spu));
	}

	if (g_cfg.core.lower_spu_priority && id_type() == 2)
	{
		thread_ctrl::set_native_priority(-1);
	}

	// Check thread status
	while (!(state & (cpu_flag::exit + cpu_flag::dbg_global_stop)))
	{
		// Check stop status
		if (!(state & cpu_flag::stop))
		{
			try
			{
				cpu_task();
			}
			catch (cpu_flag _s)
			{
				state += _s;
			}
			catch (const std::exception& e)
			{
				LOG_FATAL(GENERAL, "%s thrown: %s", typeid(e).name(), e.what());
				LOG_NOTICE(GENERAL, "\n%s", dump());
				Emu.Pause();
				break;
			}

			state -= cpu_flag::ret;
			continue;
		}

		thread_ctrl::wait();
	}
}

void cpu_thread::on_abort()
{
	state += cpu_flag::exit;
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
	if (state & cpu_flag::dbg_pause)
	{
		fxm::get<GDBDebugServer>()->pause_from(this);
	}
#endif

	bool cpu_sleep_called = false;
	bool cpu_flag_memory = false;

	while (true)
	{
		if (state & cpu_flag::memory)
		{
			if (auto& ptr = vm::g_tls_locked)
			{
				ptr->compare_and_swap(this, nullptr);
				ptr = nullptr;
			}

			cpu_flag_memory = true;
			state -= cpu_flag::memory;
		}

		if (state & cpu_flag::exit + cpu_flag::dbg_global_stop)
		{
			return true;
		}

		if (state & cpu_flag::signal && state.test_and_reset(cpu_flag::signal))
		{
			cpu_sleep_called = false;
		}

		if (!is_paused())
		{
			if (cpu_flag_memory)
			{
				cpu_mem();
			}

			break;
		}
		else if (!cpu_sleep_called && state & cpu_flag::suspend)
		{
			cpu_sleep();
			cpu_sleep_called = true;
			continue;
		}

		thread_ctrl::wait();
	}

	const auto state_ = state.load();

	if (state_ & (cpu_flag::ret + cpu_flag::stop))
	{
		return true;
	}

	if (state_ & cpu_flag::dbg_step)
	{
		state += cpu_flag::dbg_pause;
		state -= cpu_flag::dbg_step;
	}

	return false;
}

void cpu_thread::notify()
{
	if (id_type() == 1)
	{
		thread_ctrl::notify(*static_cast<named_thread<ppu_thread>*>(this));
	}
	else if (id_type() == 2)
	{
		thread_ctrl::notify(*static_cast<named_thread<spu_thread>*>(this));
	}
	else
	{
		fmt::throw_exception("Invalid cpu_thread type");
	}
}

std::string cpu_thread::dump() const
{
	return fmt::format("Type: %s\n" "State: %s\n", typeid(*this).name(), state.load());
}
