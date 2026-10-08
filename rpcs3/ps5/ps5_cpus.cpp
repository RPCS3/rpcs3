// PS5: the console's CPUs as a title gets them (ps5_cpus.h)

#include "stdafx.h"
#include "ps5_cpus.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/Thread.h"

#include <pthread.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <bit>
#include <chrono>
#include <thread>

// The platform layer's affinity calls (ps5platform/libc.h), which take a set of
// any size: libc's refuses FreeBSD's 32-byte cpuset_t (ERANGE), and the kernel's
// current CPU (ps5platform/kernel.h)
extern "C" int ps5_pthread_getaffinity_np(pthread_t thread, size_t size, void* set);
extern "C" int ps5_pthread_setaffinity_np(pthread_t thread, size_t size, const void* set);
extern "C" int32_t sceKernelGetCurrentCpu(void);

namespace
{
	std::atomic<u64> g_cpus = 0;    // the title's CPUs, once measured
	s8 g_sibling[64];               // each one's SMT sibling among them, or -1; written once, before g_cpus
	atomic_t<u64> g_own_core_run = 0; // the emulation whose SPU took the core
	std::string g_layout;           // what the measurement found, for the status lines; written once, before g_cpus
	shared_mutex g_own_core_mutex;
	std::string g_own_core_thread;  // the SPU on the kept core, and its CPU
	s32 g_own_core_cpu = -1;

	bool pin_self(s32 cpu)
	{
		const u64 mask = 1ull << cpu;
		if (ps5_pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) != 0)
		{
			return false;
		}
		// The move may wait for the next switch
		for (int tries = 0; tries < 100 && sceKernelGetCurrentCpu() != cpu; tries++)
		{
			std::this_thread::yield();
		}
		return sceKernelGetCurrentCpu() == cpu;
	}

	// A busy loop's iterations on `cpu` in `ms`, beside the same loop on
	// `other` (or alone, for -1); 0 when a thread could not be placed
	u64 loop_speed(s32 cpu, s32 other, f64 ms)
	{
		std::atomic<int> ready = 0;
		std::atomic<bool> go = false;
		std::atomic<bool> stop = false;
		std::atomic<bool> placed = true;
		u64 counts[2]{};

		// Eight independent products an iteration keep the core's multiplier
		// busy, so its other thread's loop takes half of it, as PS5_PayloadSDK's
		// topology probe has it (src/probe_threads.c); and the loop runs until
		// told to stop, looking at nothing else. Build 100's one chain of
		// products left the multiplier mostly idle, and build 101's loop asked
		// the clock every two thousand products, a call into the kernel that
		// took most of its time: neither slowed beside its sibling (92-100%,
		// on my console)
		const auto loop = [&](s32 on, u64& count)
		{
			if (!pin_self(on))
			{
				placed = false;
			}
			ready++;
			while (!go.load(std::memory_order_acquire))
			{
			}
			u64 n = 0;
			u64 v[8]{1, 2, 3, 4, 5, 6, 7, 8};
			while (!stop.load(std::memory_order_relaxed))
			{
				for (u32 k = 0; k < 256; k++)
				{
					for (u32 i = 0; i < 8; i++)
					{
						v[i] = v[i] * 6364136223846793005ull + i;
					}
				}
				n++;
			}
			count = n + ((v[0] ^ v[1] ^ v[2] ^ v[3] ^ v[4] ^ v[5] ^ v[6] ^ v[7]) == 42); // the products kept alive
		};

		std::thread first(loop, cpu, std::ref(counts[0]));
		std::thread second;
		if (other >= 0)
		{
			second = std::thread(loop, other, std::ref(counts[1]));
		}
		while (ready.load() < (other >= 0 ? 2 : 1))
		{
			std::this_thread::yield();
		}
		go.store(true, std::memory_order_release);
		std::this_thread::sleep_for(std::chrono::duration<f64, std::milli>(ms));
		stop.store(true, std::memory_order_relaxed);
		first.join();
		if (second.joinable())
		{
			second.join();
		}
		return placed ? counts[0] : 0;
	}
}

s32 ps5_current_cpu()
{
	return sceKernelGetCurrentCpu();
}

// The console's other ways to keep a thread on a CPU, bound by name: an
// attribute at creation, and FreeBSD's cpuset_setaffinity (libkernel exports
// both, beside the scePthreadSetaffinity the platform layer calls)
extern "C" int ps5_attr_setaffinity(pthread_attr_t* attr, size_t size, const void* set) __asm__("pthread_attr_setaffinity_np");
extern "C" int ps5_cpuset_setaffinity(int level, int which, s64 id, size_t size, const void* set) __asm__("cpuset_setaffinity");

namespace
{
	// Where a spinning thread runs, sampled for `ms`
	struct cpu_samples
	{
		u32 counts[64]{};
		u32 total = 0;

		void take(f64 ms)
		{
			const auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64, std::milli>(ms);
			for (u32 round = 1;; round++)
			{
				for (u32 i = 0; i < 2048; i++)
				{
					__asm__ volatile("" ::: "memory");
				}
				const s32 cpu = sceKernelGetCurrentCpu();
				counts[cpu >= 0 && cpu < 64 ? cpu : 63]++;
				total++;
				if (round % 64 == 0 && std::chrono::steady_clock::now() >= end)
				{
					break;
				}
			}
		}

		std::string describe(s32 target) const
		{
			if (!total)
			{
				return "no samples";
			}
			u32 seen = 0;
			s32 top = 0;
			for (s32 cpu = 0; cpu < 64; cpu++)
			{
				seen += counts[cpu] != 0;
				top = counts[cpu] > counts[top] ? cpu : top;
			}
			std::string text = target >= 0 ? fmt::format("on CPU %d %.0f%% of the time", target, 100.0 * counts[target] / total) : fmt::format("on %u CPUs", seen);
			if (target < 0 || counts[target] < total)
			{
				fmt::append(text, " (%u CPUs, CPU %d the commonest at %.0f%%)", seen, top, 100.0 * counts[top] / total);
			}
			return text;
		}
	};

	struct pin_test
	{
		s32 target;
		int how; // 0 none, 1 scePthreadSetaffinity once running, 2 an attribute at creation, 3 cpuset_setaffinity once running
		int result = -1;
		cpu_samples samples;
	};

	void* pin_test_body(void* opaque)
	{
		pin_test& test = *static_cast<pin_test*>(opaque);
		const u64 mask = 1ull << test.target;
		if (test.how == 1)
		{
			test.result = ps5_pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
		}
		else if (test.how == 3)
		{
			// CPU_LEVEL_WHICH, CPU_WHICH_TID, this thread; FreeBSD's cpuset_t is 32 bytes
			u64 set[4]{mask};
			test.result = ps5_cpuset_setaffinity(3, 1, -1, sizeof(set), set);
			if (test.result != 0)
			{
				test.result = ps5_cpuset_setaffinity(3, 1, -1, sizeof(mask), &mask) == 0 ? 0 : (errno ? errno : -1);
			}
		}
		test.samples.take(250.0);
		return nullptr;
	}
}

std::string ps5_test_pinning()
{
	u64 mask = 0;
	if (ps5_pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) != 0 || !mask)
	{
		return "the title's CPUs could not be read";
	}
	const s32 target = 63 - std::countl_zero(mask);
	std::string out;
	for (int how = 0; how < 4; how++)
	{
		pin_test test{how ? target : -1, how};
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		if (how == 2)
		{
			const u64 one = 1ull << target;
			test.result = ps5_attr_setaffinity(&attr, sizeof(one), &one);
		}
		pthread_t thread;
		const int created = pthread_create(&thread, &attr, pin_test_body, &test);
		pthread_attr_destroy(&attr);
		if (created != 0)
		{
			fmt::append(out, "%stest %d: no thread (%d)", out.empty() ? "" : "; ", how, created);
			continue;
		}
		pthread_join(thread, nullptr);
		static constexpr const char* names[] = {"unpinned", "scePthreadSetaffinity once running", "an affinity attribute at creation", "cpuset_setaffinity once running"};
		fmt::append(out, "%s%s", out.empty() ? "" : "; ", names[how]);
		if (how)
		{
			fmt::append(out, " (result %d)", test.result);
		}
		fmt::append(out, ": %s", test.samples.describe(test.target));
	}
	return out;
}

std::string ps5_measure_cpus()
{
	u64 mask = 0;
	if (ps5_pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) != 0 || !mask)
	{
		return "the title's CPUs could not be read";
	}

	const auto started = std::chrono::steady_clock::now();
	std::fill(std::begin(g_sibling), std::end(g_sibling), s8{-1});
	constexpr f64 run_ms = 15.0;
	f64 sibling_low = 1.0, sibling_high = 0.0, others_low = 2.0, others_high = 0.0;
	bool refused = false;

	for (s32 cpu = 0; cpu < 64; cpu++)
	{
		if (!(mask >> cpu & 1) || g_sibling[cpu] >= 0)
		{
			continue;
		}

		// Alone: the better of two runs after one that wakes the core
		loop_speed(cpu, -1, run_ms);
		const u64 alone = std::max(loop_speed(cpu, -1, run_ms), loop_speed(cpu, -1, run_ms));
		if (!alone)
		{
			refused = true;
			continue;
		}

		for (s32 other = cpu + 1; other < 64; other++)
		{
			if (!(mask >> other & 1) || g_sibling[other] >= 0)
			{
				continue;
			}
			const u64 beside = loop_speed(cpu, other, run_ms);
			if (!beside)
			{
				refused = true;
				continue;
			}
			const f64 kept = static_cast<f64>(beside) / alone;
			// Beside its sibling build 103's loop kept 74-75% on my console, beside the others nearly all
			if (kept < 0.85)
			{
				g_sibling[cpu] = static_cast<s8>(other);
				g_sibling[other] = static_cast<s8>(cpu);
				sibling_low = std::min(sibling_low, kept);
				sibling_high = std::max(sibling_high, kept);
				break;
			}
			others_low = std::min(others_low, kept);
			others_high = std::max(others_high, kept);
		}
	}

	// Back on every CPU it had
	ps5_pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);

	std::string cores, alone;
	for (s32 cpu = 0; cpu < 64; cpu++)
	{
		if (!(mask >> cpu & 1))
		{
			continue;
		}
		if (g_sibling[cpu] < 0)
		{
			fmt::append(alone, "%s%d", alone.empty() ? "" : ", ", cpu);
		}
		else if (g_sibling[cpu] > cpu)
		{
			fmt::append(cores, " %d+%d", cpu, g_sibling[cpu]);
		}
	}
	const f64 seconds = std::chrono::duration<f64>(std::chrono::steady_clock::now() - started).count();
	std::string result = fmt::format("the title's CPUs 0x%x (%u); cores shared:%s", mask, std::popcount(mask), cores.empty() ? " none found" : cores.c_str());
	if (!alone.empty())
	{
		fmt::append(result, "; without a sibling among them: %s", alone);
	}
	if (sibling_high > 0.0)
	{
		fmt::append(result, "; a busy loop kept %.0f-%.0f%% of its speed beside its sibling", sibling_low * 100, sibling_high * 100);
	}
	if (others_high > 0.0)
	{
		fmt::append(result, ", %.0f-%.0f%% beside the others", others_low * 100, others_high * 100);
	}
	fmt::append(result, "%s (%.2f s)", refused ? "; some CPUs refused a thread" : "", seconds);
	g_layout = cores.empty() ? "no shared core found" : "cores shared:" + cores;
	g_cpus = mask;
	return result;
}

namespace
{
	// The kept core: the last pair of siblings among the title's CPUs; with
	// none found, the last CPU alone, so the SPU at least stays on one
	std::pair<s32, s32> kept_core()
	{
		const u64 cpus = g_cpus.load();
		for (s32 cpu = 63; cpu >= 0; cpu--)
		{
			if ((cpus >> cpu & 1) && g_sibling[cpu] >= 0)
			{
				return {std::min<s32>(cpu, g_sibling[cpu]), std::max<s32>(cpu, g_sibling[cpu])};
			}
		}
		if (cpus)
		{
			const s32 last = 63 - std::countl_zero(cpus);
			return {last, last};
		}
		return {-1, -1};
	}
}

u64 ps5_cpu_kept_mask()
{
	if (!g_cfg.core.spu_own_core || !g_cpus.load())
	{
		return 0;
	}
	const auto [first, second] = kept_core();
	return first < 0 ? 0 : (1ull << first | 1ull << second);
}

s32 ps5_cpu_take_own_core(std::string_view thread)
{
	if (!ps5_cpu_kept_mask())
	{
		return -1;
	}
	const u64 run = static_cast<u64>(Emu.GetEmulationIdentifier());
	if (g_own_core_run.exchange(run) == run)
	{
		return -1;
	}
	const s32 cpu = kept_core().first;
	std::lock_guard lock(g_own_core_mutex);
	g_own_core_thread = thread;
	g_own_core_cpu = cpu;
	return cpu;
}

std::string ps5_cpu_own_core_text()
{
	if (!g_cfg.core.spu_own_core)
	{
		return "own core off";
	}
	if (!g_cpus.load())
	{
		return "own core on, CPUs not measured";
	}
	const auto [first, second] = kept_core();
	std::string text = first == second ? fmt::format("own core on: CPU %d, alone (%s)", first, g_layout) : fmt::format("own core on: CPU %d, %d kept free (%s)", first, second, g_layout);
	reader_lock lock(g_own_core_mutex);
	if (g_own_core_run == static_cast<u64>(Emu.GetEmulationIdentifier()) && !g_own_core_thread.empty())
	{
		fmt::append(text, ", for %s", g_own_core_thread);
	}
	else
	{
		text += ", no SPU took it";
	}
	return text;
}
