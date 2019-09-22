#include "stdafx.h"
#include "CPUThread.h"

#include "Emu/System.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/IdManager.h"
#include "Utilities/GDBDebugServer.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include <thread>

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
		case cpu_flag::wait: return "w";
		case cpu_flag::pause: return "p";
		case cpu_flag::suspend: return "s";
		case cpu_flag::ret: return "ret";
		case cpu_flag::signal: return "sig";
		case cpu_flag::memory: return "mem";
		case cpu_flag::jit_return: return "JIT";
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

// For synchronizing suspend_all operation
alignas(64) shared_mutex g_cpu_suspend_lock;

// Semaphore for global thread array (global counter)
alignas(64) atomic_t<u32> g_cpu_array_sema{0};

// Semaphore subdivision for each array slot (64 x N in total)
atomic_t<u64> g_cpu_array_bits[6]{};

// All registered threads
atomic_t<cpu_thread*> g_cpu_array[sizeof(g_cpu_array_bits) * 8]{};

template <typename F>
void for_all_cpu(F&& func) noexcept
{
	for (u32 i = 0; i < ::size32(g_cpu_array_bits); i++)
	{
		for (u64 bits = g_cpu_array_bits[i]; bits; bits &= bits - 1)
		{
			const u64 index = i * 64 + utils::cnttz64(bits, true);

			if (cpu_thread* cpu = g_cpu_array[index].load())
			{
				func(cpu);
			}
		}
	}
}

void cpu_thread::operator()()
{
	g_tls_current_cpu_thread = this;

	if (g_cfg.core.thread_scheduler_enabled)
	{
		thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(id_type() == 1 ? thread_class::ppu : thread_class::spu));
	}

	if (g_cfg.core.lower_spu_priority && id_type() == 2)
	{
		thread_ctrl::set_native_priority(-1);
	}

	// Register thread in g_cpu_array
	if (!g_cpu_array_sema.try_inc(sizeof(g_cpu_array_bits) * 8))
	{
		LOG_FATAL(GENERAL, "Too many threads");
		Emu.Pause();
		return;
	}

	u64 array_slot = -1;

	for (u32 i = 0;; i = (i + 1) % ::size32(g_cpu_array_bits))
	{
		if (LIKELY(~g_cpu_array_bits[i]))
		{
			const u64 found = g_cpu_array_bits[i].atomic_op([](u64& bits) -> u64
			{
				// Find empty array slot and set its bit
				if (LIKELY(~bits))
				{
					const u64 bit = utils::cnttz64(~bits, true);
					bits |= 1ull << bit;
					return bit;
				}

				return 64;
			});

			if (LIKELY(found < 64))
			{
				// Fixup
				array_slot = i * 64 + found;
				break;
			}
		}
	}

	// Register and wait if necessary
	verify("g_cpu_array[...] -> this" HERE), g_cpu_array[array_slot].exchange(this) == nullptr;

	state += cpu_flag::wait;
	g_cpu_suspend_lock.lock_unlock();

	// Check thread status
	while (!(state & (cpu_flag::exit + cpu_flag::dbg_global_stop)) && !Emu.IsStopped())
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
				Emu.Pause();
				LOG_FATAL(GENERAL, "%s thrown: %s", typeid(e).name(), e.what());
				LOG_NOTICE(GENERAL, "\n%s", dump());
				break;
			}

			state -= cpu_flag::ret;
			continue;
		}

		thread_ctrl::wait();
	}

	if (auto ptr = vm::g_tls_locked)
	{
		ptr->compare_and_swap(this, nullptr);
	}

	// Unregister and wait if necessary
	state += cpu_flag::wait;
	verify("g_cpu_array[...] -> null" HERE), g_cpu_array[array_slot].exchange(nullptr) == this;
	g_cpu_array_bits[array_slot / 64] &= ~(1ull << (array_slot % 64));
	g_cpu_array_sema--;
	g_cpu_suspend_lock.lock_unlock();
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

bool cpu_thread::check_state() noexcept
{
#ifdef WITH_GDB_DEBUGGER
	if (state & cpu_flag::dbg_pause)
	{
		fxm::get<GDBDebugServer>()->pause_from(this);
	}
#endif

	bool cpu_sleep_called = false;
	bool cpu_flag_memory = false;

	if (!(state & cpu_flag::wait))
	{
		state += cpu_flag::wait;
	}

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

		if (state & (cpu_flag::exit + cpu_flag::jit_return + cpu_flag::dbg_global_stop))
		{
			state += cpu_flag::wait;
			return true;
		}

		const auto [state0, escape] = state.fetch_op([&](bs_t<cpu_flag>& flags)
		{
			// Atomically clean wait flag and escape
			if (!(flags & (cpu_flag::exit + cpu_flag::jit_return + cpu_flag::dbg_global_stop + cpu_flag::ret + cpu_flag::stop)))
			{
				// Check pause flags which hold thread inside check_state
				if (flags & (cpu_flag::pause + cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause))
				{
					return false;
				}

				flags -= cpu_flag::wait;
			}

			return true;
		});

		if (state & cpu_flag::signal && state.test_and_reset(cpu_flag::signal))
		{
			cpu_sleep_called = false;
		}

		if (escape)
		{
			if (cpu_flag_memory)
			{
				cpu_mem();
			}

			break;
		}
		else if (!cpu_sleep_called && state0 & cpu_flag::suspend)
		{
			cpu_sleep();
			cpu_sleep_called = true;
			continue;
		}

		if (state0 & (cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause))
		{
			thread_ctrl::wait();
		}
		else
		{
			// If only cpu_flag::pause was set, notification won't arrive
			g_cpu_suspend_lock.lock_unlock();
		}
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

void cpu_thread::abort()
{
	if (id_type() == 1)
	{
		*static_cast<named_thread<ppu_thread>*>(this) = thread_state::aborting;
	}
	else if (id_type() == 2)
	{
		*static_cast<named_thread<spu_thread>*>(this) = thread_state::aborting;
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

cpu_thread::suspend_all::suspend_all(cpu_thread* _this) noexcept
	: m_this(_this)
{
	if (m_this)
	{
		m_this->state += cpu_flag::wait;
	}

	g_cpu_suspend_lock.lock_vip();

	for_all_cpu([](cpu_thread* cpu)
	{
		cpu->state += cpu_flag::pause;
	});

	busy_wait(500);

	while (true)
	{
		bool ok = true;

		for_all_cpu([&](cpu_thread* cpu)
		{
			if (!(cpu->state & cpu_flag::wait))
			{
				ok = false;
			}
		});

		if (LIKELY(ok))
		{
			break;
		}

		busy_wait(500);
	}
}

cpu_thread::suspend_all::~suspend_all()
{
	// Make sure the latest thread does the cleanup and notifies others
	if (g_cpu_suspend_lock.downgrade_unique_vip_lock_to_low_or_unlock())
	{
		for_all_cpu([&](cpu_thread* cpu)
		{
			cpu->state -= cpu_flag::pause;
		});

		g_cpu_suspend_lock.unlock_low();
	}
	else
	{
		g_cpu_suspend_lock.lock_unlock();
	}

	if (m_this)
	{
		m_this->check_state();
	}
}

void cpu_thread::stop_all() noexcept
{
	if (g_tls_current_cpu_thread)
	{
		// Report unsupported but unnecessary case
		LOG_FATAL(GENERAL, "cpu_thread::stop_all() has been called from a CPU thread.");
		return;
	}
	else
	{
		::vip_lock lock(g_cpu_suspend_lock);

		for_all_cpu([](cpu_thread* cpu)
		{
			cpu->state += cpu_flag::dbg_global_stop;
			cpu->abort();
		});
	}

	LOG_NOTICE(GENERAL, "All CPU threads have been signaled.");

	while (g_cpu_array_sema)
	{
		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All CPU threads have been stopped.");
}
