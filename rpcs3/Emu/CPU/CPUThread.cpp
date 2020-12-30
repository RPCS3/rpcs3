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

#include "util/asm.hpp"
#include <thread>
#include <unordered_map>
#include <map>

#include <immintrin.h>
#include <emmintrin.h>

DECLARE(cpu_thread::g_threads_created){0};
DECLARE(cpu_thread::g_threads_deleted){0};
DECLARE(cpu_thread::g_suspend_counter){0};

LOG_CHANNEL(profiler);
LOG_CHANNEL(sys_log, "SYS");

static thread_local u32 s_tls_thread_slot = -1;

// Suspend counter stamp
static thread_local u64 s_tls_sctr = -1;

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
				atomic_wait::list(registered).wait();
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

// Total number of CPU threads
static atomic_t<u64, 64> s_cpu_counter{0};

// List of posted tasks for suspend_all
static atomic_t<cpu_thread::suspend_work*> s_cpu_work[128]{};

// Linked list of pushed tasks for suspend_all
static atomic_t<cpu_thread::suspend_work*> s_pushed{};

// Lock for suspend_all operations
static shared_mutex s_cpu_lock;

// Bit allocator for threads which need to be suspended
static atomic_t<u128> s_cpu_bits{};

// List of active threads which need to be suspended
static atomic_t<cpu_thread*> s_cpu_list[128]{};

namespace cpu_counter
{
	void add(cpu_thread* _this) noexcept
	{
		std::lock_guard lock(s_cpu_lock);

		u32 id = -1;

		for (u64 i = 0;; i++)
		{
			const auto [bits, ok] = s_cpu_bits.fetch_op([](u128& bits)
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
				id = utils::ctz128(~bits);

				// Register thread
				if (s_cpu_list[id].compare_and_swap_test(nullptr, _this)) [[likely]]
				{
					break;
				}

				sys_log.fatal("Unexpected slot registration failure (%u).", id);
				id = -1;
				continue;
			}

			if (i > 50)
			{
				sys_log.fatal("Too many threads.");
				return;
			}

			busy_wait(300);
		}

		s_tls_thread_slot = id;
	}

	static void remove_cpu_bit(u32 bit)
	{
		s_cpu_bits.atomic_op([=](u128& val)
		{
			val &= ~(u128{1} << (bit % 128));
		});
	}

	void remove(cpu_thread* _this) noexcept
	{
		// Return if not registered
		const u32 slot = s_tls_thread_slot;

		if (slot == umax)
		{
			return;
		}

		if (slot >= std::size(s_cpu_list))
		{
			sys_log.fatal("Index out of bounds (%u).", slot);
			return;
		}

		// Asynchronous unregister
		if (!s_cpu_list[slot].compare_and_swap_test(_this, nullptr))
		{
			sys_log.fatal("Inconsistency for array slot %u", slot);
			return;
		}

		remove_cpu_bit(slot);

		s_tls_thread_slot = -1;
	}

	template <typename F>
	u128 for_all_cpu(/*mutable*/ u128 copy, F func) noexcept
	{
		for (u128 bits = copy; bits; bits &= bits - 1)
		{
			const u32 index = utils::ctz128(bits);

			if (cpu_thread* cpu = s_cpu_list[index].load())
			{
				if constexpr (std::is_invocable_v<F, cpu_thread*, u32>)
				{
					if (!func(cpu, index))
						copy &= ~(u128{1} << index);
					continue;
				}

				if constexpr (std::is_invocable_v<F, cpu_thread*>)
				{
					if (!func(cpu))
						copy &= ~(u128{1} << index);
					continue;
				}

				sys_log.fatal("cpu_counter::for_all_cpu: bad callback");
			}
			else
			{
				copy &= ~(u128{1} << index);
			}
		}

		return copy;
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

	while (!g_fxo->get<cpu_profiler>())
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
	s_cpu_counter++;

	atomic_wait_engine::set_notify_callback([](const void*, u64 progress)
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
			ensure(!_cpu->check_state());
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
			ensure(!_cpu->check_state());
			return;
		}
	};

	static thread_local struct thread_cleanup_t
	{
		cpu_thread* _this = nullptr;
		std::string name;

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

			atomic_wait_engine::set_notify_callback(nullptr);

			g_tls_log_control = [](const char*, u64){};

			if (s_tls_thread_slot != umax)
			{
				cpu_counter::remove(_this);
			}

			s_cpu_lock.lock_unlock();

			s_cpu_counter--;

			g_tls_current_cpu_thread = nullptr;

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
	} cleanup;

	cleanup._this = this;
	cleanup.name = thread_ctrl::get_name();

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
	bool cpu_sleep_called = false;
	bool cpu_can_stop = true;
	bool escape, retval;

	while (true)
	{
		// Process all flags in a single atomic op
		const auto state0 = state.fetch_op([&](bs_t<cpu_flag>& flags)
		{
			bool store = false;

			if (flags & cpu_flag::pause && s_tls_thread_slot != umax)
			{
				// Save value before state is saved and cpu_flag::wait is observed
				if (s_tls_sctr == umax)
				{
					u64 ctr = g_suspend_counter;

					if (flags & cpu_flag::wait)
					{
						if ((ctr & 3) == 2)
						{
							s_tls_sctr = ctr;
						}
					}
					else
					{
						s_tls_sctr = ctr;
					}
				}
			}
			else
			{
				// Cleanup after asynchronous remove()
				if (flags & cpu_flag::pause && s_tls_thread_slot == umax)
				{
					flags -= cpu_flag::pause;
					store = true;
				}

				s_tls_sctr = -1;
			}

			if (flags & cpu_flag::temp) [[unlikely]]
			{
				// Sticky flag, indicates check_state() is not allowed to return true
				flags -= cpu_flag::temp;
				flags -= cpu_flag::wait;
				cpu_can_stop = false;
				store = true;
			}

			if (cpu_can_stop && flags & cpu_flag::signal)
			{
				flags -= cpu_flag::signal;
				cpu_sleep_called = false;
				store = true;
			}

			// Atomically clean wait flag and escape
			if (!(flags & (cpu_flag::exit + cpu_flag::dbg_global_stop + cpu_flag::ret + cpu_flag::stop)))
			{
				// Check pause flags which hold thread inside check_state (ignore suspend on cpu_flag::temp)
				if (flags & (cpu_flag::pause + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause + cpu_flag::memory + (cpu_can_stop ? cpu_flag::suspend : cpu_flag::pause)))
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
			if (s_tls_thread_slot == umax && !retval)
			{
				// Restore thread in the suspend list
				cpu_counter::add(this);
			}

			ensure(cpu_can_stop || !retval);
			return retval;
		}

		if (cpu_can_stop && !cpu_sleep_called && state0 & cpu_flag::suspend)
		{
			cpu_sleep();
			cpu_sleep_called = true;

			if (s_tls_thread_slot != umax)
			{
				// Exclude inactive threads from the suspend list (optimization)
				cpu_counter::remove(this);
			}

			continue;
		}

		if (state0 & ((cpu_can_stop ? cpu_flag::suspend : cpu_flag::dbg_pause) + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause))
		{
			if (state0 & cpu_flag::dbg_pause)
			{
				g_fxo->get<gdb_server>()->pause_from(this);
			}

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
				// Wait for current suspend_all operation
				for (u64 i = 0;; i++)
				{
					u64 ctr = g_suspend_counter;

					if (ctr >> 2 == s_tls_sctr >> 2)
					{
						if (i < 20 || ctr & 1)
						{
							busy_wait(300);
						}
						else
						{
							g_suspend_counter.wait(ctr, -4);
						}
					}
					else
					{
						s_tls_sctr = -1;
						break;
					}
				}
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
		fmt::throw_exception("Invalid cpu_thread type");
	}
}

u32 cpu_thread::get_pc() const
{
	const u32* pc = nullptr;

	switch (id_type())
	{
	case 1:
	{
		pc = &static_cast<const ppu_thread*>(this)->cia;
		break;
	}
	case 2:
	{
		pc = &static_cast<const spu_thread*>(this)->pc;
		break;
	}
	default: break;
	}

	return pc ? atomic_storage<u32>::load(*pc) : UINT32_MAX;
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

bool cpu_thread::suspend_work::push(cpu_thread* _this) noexcept
{
	// Can't allow pre-set wait bit (it'd be a problem)
	ensure(!_this || !(_this->state & cpu_flag::wait));

	do
	{
		// Load current head
		next = s_pushed.load();

		if (!next && cancel_if_not_suspended) [[unlikely]]
		{
			// Give up if not suspended
			return false;
		}

		if (!_this && next)
		{
			// If _this == nullptr, it only works if this is the first workload pushed
			s_cpu_lock.lock_unlock();
			continue;
		}
	}
	while (!s_pushed.compare_and_swap_test(next, this));

	if (!next)
	{
		// Monitor the performance only of the actual suspend processing owner
		perf_meter<"SUSPEND"_u64> perf0;

		// First thread to push the work to the workload list pauses all threads and processes it
		std::lock_guard lock(s_cpu_lock);

		u128 copy = s_cpu_bits.load();

		// Try to prefetch cpu->state earlier
		copy = cpu_counter::for_all_cpu(copy, [&](cpu_thread* cpu)
		{
			if (cpu != _this)
			{
				utils::prefetch_write(&cpu->state);
				return true;
			}

			return false;
		});

		// Initialization (first increment)
		g_suspend_counter += 2;

		// Copy snapshot for finalization
		u128 copy2 = copy;

		copy = cpu_counter::for_all_cpu(copy, [&](cpu_thread* cpu, u32 index)
		{
			if (cpu->state.fetch_add(cpu_flag::pause) & cpu_flag::wait)
			{
				// Clear bits as long as wait flag is set
				return false;
			}

			return true;
		});

		while (copy)
		{
			// Check only CPUs which haven't acknowledged their waiting state yet
			copy = cpu_counter::for_all_cpu(copy, [&](cpu_thread* cpu, u32 index)
			{
				if (cpu->state & cpu_flag::wait)
				{
					return false;
				}

				return true;
			});

			if (!copy)
			{
				break;
			}

			utils::pause();
		}

		// Second increment: all threads paused
		g_suspend_counter++;

		// Extract queue and reverse element order (FILO to FIFO) (TODO: maybe leave order as is?)
		auto* head = s_pushed.exchange(nullptr);

		u8 min_prio = head->prio;
		u8 max_prio = head->prio;

		if (auto* prev = head->next)
		{
			head->next = nullptr;

			do
			{
				auto* pre2 = prev->next;
				prev->next = head;

				head = std::exchange(prev, pre2);

				// Fill priority range
				min_prio = std::min<u8>(min_prio, head->prio);
				max_prio = std::max<u8>(max_prio, head->prio);
			}
			while (prev);
		}

		// Execute prefetch hint(s)
		for (auto work = head; work; work = work->next)
		{
			for (u32 i = 0; i < work->prf_size; i++)
			{
				utils::prefetch_write(work->prf_list[0]);
			}
		}

		cpu_counter::for_all_cpu(copy2, [&](cpu_thread* cpu)
		{
			utils::prefetch_write(&cpu->state);
			return true;
		});

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

		// Finalization (last increment)
		ensure(g_suspend_counter++ & 1);

		cpu_counter::for_all_cpu(copy2, [&](cpu_thread* cpu)
		{
			cpu->state -= cpu_flag::pause;
			return true;
		});
	}
	else
	{
		// Seems safe to set pause on self because wait flag hasn't been observed yet
		s_tls_sctr = g_suspend_counter;
		_this->state += cpu_flag::pause + cpu_flag::wait + cpu_flag::temp;
		_this->check_state();
		s_tls_sctr = -1;
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
		auto on_stop = [](u32, cpu_thread& cpu)
		{
			cpu.state += cpu_flag::dbg_global_stop;
			cpu.abort();
		};

		idm::select<named_thread<ppu_thread>>(on_stop);
		idm::select<named_thread<spu_thread>>(on_stop);
	}

	sys_log.notice("All CPU threads have been signaled.");

	while (s_cpu_counter)
	{
		std::this_thread::sleep_for(1ms);
	}

	sys_log.notice("All CPU threads have been stopped. [+: %u]", +g_threads_created);

	g_threads_deleted -= g_threads_created.load();
	g_threads_created = 0;
}

void cpu_thread::flush_profilers() noexcept
{
	if (!g_fxo->get<cpu_profiler>())
	{
		profiler.fatal("cpu_thread::flush_profilers() has been called incorrectly.");
		return;
	}

	if (g_cfg.core.spu_prof || false)
	{
		g_fxo->get<cpu_profiler>()->registered.push(0);
	}
}
