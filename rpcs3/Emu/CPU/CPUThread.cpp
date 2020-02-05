#include "stdafx.h"
#include "CPUThread.h"

#include "Emu/System.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/IdManager.h"
#include "Emu/GDB.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include <thread>
#include <unordered_map>
#include <map>

DECLARE(cpu_thread::g_threads_created){0};
DECLARE(cpu_thread::g_threads_deleted){0};

LOG_CHANNEL(profiler);
LOG_CHANNEL(sys_log, "SYS");

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

// CPU profiler thread
struct cpu_prof
{
	// PPU/SPU id enqueued for registration
	lf_queue<u32> registered;

	struct sample_info
	{
		// Weak pointer to the thread
		std::weak_ptr<cpu_thread> wptr;

		// Block occurences: name -> sample_count
		std::unordered_map<u64, u64, value_hash<u64>> freq;

		// Total number of samples
		u64 samples = 0, idle = 0;

		sample_info(const std::shared_ptr<cpu_thread>& ptr)
			: wptr(ptr)
		{
		}

		void reset()
		{
			freq.clear();
			samples = 0;
			idle = 0;
		}

		// Print info
		void print(u32 id) const
		{
			// Make reversed map: sample_count -> name
			std::multimap<u64, u64> chart;

			for (auto& [name, count] : freq)
			{
				// Inverse bits to sort in descending order
				chart.emplace(~count, name);
			}

			// Print results
			std::string results;
			results.reserve(5100);

			// Fraction of non-idle samples
			const f64 busy = 1. * (samples - idle) / samples;

			for (auto& [rcount, name] : chart)
			{
				// Get correct count value
				const u64 count = ~rcount;
				const f64 _frac = count / busy / samples;

				// Print only 7 hash characters out of 11 (which covers roughly 48 bits)
				fmt::append(results, "\n\t[%s", fmt::base57(be_t<u64>{name}));
				results.resize(results.size() - 4);

				// Print chunk address from lowest 16 bits
				fmt::append(results, "...chunk-0x%05x]: %.4f%% (%u)", (name & 0xffff) * 4, _frac * 100., count);

				if (results.size() >= 5000)
				{
					// Stop printing after reaching some arbitrary limit in characters
					break;
				}
			}

			profiler.notice("Thread [0x%08x]: %u samples (%.4f%% idle):%s", id, samples, 100. * idle / samples, results);
		}
	};

	void operator()()
	{
		std::unordered_map<u32, sample_info, value_hash<u64>> threads;

		while (thread_ctrl::state() != thread_state::aborting)
		{
			bool flush = false;

			// Handle registration channel
			for (u32 id : registered.pop_all())
			{
				if (id == 0)
				{
					// Handle id zero as a command to flush results
					flush = true;
					continue;
				}

				std::shared_ptr<cpu_thread> ptr;

				if (id >> 24 == 1)
				{
					ptr = idm::get<named_thread<ppu_thread>>(id);
				}
				else if (id >> 24 == 2)
				{
					ptr = idm::get<named_thread<spu_thread>>(id);
				}
				else
				{
					profiler.error("Invalid Thread ID: 0x%08x", id);
					continue;
				}

				if (ptr)
				{
					auto [found, add] = threads.try_emplace(id, ptr);

					if (!add)
					{
						// Overwritten: print previous data
						found->second.print(id);
						found->second.reset();
						found->second.wptr = ptr;
					}
				}
			}

			if (threads.empty())
			{
				// Wait for messages if no work (don't waste CPU)
				registered.wait();
				continue;
			}

			// Sample active threads
			for (auto& [id, info] : threads)
			{
				if (auto ptr = info.wptr.lock())
				{
					// Get short function hash
					const u64 name = atomic_storage<u64>::load(ptr->block_hash);

					// Append occurrence
					info.samples++;

					if (!(ptr->state.load() & (cpu_flag::wait + cpu_flag::stop + cpu_flag::dbg_global_pause)))
					{
						info.freq[name]++;

						// Append verification time to fixed common name 0000000...chunk-0x3fffc
						if ((name & 0xffff) == 0)
							info.freq[0xffff]++;
					}
					else
					{
						info.idle++;
					}
				}
			}

			// Cleanup and print results for deleted threads
			for (auto it = threads.begin(), end = threads.end(); it != end;)
			{
				if (it->second.wptr.expired())
					it->second.print(it->first), it = threads.erase(it);
				else
					it++;
			}

			if (flush)
			{
				profiler.success("Flushing profiling results...");

				// Print all results and cleanup
				for (auto& [id, info] : threads)
				{
					info.print(id);
					info.reset();
				}
			}

			// Wait, roughly for 20µs
			thread_ctrl::wait_for(20, false);
		}

		// Print all remaining results
		for (auto& [id, info] : threads)
		{
			info.print(id);
		}
	}

	static constexpr auto thread_name = "CPU Profiler"sv;
};

using cpu_profiler = named_thread<cpu_prof>;

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

	if (id_type() == 2)
	{
		// force input/output denormals to zero for SPU threads (FTZ/DAZ)
		_mm_setcsr( _mm_getcsr() | 0x8040 );

		const volatile int a = 0x1fc00000;
		__m128 b = _mm_castsi128_ps(_mm_set1_epi32(a));
		int c = _mm_cvtsi128_si32(_mm_castps_si128(_mm_mul_ps(b,b)));

		if (c != 0)
		{
			sys_log.fatal("Could not disable denormals.");
		}
	}

	if (id_type() == 1 && false)
	{
		g_fxo->get<cpu_profiler>()->registered.push(id);
	}

	if (id_type() == 2 && g_cfg.core.spu_prof)
	{
		g_fxo->get<cpu_profiler>()->registered.push(id);
	}

	// Register thread in g_cpu_array
	if (!g_cpu_array_sema.try_inc(sizeof(g_cpu_array_bits) * 8))
	{
		sys_log.fatal("Too many threads.");
		Emu.Pause();
		return;
	}

	u64 array_slot = -1;

	for (u32 i = 0;; i = (i + 1) % ::size32(g_cpu_array_bits))
	{
		const auto [bits, ok] = g_cpu_array_bits[i].fetch_op([](u64& bits) -> u64
		{
			if (~bits) [[likely]]
			{
				// Set lowest clear bit
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (ok) [[likely]]
		{
			// Get actual slot number
			array_slot = i * 64 + utils::cnttz64(~bits, false);
			break;
		}
	}

	// Register and wait if necessary
	verify("g_cpu_array[...] -> this" HERE), g_cpu_array[array_slot].exchange(this) == nullptr;

	state += cpu_flag::wait;
	g_cpu_suspend_lock.lock_unlock();

	// Check thread status
	while (!(state & (cpu_flag::exit + cpu_flag::dbg_global_stop)) && thread_ctrl::state() != thread_state::aborting)
	{
		// Check stop status
		if (!(state & cpu_flag::stop))
		{
			try
			{
				cpu_task();
			}
			catch (const std::exception& e)
			{
				Emu.Pause();
				sys_log.fatal("%s thrown: %s", typeid(e).name(), e.what());
				sys_log.notice("\n%s", dump());
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
	if (state & cpu_flag::dbg_pause)
	{
		g_fxo->get<gdb_server>()->pause_from(this);
	}

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

		if (state & (cpu_flag::exit + cpu_flag::dbg_global_stop))
		{
			state += cpu_flag::wait;
			return true;
		}

		const auto [state0, escape] = state.fetch_op([&](bs_t<cpu_flag>& flags)
		{
			// Atomically clean wait flag and escape
			if (!(flags & (cpu_flag::exit + cpu_flag::dbg_global_stop + cpu_flag::ret + cpu_flag::stop)))
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
	// Downcast to correct type
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
	// Downcast to correct type
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

		if (ok) [[likely]]
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
		sys_log.fatal("cpu_thread::stop_all() has been called from a CPU thread.");
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

	sys_log.notice("All CPU threads have been signaled.");

	while (g_cpu_array_sema)
	{
		std::this_thread::sleep_for(10ms);
	}

	// Workaround for remaining threads (TODO)
	std::this_thread::sleep_for(1300ms);

	sys_log.notice("All CPU threads have been stopped.");
}

void cpu_thread::flush_profilers() noexcept
{
	if (!g_fxo->get<cpu_profiler>())
	{
		profiler.fatal("cpu_thread::flush_profilers() has been called incorrectly." HERE);
		return;
	}

	if (g_cfg.core.spu_prof || false)
	{
		g_fxo->get<cpu_profiler>()->registered.push(0);
	}
}
