#include "stdafx.h"
#include "CPUThread.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/IdManager.h"
#include "Emu/GDB.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/perf_meter.hpp"

#include <thread>
#include <unordered_map>
#include <numeric>
#include <map>

DECLARE(cpu_thread::g_threads_created){0};
DECLARE(cpu_thread::g_threads_deleted){0};
DECLARE(cpu_thread::g_suspend_counter){0};

LOG_CHANNEL(profiler);
LOG_CHANNEL(sys_log, "SYS");

static thread_local u64 s_tls_thread_slot = -1;

extern thread_local void(*g_tls_log_control)(const char* fmt, u64 progress);

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
		case cpu_flag::temp: return "t";
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
			std::multimap<u64, u64, std::greater<u64>> chart;

			for (auto& [name, count] : freq)
			{
				chart.emplace(count, name);
			}

			// Print results
			std::string results;
			results.reserve(5100);

			// Fraction of non-idle samples
			const f64 busy = 1. * (samples - idle) / samples;

			for (auto& [count, name] : chart)
			{
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

struct cpu_counter
{
	// For synchronizing suspend_all operation
	alignas(64) shared_mutex cpu_suspend_lock;

	// Workload linked list
	alignas(64) atomic_t<cpu_thread::suspend_work*> cpu_suspend_work{};

	// Semaphore for global thread array (global counter)
	alignas(64) atomic_t<u32> cpu_array_sema{0};

	// Semaphore subdivision for each array slot (64 x N in total)
	alignas(64) atomic_t<u64> cpu_array_bits[3]{};

	// Copy of array bits for internal use
	alignas(64) u64 cpu_copy_bits[3]{};

	// All registered threads
	atomic_t<cpu_thread*> cpu_array[sizeof(cpu_array_bits) * 8]{};

	u64 add(cpu_thread* _this, bool restore = false) noexcept
	{
		u64 array_slot = -1;

		if (!restore && !cpu_array_sema.try_inc(sizeof(cpu_counter::cpu_array_bits) * 8))
		{
			sys_log.fatal("Too many threads.");
			return array_slot;
		}

		for (u32 i = 0;; i = (i + 1) % ::size32(cpu_array_bits))
		{
			const auto [bits, ok] = cpu_array_bits[i].fetch_op([](u64& bits) -> u64
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
				array_slot = i * 64 + std::countr_one(bits);

				// Register thread
				if (cpu_array[array_slot].compare_and_swap_test(nullptr, _this)) [[likely]]
				{
					break;
				}

				sys_log.fatal("Unexpected slot registration failure (%u).", array_slot);
				cpu_array_bits[array_slot / 64] &= ~(1ull << (array_slot % 64));
				continue;
			}
		}

		if (!restore)
		{
			// First time (thread created)
			_this->state += cpu_flag::wait;
			cpu_suspend_lock.lock_unlock();
		}

		return array_slot;
	}

	void remove(cpu_thread* _this, u64 slot) noexcept
	{
		// Unregister and wait if necessary
		_this->state += cpu_flag::wait;

		std::lock_guard lock(cpu_suspend_lock);

		if (!cpu_array[slot].compare_and_swap_test(_this, nullptr))
		{
			sys_log.fatal("Inconsistency for array slot %u", slot);
			return;
		}

		cpu_array_bits[slot / 64] &= ~(1ull << (slot % 64));
		cpu_array_sema--;
	}

	// Remove temporarily
	void remove(cpu_thread* _this) noexcept
	{
		// Unregister temporarily (called from check_state)
		const u64 index = s_tls_thread_slot;

		if (index >= std::size(cpu_array))
		{
			sys_log.fatal("Index out of bounds (%u).", index);
			return;
		}

		if (cpu_array[index].load() == _this && cpu_array[index].compare_and_swap_test(_this, nullptr))
		{
			cpu_array_bits[index / 64] &= ~(1ull << (index % 64));
			return;
		}

		sys_log.fatal("Thread not found in cpu_array (%s).", _this->get_name());
	}
};

template <bool UseCopy = false, typename F>
void for_all_cpu(F func) noexcept
{
	const auto ctr = g_fxo->get<cpu_counter>();

	for (u32 i = 0; i < ::size32(ctr->cpu_array_bits); i++)
	{
		for (u64 bits = (UseCopy ? ctr->cpu_copy_bits[i] : ctr->cpu_array_bits[i].load()); bits; bits &= bits - 1)
		{
			const u64 index = i * 64 + std::countr_zero(bits);

			if (cpu_thread* cpu = ctr->cpu_array[index].load())
			{
				if constexpr (std::is_invocable_v<F, cpu_thread*, u64>)
				{
					func(cpu, index);
					continue;
				}

				if constexpr (std::is_invocable_v<F, cpu_thread*>)
				{
					func(cpu);
					continue;
				}
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
	if (id_type() == 2)
	{
		if (g_cfg.core.lower_spu_priority)
		{
			thread_ctrl::set_native_priority(-1);
		}

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

	while (!g_fxo->get<cpu_counter>() && !g_fxo->get<cpu_profiler>())
	{
		// Can we have a little race, right? First thread is started concurrently with g_fxo->init()
		std::this_thread::sleep_for(1ms);
	}

	switch (id_type())
	{
	case 1:
	{
		//g_fxo->get<cpu_profiler>()->registered.push(id);
		break;
	}
	case 2:
	{
		if (g_cfg.core.spu_prof)
		{
			g_fxo->get<cpu_profiler>()->registered.push(id);
		}

		break;
	}
	default: break;
	}

	// Register thread in g_cpu_array
	s_tls_thread_slot = g_fxo->get<cpu_counter>()->add(this);

	if (s_tls_thread_slot == umax)
	{
		return;
	}

	atomic_storage_futex::set_notify_callback([](const void*, u64 progress)
	{
		static thread_local bool wait_set = false;

		cpu_thread* _cpu = get_current_cpu_thread();

		// Wait flag isn't set asynchronously so this should be thread-safe
		if (progress == 0 && !(_cpu->state & cpu_flag::wait))
		{
			// Operation just started and syscall is imminent
			_cpu->state += cpu_flag::wait + cpu_flag::temp;
			wait_set = true;
			return;
		}

		if (progress == umax && std::exchange(wait_set, false))
		{
			// Operation finished: need to clean wait flag
			verify(HERE), !_cpu->check_state();
			return;
		}
	});

	g_tls_log_control = [](const char* fmt, u64 progress)
	{
		static thread_local bool wait_set = false;

		cpu_thread* _cpu = get_current_cpu_thread();

		if (progress == 0 && !(_cpu->state & cpu_flag::wait))
		{
			_cpu->state += cpu_flag::wait + cpu_flag::temp;
			wait_set = true;
			return;
		}

		if (progress == umax && std::exchange(wait_set, false))
		{
			verify(HERE), !_cpu->check_state();
			return;
		}
	};

	static thread_local struct thread_cleanup_t
	{
		cpu_thread* _this;
		std::string name;

		thread_cleanup_t(cpu_thread* _this)
			: _this(_this)
			, name(thread_ctrl::get_name())
		{
		}

		void cleanup()
		{
			if (_this == nullptr)
			{
				return;
			}

			if (auto ptr = vm::g_tls_locked)
			{
				ptr->compare_and_swap(_this, nullptr);
			}

			atomic_storage_futex::set_notify_callback(nullptr);

			g_tls_log_control = [](const char*, u64){};

			g_fxo->get<cpu_counter>()->remove(_this, s_tls_thread_slot);

			_this = nullptr;
		}

		~thread_cleanup_t()
		{
			if (_this)
			{
				sys_log.warning("CPU Thread '%s' terminated abnormally:\n%s", name, _this->dump_all());
				cleanup();
			}
		}
	} cleanup{this};

	// Check thread status
	while (!(state & (cpu_flag::exit + cpu_flag::dbg_global_stop)) && thread_ctrl::state() != thread_state::aborting)
	{
		// Check stop status
		if (!(state & cpu_flag::stop))
		{
			cpu_task();

			if (state & cpu_flag::ret && state.test_and_reset(cpu_flag::ret))
			{
				cpu_return();
			}

			continue;
		}

		thread_ctrl::wait();

		if (state & cpu_flag::ret && state.test_and_reset(cpu_flag::ret))
		{
			cpu_return();
		}
	}

	// Complete cleanup gracefully
	cleanup.cleanup();
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
	bool cpu_can_stop = true;
	bool escape, retval;
	u64 susp_ctr = -1;

	while (true)
	{
		// Process all flags in a single atomic op
		const auto state0 = state.fetch_op([&](bs_t<cpu_flag>& flags)
		{
			bool store = false;

			if (flags & cpu_flag::pause && !(flags & cpu_flag::wait))
			{
				// Save value before state is saved and cpu_flag::wait is observed
				susp_ctr = g_suspend_counter;
			}

			if (flags & cpu_flag::temp) [[unlikely]]
			{
				// Sticky flag, indicates check_state() is not allowed to return true
				flags -= cpu_flag::temp;
				flags -= cpu_flag::wait;
				cpu_can_stop = false;
				store = true;
			}

			if (flags & cpu_flag::signal)
			{
				flags -= cpu_flag::signal;
				cpu_sleep_called = false;
				store = true;
			}

			// Atomically clean wait flag and escape
			if (!(flags & (cpu_flag::exit + cpu_flag::dbg_global_stop + cpu_flag::ret + cpu_flag::stop)))
			{
				// Check pause flags which hold thread inside check_state
				if (flags & (cpu_flag::pause + cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause + cpu_flag::memory))
				{
					if (!(flags & cpu_flag::wait))
					{
						flags += cpu_flag::wait;
						store = true;
					}

					escape = false;
					return store;
				}

				if (flags & cpu_flag::wait)
				{
					flags -= cpu_flag::wait;
					store = true;
				}

				retval = false;
			}
			else
			{
				if (cpu_can_stop && !(flags & cpu_flag::wait))
				{
					flags += cpu_flag::wait;
					store = true;
				}

				retval = cpu_can_stop;
			}

			if (cpu_can_stop && flags & cpu_flag::dbg_step)
			{
				// Can't process dbg_step if we only paused temporarily
				flags += cpu_flag::dbg_pause;
				flags -= cpu_flag::dbg_step;
				store = true;
			}

			escape = true;
			return store;
		}).first;

		if (escape)
		{
			if (s_tls_thread_slot == umax)
			{
				// Restore thread in the suspend list
				std::lock_guard lock(g_fxo->get<cpu_counter>()->cpu_suspend_lock);
				s_tls_thread_slot = g_fxo->get<cpu_counter>()->add(this, true);
			}

			verify(HERE), cpu_can_stop || !retval;
			verify(HERE), cpu_can_stop || !(state & cpu_flag::wait);
			return retval;
		}

		if (!cpu_sleep_called && state0 & cpu_flag::suspend)
		{
			cpu_sleep();
			cpu_sleep_called = true;

			if (cpu_can_stop && s_tls_thread_slot != umax)
			{
				// Exclude inactive threads from the suspend list (optimization)
				std::lock_guard lock(g_fxo->get<cpu_counter>()->cpu_suspend_lock);
				g_fxo->get<cpu_counter>()->remove(this);
				s_tls_thread_slot = -1;
			}

			continue;
		}

		if (state0 & (cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause))
		{
			thread_ctrl::wait();
		}
		else
		{
			if (state0 & cpu_flag::memory)
			{
				vm::passive_lock(*this);
				continue;
			}

			// If only cpu_flag::pause was set, wait on suspend counter instead
			if (state0 & cpu_flag::pause)
			{
				if (state0 & cpu_flag::wait)
				{
					// Otherwise, value must be reliable because cpu_flag::wait hasn't been observed yet
					susp_ctr = -1;
				}

				// Hard way
				if (susp_ctr == umax) [[unlikely]]
				{
					g_fxo->get<cpu_counter>()->cpu_suspend_lock.lock_unlock();
					continue;
				}

				// Wait for current suspend_all operation
				for (u64 i = 0;; i++)
				{
					if (i < 20)
					{
						busy_wait(300);
					}
					else
					{
						g_suspend_counter.wait(susp_ctr);
					}

					if (!(state & cpu_flag::pause))
					{
						break;
					}
				}

				susp_ctr = -1;
			}
		}
	}
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
		fmt::throw_exception("Invalid cpu_thread type" HERE);
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
		fmt::throw_exception("Invalid cpu_thread type" HERE);
	}
}

std::string cpu_thread::get_name() const
{
	// Downcast to correct type
	if (id_type() == 1)
	{
		return thread_ctrl::get_name(*static_cast<const named_thread<ppu_thread>*>(this));
	}
	else if (id_type() == 2)
	{
		return thread_ctrl::get_name(*static_cast<const named_thread<spu_thread>*>(this));
	}
	else
	{
		fmt::throw_exception("Invalid cpu_thread type" HERE);
	}
}

std::string cpu_thread::dump_all() const
{
	return {};
}

std::string cpu_thread::dump_regs() const
{
	return {};
}

std::string cpu_thread::dump_callstack() const
{
	return {};
}

std::vector<std::pair<u32, u32>> cpu_thread::dump_callstack_list() const
{
	return {};
}

std::string cpu_thread::dump_misc() const
{
	return fmt::format("Type: %s\n" "State: %s\n", typeid(*this).name(), state.load());
}

bool cpu_thread::suspend_work::push(cpu_thread* _this, bool cancel_if_not_suspended) noexcept
{
	// Can't allow pre-set wait bit (it'd be a problem)
	verify(HERE), !_this || !(_this->state & cpu_flag::wait);

	// cpu_counter object
	const auto ctr = g_fxo->get<cpu_counter>();

	// Try to push workload
	auto& queue = ctr->cpu_suspend_work;

	do
	{
		// Load current head
		next = queue.load();

		if (!next && cancel_if_not_suspended) [[unlikely]]
		{
			// Give up if not suspended
			return false;
		}

		if (!_this && next)
		{
			// If _this == nullptr, it only works if this is the first workload pushed
			ctr->cpu_suspend_lock.lock_unlock();
			continue;
		}
	}
	while (!queue.compare_and_swap_test(next, this));

	if (!next)
	{
		// Monitor the performance only of the actual suspend processing owner
		perf_meter<"SUSPEND"_u64> perf0;

		// First thread to push the work to the workload list pauses all threads and processes it
		std::lock_guard lock(ctr->cpu_suspend_lock);

		// Try to prefetch cpu->state earlier
		for_all_cpu([&](cpu_thread* cpu)
		{
			if (cpu != _this)
			{
				_m_prefetchw(&cpu->state);
			}
		});

		// Copy of thread bits
		decltype(ctr->cpu_copy_bits) copy2{};

		for (u32 i = 0; i < ::size32(ctr->cpu_copy_bits); i++)
		{
			copy2[i] = ctr->cpu_copy_bits[i] = ctr->cpu_array_bits[i].load();
		}

		for_all_cpu([&](cpu_thread* cpu, u64 index)
		{
			if (cpu == _this || cpu->state.fetch_add(cpu_flag::pause) & cpu_flag::wait)
			{
				// Clear bits as long as wait flag is set
				ctr->cpu_copy_bits[index / 64] &= ~(1ull << (index % 64));
			}

			if (cpu == _this)
			{
				copy2[index / 64] &= ~(1ull << (index % 64));
			}
		});

		while (std::accumulate(std::begin(ctr->cpu_copy_bits), std::end(ctr->cpu_copy_bits), u64{0}, std::bit_or()))
		{
			// Check only CPUs which haven't acknowledged their waiting state yet
			for_all_cpu<true>([&](cpu_thread* cpu, u64 index)
			{
				if (cpu->state & cpu_flag::wait)
				{
					ctr->cpu_copy_bits[index / 64] &= ~(1ull << (index % 64));
				}
			});

			_mm_pause();
		}

		// Extract queue and reverse element order (FILO to FIFO) (TODO: maybe leave order as is?)
		auto* head = queue.exchange(nullptr);

		s8 min_prio = head->prio;
		s8 max_prio = head->prio;

		if (auto* prev = head->next)
		{
			head->next = nullptr;

			do
			{
				auto* pre2 = prev->next;
				prev->next = head;

				head = std::exchange(prev, pre2);

				// Fill priority range
				min_prio = std::min<s8>(min_prio, head->prio);
				max_prio = std::max<s8>(max_prio, head->prio);
			}
			while (prev);
		}

		for_all_cpu<true>([&](cpu_thread* cpu)
		{
			_m_prefetchw(&cpu->state);
		});

		_m_prefetchw(&g_suspend_counter);

		// Execute all stored workload
		for (s32 prio = max_prio; prio >= min_prio; prio--)
		{
			// ... according to priorities
			for (auto work = head; work; work = work->next)
			{
				// Properly sorting single-linked list may require to optimize the loop
				if (work->prio == prio)
				{
					work->exec(work->func_ptr, work->res_buf);
				}
			}
		}

		// Finalization
		g_suspend_counter++;

		// Exact bitset for flag pause removal
		std::memcpy(ctr->cpu_copy_bits, copy2, sizeof(copy2));

		for_all_cpu<true>([&](cpu_thread* cpu)
		{
			cpu->state -= cpu_flag::pause;
		});
	}
	else
	{
		// Seems safe to set pause on self because wait flag hasn't been observed yet
		_this->state += cpu_flag::pause + cpu_flag::temp;
		_this->check_state();
		return true;
	}

	g_suspend_counter.notify_all();
	return true;
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
		std::lock_guard lock(g_fxo->get<cpu_counter>()->cpu_suspend_lock);

		auto on_stop = [](u32, cpu_thread& cpu)
		{
			cpu.state += cpu_flag::dbg_global_stop;
			cpu.abort();
		};

		idm::select<named_thread<ppu_thread>>(on_stop);
		idm::select<named_thread<spu_thread>>(on_stop);
	}

	sys_log.notice("All CPU threads have been signaled.");

	while (g_fxo->get<cpu_counter>()->cpu_array_sema)
	{
		std::this_thread::sleep_for(10ms);
	}

	sys_log.notice("All CPU threads have been stopped. [+: %u]", +g_threads_created);

	std::lock_guard lock(g_fxo->get<cpu_counter>()->cpu_suspend_lock);

	g_threads_deleted -= g_threads_created.load();
	g_threads_created = 0;
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
