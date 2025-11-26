#include "stdafx.h"
#include "CPUThread.h"
#include "CPUDisAsm.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/IdManager.h"
#include "Emu/GDB.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/perf_meter.hpp"

#include "util/asm.hpp"
#include <thread>
#include <unordered_map>
#include <map>
#include <shared_mutex>

#if defined(ARCH_X64)
#include <emmintrin.h>
#endif

DECLARE(cpu_thread::g_threads_created){0};
DECLARE(cpu_thread::g_threads_deleted){0};
DECLARE(cpu_thread::g_suspend_counter){0};

LOG_CHANNEL(profiler);
LOG_CHANNEL(sys_log, "SYS");

static thread_local u32 s_tls_thread_slot = -1;

// Suspend counter stamp
static thread_local u64 s_tls_sctr = -1;

extern thread_local void(*g_tls_log_control)(const char* fmt, u64 progress);
extern thread_local std::string(*g_tls_log_prefix)();

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
		case cpu_flag::again: return "a";
		case cpu_flag::signal: return "sig";
		case cpu_flag::memory: return "mem";
		case cpu_flag::pending: return "pend";
		case cpu_flag::pending_recheck: return "pend-re";
		case cpu_flag::notify: return "ntf";
		case cpu_flag::yield: return "y";
		case cpu_flag::preempt: return "PREEMPT";
		case cpu_flag::dbg_global_pause: return "G-PAUSE";
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

enum cpu_threads_emulation_info_dump_t : u32 {};

template<>
void fmt_class_string<cpu_threads_emulation_info_dump_t>::format(std::string& out, u64 arg)
{
	// Do not dump all threads, only select few
	// Aided by thread ID for filtering
	const u32 must_have_cpu_id = static_cast<u32>(arg);

	// Dump main_thread
	const auto main_ppu = idm::get_unlocked<named_thread<ppu_thread>>(ppu_thread::id_base);

	if (main_ppu)
	{
		fmt::append(out, "\n%s's thread context:\n", main_ppu->get_name());
		main_ppu->dump_all(out);
	}

	if (must_have_cpu_id >> 24 == ppu_thread::id_base >> 24)
	{
		if (must_have_cpu_id != ppu_thread::id_base)
		{
			const auto selected_ppu = idm::get_unlocked<named_thread<ppu_thread>>(must_have_cpu_id);

			if (selected_ppu)
			{
				fmt::append(out, "\n%s's thread context:\n", selected_ppu->get_name());
				selected_ppu->dump_all(out);
			}
		}
	}
	else if (must_have_cpu_id >> 24 == spu_thread::id_base >> 24)
	{
		const auto selected_spu = idm::get_unlocked<named_thread<spu_thread>>(must_have_cpu_id);

		if (selected_spu)
		{
			if (selected_spu->get_type() == spu_type::threaded && selected_spu->group->max_num > 1u)
			{
				// Dump the information of the entire group
				// Do not block because it is a potentially sensitive context
				std::shared_lock rlock(selected_spu->group->mutex, std::defer_lock);

				for (u32 i = 0; !rlock.try_lock() && i < 100; i++)
				{
					busy_wait();
				}

				if (rlock)
				{
					for (const auto& spu : selected_spu->group->threads)
					{
						if (spu && spu != selected_spu)
						{
							fmt::append(out, "\n%s's thread context:\n", spu->get_name());
							spu->dump_all(out);
						}
					}
				}
				else
				{
					fmt::append(out, "\nFailed to dump SPU thread group's thread's information!");
				}

				// Print the specified SPU thread last
			}

			fmt::append(out, "\n%s's thread context:\n", selected_spu->get_name());
			selected_spu->dump_all(out);
		}
	}
	else if (must_have_cpu_id >> 24 == rsx::thread::id_base >> 24)
	{
		if (auto rsx = rsx::get_current_renderer())
		{
			fmt::append(out, "\n%s's thread context:\n", rsx->get_name());
			rsx->dump_all(out);
		}
	}
}

// CPU profiler thread
struct cpu_prof
{
	// PPU/SPU id enqueued for registration
	lf_queue<u32> registered;

	struct sample_info
	{
		// Block occurences: name -> sample_count
		std::unordered_map<u64, u64, value_hash<u64>> freq;

		// Total number of samples
		u64 samples = 0, idle = 0;

		// Total number of sample collected in reservation operation
		u64 reservation_samples = 0;

		// Avoid printing replicas or when not much changed
		u64 new_samples = 0;

		static constexpr u64 min_print_samples = 500;
		static constexpr u64 min_print_all_samples = min_print_samples * 20;

		void reset()
		{
			freq.clear();
			samples = 0;
			idle = 0;
			new_samples = 0;
			reservation_samples = 0;
		}

		static std::string format(const std::multimap<u64, u64, std::greater<u64>>& chart, u64 samples, u64 idle, u32 type_id, bool extended_print = false)
		{
			// Print results
			std::string results;
			results.reserve(extended_print ? 10100 : 5100);

			// Fraction of non-idle samples
			const f64 busy = 1. * (samples - idle) / samples;

			for (auto& [count, name] : chart)
			{
				const f64 _frac = count / busy / samples;

				// Print only 7 hash characters out of 11 (which covers roughly 48 bits)
				if (type_id == 2)
				{
					fmt::append(results, "\n\t[%s", fmt::base57(be_t<u64>{name}));
					results.resize(results.size() - 4);

					// Print chunk address from lowest 16 bits
					fmt::append(results, "...chunk-0x%05x]: %.4f%% (%u)", (name & 0xffff) * 4, _frac * 100., count);
				}
				else
				{
					fmt::append(results, "\n\t[0x%07x]: %.4f%% (%u)", name, _frac * 100., count);
				}

				if (results.size() >= (extended_print ? 10000 : 5000))
				{
					// Stop printing after reaching some arbitrary limit in characters
					break;
				}
			}

			return results;
		}

		static f64 get_percent(u64 dividend, u64 divisor)
		{
			if (!dividend)
			{
				return 0;
			}

			if (dividend >= divisor)
			{
				return 100;
			}

			return 100. * dividend / divisor;
		}

		// Print info
		void print(const shared_ptr<cpu_thread>& ptr)
		{
			if (new_samples < min_print_samples || samples == idle)
			{
				if (cpu_flag::exit - ptr->state)
				{
					profiler.notice("Thread \"%s\" [0x%08x]: %u samples (%.4f%% idle), %u new, %u reservation (%.4f%%): Not enough new samples have been collected since the last print.", ptr->get_name(), ptr->id, samples, get_percent(idle, samples), new_samples, reservation_samples, get_percent(reservation_samples, samples - idle));
				}

				return;
			}

			// Make reversed map: sample_count -> name
			std::multimap<u64, u64, std::greater<u64>> chart;

			for (auto& [name, count] : freq)
			{
				chart.emplace(count, name);
			}

			// Print results
			const std::string results = format(chart, samples, idle, ptr->id_type());
			profiler.notice("Thread \"%s\" [0x%08x]: %u samples (%.4f%% idle), %u new, %u reservation (%.4f%%):\n%s", ptr->get_name(), ptr->id, samples, get_percent(idle, samples), new_samples, reservation_samples, get_percent(reservation_samples, samples - idle), results);

			new_samples = 0;
		}

		static void print_all(std::unordered_map<shared_ptr<cpu_thread>, sample_info>& threads, sample_info& all_info, u32 type_id)
		{
			u64 new_samples = 0;

			// Print all results and cleanup
			for (auto& [ptr, info] : threads)
			{
				if (ptr->id_type() != type_id)
				{
					continue;
				}

				new_samples += info.new_samples;
				info.print(ptr);
			}

			std::multimap<u64, u64, std::greater<u64>> chart;

			for (auto& [ptr, info] : threads)
			{
				if (ptr->id_type() != type_id)
				{
					continue;
				}

				// This function collects thread information regardless of 'new_samples' member state
				for (auto& [name, count] : info.freq)
				{
					all_info.freq[name] += count;
				}

				all_info.samples += info.samples;
				all_info.idle += info.idle;
				all_info.reservation_samples += info.reservation_samples;
			}

			const u64 samples = all_info.samples;
			const u64 idle = all_info.idle;
			const u64 reservation = all_info.reservation_samples;
			const auto& freq = all_info.freq;

			if (samples == idle)
			{
				return;
			}

			if (new_samples < min_print_all_samples && thread_ctrl::state() != thread_state::aborting)
			{
				profiler.notice("All %s Threads: %u samples (%.4f%% idle), %u new, %u reservation (%.4f%%): Not enough new samples have been collected since the last print.", type_id == 1 ? "PPU" : "SPU", samples, get_percent(idle, samples), new_samples, reservation, get_percent(reservation, samples - idle));
				return;
			}

			for (auto& [name, count] : freq)
			{
				chart.emplace(count, name);
			}

			const std::string results = format(chart, samples, idle, type_id, true);
			profiler.notice("All %s Threads: %u samples (%.4f%% idle), %u new, %u reservation (%.4f%%):%s", type_id == 1 ? "PPU" : "SPU", samples, get_percent(idle, samples), new_samples, reservation, get_percent(reservation, samples - idle), results);
		}
	};

	sample_info all_spu_threads_info{};
	sample_info all_ppu_threads_info{};

	void operator()()
	{
		std::unordered_map<shared_ptr<cpu_thread>, sample_info> threads;

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

				shared_ptr<cpu_thread> ptr;

				if (id >> 24 == 1)
				{
					ptr = idm::get_unlocked<named_thread<ppu_thread>>(id);
				}
				else if (id >> 24 == 2)
				{
					ptr = idm::get_unlocked<named_thread<spu_thread>>(id);
				}
				else
				{
					profiler.error("Invalid Thread ID: 0x%08x", id);
					continue;
				}

				if (ptr && cpu_flag::exit - ptr->state)
				{
					auto [found, add] = threads.try_emplace(std::move(ptr));

					if (!add)
					{
						// Overwritten (impossible?): print previous data
						found->second.print(found->first);
						found->second.reset();
					}
				}
			}

			if (threads.empty())
			{
				// Wait for messages if no work (don't waste CPU)
				thread_ctrl::wait_on(registered);
				continue;
			}

			// Sample active threads
			for (auto& [ptr, info] : threads)
			{
				if (auto state = +ptr->state; cpu_flag::exit - state)
				{
					const auto spu = ptr->try_get<spu_thread>();
					const auto ppu = ptr->try_get<ppu_thread>();

					// Get short function hash
					const u64 name = ppu ? atomic_storage<u32>::load(ppu->cia) : atomic_storage<u64>::load(ptr->block_hash);

					// Append occurrence
					info.samples++;

					if (cpu_flag::wait - state)
					{
						info.freq[name]++;
						info.new_samples++;

						if (spu)
						{
							if (spu->raddr)
							{
								info.reservation_samples++;
							}

							// Append verification time to fixed common name 0000000...chunk-0x3fffc
							if (name >> 16 && (name & 0xffff) == 0)
								info.freq[0xffff]++;
						}
					}
					else
					{
						if (state & (cpu_flag::dbg_pause + cpu_flag::dbg_global_pause))
						{
							// Idle state caused by emulation pause is not accounted for
							continue;
						}

						info.idle++;
					}
				}
				else
				{
					info.print(ptr);
				}
			}

			if (flush)
			{
				profiler.success("Flushing profiling results...");

				all_ppu_threads_info = {};
				all_spu_threads_info = {};
				sample_info::print_all(threads, all_ppu_threads_info, 1);
				sample_info::print_all(threads, all_spu_threads_info, 2);
			}

			if (Emu.IsPaused())
			{
				thread_ctrl::wait_for(5000);
				continue;
			}

			if (!g_cfg.core.spu_debug)
			{
				// Reduce accuracy in favor of performance when enabled alone
				thread_ctrl::wait_for(60, false);
				continue;
			}

			// Wait, roughly for 20us
			thread_ctrl::wait_for(20, false);
		}

		// Print all remaining results
		sample_info::print_all(threads, all_ppu_threads_info, 1);
		sample_info::print_all(threads, all_spu_threads_info, 2);
	}

	static constexpr auto thread_name = "CPU Profiler"sv;
};


using cpu_profiler = named_thread<cpu_prof>;

extern f64 get_cpu_program_usage_percent(u64 hash)
{
	if (auto prof = g_fxo->try_get<cpu_profiler>(); prof && *prof == thread_state::finished)
	{
		if (Emu.IsStopped())
		{
			u64 total = 0;

			for (auto [name, count] : prof->all_spu_threads_info.freq)
			{
				if ((name & -65536) == hash)
				{
					total += count;
				}
			}

			if (!total)
			{
				return 0;
			}

			return std::max<f64>(0.0001, static_cast<f64>(total) * 100 / (prof->all_spu_threads_info.samples - prof->all_spu_threads_info.idle));
		}
	}

	return 0;
}

thread_local DECLARE(cpu_thread::g_tls_this_thread) = nullptr;

// Total number of CPU threads
static atomic_t<u64, 64> s_cpu_counter{0};

// List of posted tasks for suspend_all
//static atomic_t<cpu_thread::suspend_work*> s_cpu_work[128]{};

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
		switch (_this->get_class())
		{
		case thread_class::ppu:
		case thread_class::spu:
			break;
		default: return;
		}

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
	const auto old_prefix = g_tls_log_prefix;

	g_tls_this_thread = this;

	if (g_cfg.core.thread_scheduler != thread_scheduler_mode::os)
	{
		thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(get_class()));
	}

	ensure(g_fxo->is_init<cpu_profiler>());

	switch (get_class())
	{
	case thread_class::ppu:
	{
		if (g_cfg.core.ppu_prof)
		{
			g_fxo->get<cpu_profiler>().registered.push(id);
		}

		break;
	}
	case thread_class::spu:
	{
		if (g_cfg.core.spu_prof || g_cfg.core.spu_debug)
		{
			g_fxo->get<cpu_profiler>().registered.push(id);
		}

		break;
	}
	default: break;
	}

	// Register thread in g_cpu_array
	s_cpu_counter++;

	static thread_local struct thread_cleanup_t
	{
		cpu_thread* _this = nullptr;
		std::string name;
		std::string(*log_prefix)() = nullptr;

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

			g_tls_log_control = [](const char*, u64){};

			if (s_tls_thread_slot != umax)
			{
				cpu_counter::remove(_this);
			}

			s_cpu_lock.lock_unlock();

			s_cpu_counter--;

			g_tls_log_prefix = log_prefix;

			g_tls_this_thread = nullptr;

			g_threads_deleted++;

			_this = nullptr;
		}

		~thread_cleanup_t()
		{
			if (_this)
			{
				sys_log.warning("CPU Thread '%s' terminated abnormally!", name);
				cleanup();
			}
		}
	} cleanup;

	cleanup._this = this;
	cleanup.name = thread_ctrl::get_name();
	cleanup.log_prefix = old_prefix;

	// Check thread status
	while (!(state & cpu_flag::exit) && thread_ctrl::state() != thread_state::aborting)
	{
		// Check stop status
		const auto state0 = +state;

		if (is_stopped(state0 - cpu_flag::stop))
		{
			break;
		}

		if (!(state0 & cpu_flag::stop))
		{
			cpu_task();
			state += cpu_flag::wait;

			if (state & cpu_flag::ret && state.test_and_reset(cpu_flag::ret))
			{
				cpu_return();
			}

			continue;
		}

		state.wait(state0);

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
}

cpu_thread::cpu_thread(u32 id)
	: id(id)
{
	while (Emu.GetStatus() == system_state::paused)
	{
		// Solve race between Emulator::Pause and this construction of thread which most likely is guarded by IDM mutex
		state += cpu_flag::dbg_global_pause;

		if (Emu.GetStatus() != system_state::paused)
		{
			// Emulator::Resume was called inbetween
			state -= cpu_flag::dbg_global_pause;

			// Recheck if state is inconsistent
			continue;
		}

		break;
	}

	if (Emu.IsStopped())
	{
		// For similar race as above
		state += cpu_flag::exit;
	}

	g_threads_created++;

	if (u32* pc2 = get_pc2())
	{
		*pc2 = umax;
	}
}

void cpu_thread::cpu_wait(bs_t<cpu_flag> old)
{
	state.wait(old);
}

static atomic_t<u32> s_dummy_atomic = 0;

bool cpu_thread::check_state() noexcept
{
	bool cpu_sleep_called = false;
	bool cpu_memory_checked = false;
	bool cpu_can_stop = true;
	bool escape{}, retval{};

	while (true)
	{
		// Process all flags in a single atomic op
		bs_t<cpu_flag> state1;
		auto state0 = state.fetch_op([&](bs_t<cpu_flag>& flags)
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
				cpu_can_stop = false;
				store = true;
			}

			if (cpu_can_stop && flags & cpu_flag::signal)
			{
				flags -= cpu_flag::signal;
				cpu_sleep_called = false;
				store = true;
			}

			if (flags & cpu_flag::notify)
			{
				flags -= cpu_flag::notify;
				store = true;
			}

			// Can't process dbg_step if we only paused temporarily
			if (cpu_can_stop && flags & cpu_flag::dbg_step)
			{
				if (u32 pc = get_pc(), *pc2 = get_pc2(); pc != umax && pc2)
				{
					if (pc != *pc2)
					{
						flags -= cpu_flag::dbg_step;
						flags += cpu_flag::dbg_pause;
						store = true;
					}
				}
				else
				{
					// Can't test, ignore flag
					flags -= cpu_flag::dbg_step;
					store = true;
				}
			}

			// Atomically clean wait flag and escape
			if (!is_stopped(flags) && flags.none_of(cpu_flag::ret))
			{
				// Check pause flags which hold thread inside check_state (ignore suspend/debug flags on cpu_flag::temp)
				if (flags & cpu_flag::pause || (!cpu_memory_checked && flags & cpu_flag::memory) || (cpu_can_stop && flags & (cpu_flag::dbg_global_pause + cpu_flag::dbg_pause + cpu_flag::suspend + cpu_flag::yield + cpu_flag::preempt)))
				{
					if (!(flags & cpu_flag::wait))
					{
						flags += cpu_flag::wait;
						store = true;
					}

					if (flags & (cpu_flag::yield + cpu_flag::preempt) && cpu_can_stop)
					{
						flags -= (cpu_flag::yield + cpu_flag::preempt);
						store = true;
					}

					escape = false;
					state1 = flags;
					return store;
				}

				if (flags & (cpu_flag::wait + cpu_flag::memory))
				{
					flags -= (cpu_flag::wait + cpu_flag::memory);
					store = true;
				}

				if (s_tls_thread_slot == umax)
				{
					if (cpu_flag::wait - this->state.load())
					{
						// Force wait flag (must be set during ownership of s_cpu_lock), this makes the atomic op fail as a side effect
						this->state += cpu_flag::wait;
						store = true;
					}

					// Restore thread in the suspend list
					cpu_counter::add(this);
				}

				retval = false;
			}
			else
			{
				if (flags & cpu_flag::wait)
				{
					flags -= cpu_flag::wait;
					store = true;
				}

				retval = cpu_can_stop;
			}

			escape = true;
			state1 = flags;
			return store;
		}).first;

		if (state0 & cpu_flag::preempt && cpu_can_stop)
		{
			if (cpu_flag::wait - state0)
			{
				if (!escape || !retval)
				{
					// Yield itself
					state0 += cpu_flag::yield;
					escape = false;
				}
			}

			if (const u128 bits = s_cpu_bits)
			{
				reader_lock lock(s_cpu_lock);

				cpu_counter::for_all_cpu(bits & s_cpu_bits, [](cpu_thread* cpu)
				{
					if (cpu->state.none_of(cpu_flag::wait + cpu_flag::yield))
					{
						cpu->state += cpu_flag::yield;
					}

					return true;
				});
			}
		}

		if (escape)
		{
			if (vm::g_range_lock_bits[1] && vm::g_tls_locked && *vm::g_tls_locked == this)
			{
				state += cpu_flag::wait + cpu_flag::memory;
				cpu_sleep_called = false;
				cpu_memory_checked = false;
				continue;
			}

			if (cpu_can_stop && state0 & cpu_flag::pending)
			{
				// Execute pending work
				cpu_work();

				if ((state1 ^ state) - cpu_flag::pending)
				{
					// Work could have changed flags
					// Reset internal flags as if check_state() has just been called
					cpu_sleep_called = false;
					cpu_memory_checked = false;
					continue;
				}
			}

			if (retval)
			{
				cpu_on_stop();
			}

			ensure(cpu_can_stop || !retval);
			return retval;
		}

		if (cpu_can_stop && !cpu_sleep_called && state0 & cpu_flag::suspend)
		{
			cpu_sleep();
			cpu_sleep_called = true;
			cpu_memory_checked = false;

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
				g_fxo->get<gdb_server>().pause_from(this);
			}

			cpu_wait(state1);
		}
		else
		{
			if (state0 & cpu_flag::memory)
			{
				vm::passive_lock(*this);
				cpu_memory_checked = true;
				continue;
			}

			// If only cpu_flag::pause was set, wait on suspend counter instead
			if (state0 & cpu_flag::pause)
			{
				// Wait for current suspend_all operation
				for (u64 i = 0;; i++)
				{
					u64 ctr = g_suspend_counter;

					if (ctr >> 2 == s_tls_sctr >> 2 && state & cpu_flag::pause)
					{
						if (i < 20 || ctr & 1)
						{
							busy_wait(300);
						}
						else
						{
							// TODO: fix the workaround
							g_suspend_counter.wait(ctr, atomic_wait_timeout{10'000});
						}
					}
					else
					{
						s_tls_sctr = -1;
						break;
					}
				}

				continue;
			}

			if (state0 & cpu_flag::yield && cpu_can_stop)
			{
				if (auto spu = try_get<spu_thread>())
				{
					if (spu->raddr && spu->rtime == vm::reservation_acquire(spu->raddr) && spu->getllar_spin_count < 10)
					{
						// Reservation operation is a critical section (but this may result in false positives)
						continue;
					}
				}
				else if (auto ppu = try_get<ppu_thread>())
				{
					if (u32 usec = ppu->hw_sleep_time)
					{
						thread_ctrl::wait_for_accurate(usec);
						ppu->hw_sleep_time = 0;
						ppu->raddr = 0; // Also lose reservation if there is any (reservation is unsaved on hw thread switch)
						continue;
					}

					if (ppu->raddr && ppu->rtime == vm::reservation_acquire(ppu->raddr))
					{
						// Same
						continue;
					}
				}

				// Short sleep when yield flag is present alone (makes no sense when other methods which can stop thread execution have been done)
				s_dummy_atomic.wait(0, atomic_wait_timeout{80'000});
			}
		}
	}
}

void cpu_thread::notify()
{
	state.notify_one();

	// Downcast to correct type
	switch (get_class())
	{
	case thread_class::ppu:
	{
		thread_ctrl::notify(*static_cast<named_thread<ppu_thread>*>(this));
		break;
	}
	case thread_class::spu:
	{
		thread_ctrl::notify(*static_cast<named_thread<spu_thread>*>(this));
		break;
	}
	case thread_class::rsx:
	{
		break;
	}
	default:
	{
		fmt::throw_exception("Invalid cpu_thread type");
	}
	}
}

cpu_thread& cpu_thread::operator=(thread_state)
{
	if (state & cpu_flag::exit)
	{
		// Must be notified elsewhere or self-raised
		return *this;
	}

	const auto old = state.fetch_add(cpu_flag::exit);

	if (old & cpu_flag::wait && old.none_of(cpu_flag::again + cpu_flag::exit))
	{
		state.notify_one();

		if (auto thread = try_get<spu_thread>())
		{
			if (u32 resv = atomic_storage<u32>::load(thread->raddr))
			{
				vm::reservation_notifier_notify(resv, thread->rtime);
			}
		}
	}

	return *this;
}

void cpu_thread::add_remove_flags(bs_t<cpu_flag> to_add, bs_t<cpu_flag> to_remove)
{
	bs_t<cpu_flag> result{};

	if (!to_remove)
	{
		state.add_fetch(to_add);
		return;
	}
	else if (!to_add)
	{
		result = state.sub_fetch(to_remove);
	}
	else
	{
		result = state.atomic_op([&](bs_t<cpu_flag>& v)
		{
			v += to_add;
			v -= to_remove;
			return v;
		});
	}

	if (!::is_paused(to_remove) && !::is_stopped(to_remove))
	{
		// No notable change requiring notification
		return;
	}

	if (::is_paused(result) || ::is_stopped(result))
	{
		// Flags that stop thread execution
		return;
	}

	state.notify_one();
}

std::string cpu_thread::get_name() const
{
	// Downcast to correct type
	switch (get_class())
	{
	case thread_class::ppu:
	{
		return thread_ctrl::get_name(*static_cast<const named_thread<ppu_thread>*>(this));
	}
	case thread_class::spu:
	{
		return thread_ctrl::get_name(*static_cast<const named_thread<spu_thread>*>(this));
	}
	default:
	{
		if (cpu_thread::get_current() == this && thread_ctrl::get_current())
		{
			return thread_ctrl::get_name();
		}

		if (get_class() == thread_class::rsx)
		{
			return fmt::format("rsx::thread");
		}

		return fmt::format("Invalid cpu_thread type (0x%x)", id_type());
	}
	}
}

u32 cpu_thread::get_pc() const
{
	const u32* pc = nullptr;

	switch (get_class())
	{
	case thread_class::ppu:
	{
		pc = &static_cast<const ppu_thread*>(this)->cia;
		break;
	}
	case thread_class::spu:
	{
		pc = &static_cast<const spu_thread*>(this)->pc;
		break;
	}
	case thread_class::rsx:
	{
		const auto ctrl = static_cast<const rsx::thread*>(this)->ctrl;
		return ctrl ? ctrl->get.load() : umax;
	}
	default: break;
	}

	return pc ? atomic_storage<u32>::load(*pc) : u32{umax};
}

u32* cpu_thread::get_pc2()
{
	switch (get_class())
	{
	case thread_class::ppu:
	{
		return &static_cast<ppu_thread*>(this)->dbg_step_pc;
	}
	case thread_class::spu:
	{
		return &static_cast<spu_thread*>(this)->dbg_step_pc;
	}
	case thread_class::rsx:
	{
		const auto ctrl = static_cast<rsx::thread*>(this)->ctrl;
		return ctrl ? &static_cast<rsx::thread*>(this)->dbg_step_pc : nullptr;
	}
	default: break;
	}

	return nullptr;
}

cpu_thread* cpu_thread::get_next_cpu()
{
	switch (get_class())
	{
	case thread_class::ppu:
	{
		return static_cast<ppu_thread*>(this)->next_cpu;
	}
	case thread_class::spu:
	{
		return static_cast<spu_thread*>(this)->next_cpu;
	}
	default: break;
	}

	return nullptr;
}

extern std::shared_ptr<CPUDisAsm> make_disasm(const cpu_thread* cpu, shared_ptr<cpu_thread> handle);

void cpu_thread::dump_all(std::string& ret) const
{
	std::any func_data;

	ret += dump_misc();
	ret += '\n';
	dump_regs(ret, func_data);
	ret += '\n';
	ret += dump_callstack();
	ret += '\n';

	if (u32 cur_pc = get_pc(); cur_pc != umax)
	{
		// Dump a snippet of currently executed code (may be unreliable with non-static-interpreter decoders)
		auto disasm = make_disasm(this, null_ptr);

		const auto rsx = try_get<rsx::thread>();

		for (u32 i = (rsx ? rsx->try_get_pc_of_x_cmds_backwards(20, cur_pc).second : cur_pc - 4 * 20), count = 0; count < 30; count++)
		{
			u32 advance = disasm->disasm(i);
			ret += disasm->last_opcode;
			i += std::max(advance, 4u);
			disasm->dump_pc = i;
			ret += '\n';
		}
	}
}

void cpu_thread::dump_regs(std::string&, std::any&) const
{
}

std::string cpu_thread::dump_callstack() const
{
	std::string ret;

	fmt::append(ret, "Call stack:\n=========\n0x%08x (0x0) called\n", get_pc());

	for (const auto& sp : dump_callstack_list())
	{
		fmt::append(ret, "> from 0x%08x (sp=0x%08x)\n", sp.first, sp.second);
	}

	return ret;
}

std::vector<std::pair<u32, u32>> cpu_thread::dump_callstack_list() const
{
	return {};
}

std::string cpu_thread::dump_misc() const
{
	return fmt::format("Type: %s; State: %s\n", get_class() == thread_class::ppu ? "PPU" : get_class() == thread_class::spu ? "SPU" : "RSX", state.load());
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

		copy = cpu_counter::for_all_cpu(copy, [&](cpu_thread* cpu, u32 /*index*/)
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
			copy = cpu_counter::for_all_cpu(copy, [&](cpu_thread* cpu, u32 /*index*/)
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

void cpu_thread::cleanup() noexcept
{
	if (u64 count = s_cpu_counter)
	{
		fmt::throw_exception("cpu_thread::cleanup(): %u threads are still active! (created=%u, destroyed=%u)", count, +g_threads_created, +g_threads_deleted);
	}

	sys_log.notice("All CPU threads have been stopped. [+: %u]", +g_threads_created);

	g_threads_deleted -= g_threads_created.load();
	g_threads_created = 0;
}

void cpu_thread::flush_profilers() noexcept
{
	if (!g_fxo->is_init<cpu_profiler>())
	{
		profiler.fatal("cpu_thread::flush_profilers() has been called incorrectly.");
		return;
	}

	if (g_cfg.core.spu_prof || g_cfg.core.spu_debug || g_cfg.core.ppu_prof)
	{
		g_fxo->get<cpu_profiler>().registered.push(0);
	}
}

u32 CPUDisAsm::DisAsmBranchTarget(s32 /*imm*/)
{
	// Unused
	return 0;
}

extern bool try_lock_spu_threads_in_a_state_compatible_with_savestates(bool revert_lock, std::vector<std::pair<shared_ptr<named_thread<spu_thread>>, u32>>* out_list)
{
	if (out_list)
	{
		out_list->clear();
	}

	auto get_spus = [old_counter = u64{umax}, spu_list = std::vector<shared_ptr<named_thread<spu_thread>>>()](bool can_collect, bool force_collect) mutable
	{
		const u64 new_counter = cpu_thread::g_threads_created + cpu_thread::g_threads_deleted;

		if (old_counter != new_counter)
		{
			if (!can_collect)
			{
				return decltype(&spu_list){};
			}

			const u64 current = get_system_time();

			// Fetch SPU contexts
			spu_list.clear();

			bool give_up = false;

			idm::select<named_thread<spu_thread>>([&](u32 id, spu_thread& spu)
			{
				if (give_up)
				{
					return;
				}

				if (spu.current_func && spu.unsavable && !force_collect)
				{
					const u64 start = spu.start_time;

					// Automatically give up if it is asleep 5 seconds or more
					if (start && current > start && current - start >= 5'000'000)
					{
						give_up = true;
						return;
					}
				}

				spu_list.emplace_back(ensure(idm::get_unlocked<named_thread<spu_thread>>(id)));
			});

			if (give_up)
			{
				spu_list.clear();
				old_counter = umax;
				return decltype(&spu_list){};
			}

			old_counter = new_counter;
		}

		return &spu_list;
	};

	// Attempt to lock for a second, if somehow takes longer abort it
	for (u64 start = 0, passed_count = 0; passed_count < 15;)
	{
		if (revert_lock)
		{
			// Revert the operation of this function
			break;
		}

		if (!start)
		{
			start = get_system_time();
		}
		else if (get_system_time() - start >= 150'000)
		{
			std::this_thread::sleep_for(1ms);
			passed_count++;
			start = 0;
			continue;
		}

		// Try to fetch SPUs out of critical section
		const auto spu_list = get_spus(true, false);

		if (!spu_list)
		{
			// Give up for now
			std::this_thread::sleep_for(50ms);
			passed_count++;
			start = 0;
			continue;
		}

		// Avoid using suspend_all when more than 2 threads known to be unsavable
		u32 savable_threads = 0;

		for (auto& spu : *spu_list)
		{
			if (!spu->unsavable)
			{
				savable_threads++;
			}
		}

		if (!savable_threads)
		{
			std::this_thread::yield();
			continue;
		}

		if (cpu_thread::suspend_all(nullptr, {}, [&]()
		{
			if (!get_spus(false, true))
			{
				// Avoid locking IDM here because this is a critical section
				return true;
			}

			bool failed = false;
			const bool is_emu_paused = Emu.IsPaused();

			for (auto& spu : *spu_list)
			{
				if (spu->unsavable)
				{
					failed = true;
					break;
				}

				if (is_emu_paused)
				{
					// If emulation is paused, we can only hope it's already in a state compatible with savestates
					if (!(spu->state & (cpu_flag::dbg_global_pause + cpu_flag::dbg_pause)))
					{
						failed = true;
						break;
					}
				}
			}

			for (auto& spu : *spu_list)
			{
				if (!failed && !is_emu_paused)
				{
					ensure(!spu->state.test_and_set(cpu_flag::dbg_global_pause));
				}
			}

			return failed;
		}))
		{
			if (Emu.IsPaused())
			{
				return false;
			}

			for (auto& spu : *spu_list)
			{
				if (spu->state & cpu_flag::wait)
				{
					spu->state.notify_one();
				}
			}

			std::this_thread::yield();
			continue;
		}

		if (out_list)
		{
			for (auto& spu : *spu_list)
			{
				out_list->emplace_back(spu, spu->pc);
			}
		}

		return true;
	}

	for (auto& spu : *get_spus(true, true))
	{
		if (spu->state.test_and_reset(cpu_flag::dbg_global_pause))
		{
			spu->state.notify_one();
		}
	}

	return false;
}
