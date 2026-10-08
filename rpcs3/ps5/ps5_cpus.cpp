// PS5: the console's CPUs as a title gets them (ps5_cpus.h)

#include "stdafx.h"
#include "ps5_cpus.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/Thread.h"

#include <pthread.h>

#include <algorithm>
#include <atomic>
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
		std::atomic<bool> placed = true;
		u64 counts[2]{};

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
			// Eight independent products an iteration keep the core's
			// multiplier busy, so its other thread's loop takes half of it.
			// One chain of products waits on each result, leaves the
			// multiplier mostly idle, and kept nearly all its speed beside its
			// sibling: build 100 found no shared core that way (as
			// PS5_PayloadSDK's topology probe says, src/probe_threads.c)
			const auto end = std::chrono::steady_clock::now() + std::chrono::duration<f64, std::milli>(ms);
			u64 n = 0;
			u64 v[8]{1, 2, 3, 4, 5, 6, 7, 8};
			do
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
			while (std::chrono::steady_clock::now() < end);
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
			if (kept < 0.75)
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
