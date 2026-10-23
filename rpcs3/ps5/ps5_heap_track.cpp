// PS5: what the title's heap holds, by the thread and the code that allocated it.
// Copyright (c) 2026 KongaTime
// SPDX-License-Identifier: MIT
//
// An observer of the title's allocations (libre/heap.h's tracker, PS5_RPCS3Title's
// ps5/libre): the frontend's status lines say how much of the heap each thread's
// allocations hold ("memory by thread") and which code allocated it ("memory by
// place"), read against the link's llvm-pie.elf.
//
// It runs inside malloc and free, so it allocates nothing there and keeps nothing
// in thread-local storage (the console's compiler emulates it with malloc): its
// tables are made once, when recording starts. Every block of 1 MiB or more is
// recorded; of the smaller ones, one in 256, chosen by its address, and counted
// 256 times. free tests a block the same way, so for nearly every block it
// returns at once, with no lock.

#include <libre/heap.h>

#include <pthread.h>
#include <pthread_np.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
	constexpr std::size_t large_block = 1 << 20;
	constexpr std::uint64_t sample_one_in = 256;
	constexpr unsigned site_depth = 12;

	// The recorded blocks: open addressing, a power of two
	constexpr std::size_t record_slots = 1 << 17;
	// The threads that named themselves, by handle
	constexpr std::size_t thread_slots = 1 << 13;
	constexpr std::size_t name_count = 1024;
	constexpr std::size_t name_length = 48;
	// The places, gathered for a report
	constexpr std::size_t place_slots = 1 << 14;

	struct record
	{
		std::uintptr_t block; // 0: empty
		std::uint64_t size;
		std::uint16_t name;
		std::uint8_t depth;
		std::uintptr_t sites[site_depth];
	};

	struct thread_entry
	{
		std::atomic<std::uintptr_t> handle; // 0: never used; 1: ended
		std::atomic<std::uint16_t> name;
		std::uintptr_t stack_low;
		std::uintptr_t stack_high;
	};

	struct place
	{
		const record* first; // the record whose name and sites it stands for; null: empty
		std::uint64_t bytes, blocks, exact_bytes, exact_blocks;
	};

	std::mutex g_lock; // the records, and making the tables
	std::mutex g_threads_lock; // adding threads and names
	record* g_records;
	place* g_places;
	std::size_t g_record_count;
	std::uint64_t g_dropped;
	thread_entry g_threads[thread_slots];
	char g_names[name_count][name_length] = {"other"};
	std::size_t g_name_count = 1; // 0 is "other": threads that never named themselves
	std::atomic<bool> g_started;

	std::uint64_t mix(std::uint64_t value)
	{
		value ^= value >> 33;
		value *= 0xff51afd7ed558ccdull;
		value ^= value >> 33;
		value *= 0xc4ceb9fe1a85ec53ull;
		return value ^ (value >> 33);
	}

	bool recorded_kind(const void* block, std::size_t size)
	{
		return size >= large_block || mix(reinterpret_cast<std::uintptr_t>(block)) % sample_one_in == 0;
	}

	const thread_entry* find_thread(std::uintptr_t handle)
	{
		for (std::size_t i = mix(handle) % thread_slots, n = 0; n < thread_slots; i = (i + 1) % thread_slots, n++)
		{
			const std::uintptr_t at = g_threads[i].handle.load(std::memory_order_acquire);
			if (at == handle)
				return &g_threads[i];
			if (at == 0)
				return nullptr;
		}
		return nullptr;
	}

	// The return addresses above this frame, within the thread's own stack. Not
	// inlined, so the frames left out are always the same two: into allocated(),
	// and from it into the malloc family's wrap
	__attribute__((noinline)) unsigned walk(const thread_entry* thread, std::uintptr_t* sites)
	{
		if (!thread || !thread->stack_high)
			return 0;
		unsigned depth = 0;
		auto frame = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
		for (unsigned skip = 0; depth < site_depth;)
		{
			if (frame < thread->stack_low || frame + 16 > thread->stack_high || frame % 8)
				break;
			const auto* words = reinterpret_cast<const std::uintptr_t*>(frame);
			if (skip++ >= 2 && words[1])
				sites[depth++] = words[1];
			if (words[0] <= frame)
				break;
			frame = words[0];
		}
		return depth;
	}

	void allocated(void* block, std::size_t size)
	{
		if (!recorded_kind(block, size))
			return;
		const thread_entry* thread = find_thread(reinterpret_cast<std::uintptr_t>(pthread_self()));
		record entry{};
		entry.block = reinterpret_cast<std::uintptr_t>(block);
		entry.size = size;
		entry.name = thread ? thread->name.load(std::memory_order_relaxed) : 0;
		entry.depth = static_cast<std::uint8_t>(walk(thread, entry.sites));

		std::lock_guard lock(g_lock);
		if (g_record_count >= record_slots * 3 / 4)
		{
			g_dropped++;
			return;
		}
		for (std::size_t i = mix(entry.block) % record_slots;; i = (i + 1) % record_slots)
		{
			if (!g_records[i].block || g_records[i].block == entry.block)
			{
				if (!g_records[i].block)
					g_record_count++;
				g_records[i] = entry;
				return;
			}
		}
	}

	void freed(void* block)
	{
		if (!recorded_kind(block, libre_heap_usable_size(block)))
			return;
		const auto at = reinterpret_cast<std::uintptr_t>(block);
		std::lock_guard lock(g_lock);
		std::size_t i = mix(at) % record_slots;
		for (; g_records[i].block != at; i = (i + 1) % record_slots)
		{
			if (!g_records[i].block)
				return;
		}
		// Removed, and the records after it moved back where they belong
		g_records[i].block = 0;
		g_record_count--;
		for (std::size_t j = (i + 1) % record_slots; g_records[j].block; j = (j + 1) % record_slots)
		{
			const std::size_t home = mix(g_records[j].block) % record_slots;
			const bool between = i <= j ? (i < home && home <= j) : (i < home || home <= j);
			if (!between)
			{
				g_records[i] = g_records[j];
				g_records[j].block = 0;
				i = j;
			}
		}
	}

	const libre_heap_tracker g_tracker{allocated, freed};

	std::uint64_t weight(const record& entry)
	{
		return entry.size >= large_block ? 1 : sample_one_in;
	}

	std::string mib(std::uint64_t bytes)
	{
		return std::to_string(bytes >> 20) + " MiB";
	}

	std::string blocks_text(std::uint64_t bytes, std::uint64_t blocks, std::uint64_t exact_bytes, std::uint64_t exact_blocks)
	{
		return mib(bytes) + " in about " + std::to_string(blocks) + " blocks (" + mib(exact_bytes) + " in " +
			std::to_string(exact_blocks) + " of 1 MiB or more)";
	}
}

// Recording starts: the tables are made, then the heap tells of its blocks
void ps5_alloc_track_start()
{
	if (g_started.exchange(true))
		return;
	auto* records = static_cast<record*>(std::calloc(record_slots, sizeof(record)));
	auto* places = static_cast<place*>(std::calloc(place_slots, sizeof(place)));
	if (!records || !places)
	{
		std::free(records);
		std::free(places);
		return;
	}
	{
		std::lock_guard lock(g_lock);
		g_records = records;
		g_places = places;
	}
	libre_heap_set_tracker(&g_tracker);
}

// This thread's name for its allocations (nullptr: the thread ends)
void ps5_alloc_track_name(const char* name)
{
	const auto handle = reinterpret_cast<std::uintptr_t>(pthread_self());
	std::lock_guard lock(g_threads_lock);

	if (!name)
	{
		for (std::size_t i = mix(handle) % thread_slots, n = 0; n < thread_slots; i = (i + 1) % thread_slots, n++)
		{
			const std::uintptr_t at = g_threads[i].handle.load(std::memory_order_relaxed);
			if (at == handle)
			{
				g_threads[i].handle.store(1, std::memory_order_release);
				return;
			}
			if (at == 0)
				return;
		}
		return;
	}

	// The name, kept once
	std::uint16_t index = 0;
	for (std::size_t i = 1; i < g_name_count; i++)
	{
		if (std::strncmp(g_names[i], name, name_length - 1) == 0)
		{
			index = static_cast<std::uint16_t>(i);
			break;
		}
	}
	if (!index && g_name_count < name_count)
	{
		std::snprintf(g_names[g_name_count], name_length, "%s", name);
		index = static_cast<std::uint16_t>(g_name_count++);
	}

	// The thread's stack, within which its frames are read
	void* stack = nullptr;
	std::size_t stack_size = 0;
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) == 0)
	{
		if (pthread_attr_get_np(pthread_self(), &attr) == 0)
		{
			pthread_attr_getstackaddr(&attr, &stack);
			pthread_attr_getstacksize(&attr, &stack_size);
		}
		pthread_attr_destroy(&attr);
	}

	// Its entry: its own if it has one, else the first free or ended one
	thread_entry* free_entry = nullptr;
	for (std::size_t i = mix(handle) % thread_slots, n = 0; n < thread_slots; i = (i + 1) % thread_slots, n++)
	{
		const std::uintptr_t at = g_threads[i].handle.load(std::memory_order_relaxed);
		if (at == handle)
		{
			free_entry = &g_threads[i];
			break;
		}
		if (at == 1 && !free_entry)
			free_entry = &g_threads[i];
		if (at == 0)
		{
			if (!free_entry)
				free_entry = &g_threads[i];
			break;
		}
	}
	if (!free_entry)
		return;
	// Ended while it changes (never 0, which would end other threads' searches)
	free_entry->handle.store(1, std::memory_order_release);
	free_entry->stack_low = reinterpret_cast<std::uintptr_t>(stack);
	free_entry->stack_high = stack ? reinterpret_cast<std::uintptr_t>(stack) + stack_size : 0;
	free_entry->name.store(index, std::memory_order_relaxed);
	free_entry->handle.store(handle, std::memory_order_release);
}

// The live blocks by the thread that allocated them, the most first: about how
// much in all (*total), and how much in blocks of 1 MiB or more (*exact_total)
std::string ps5_alloc_report(std::uint32_t top, std::int64_t* total, std::int64_t* exact_total)
{
	struct owner
	{
		std::uint64_t bytes, blocks, exact_bytes, exact_blocks;
	};
	// Made before the lock: the report's own allocations come to allocated()
	std::vector<owner> owners(name_count);
	std::uint64_t all = 0, exact = 0;
	{
		std::lock_guard lock(g_lock);
		for (std::size_t i = 0; g_records && i < record_slots; i++)
		{
			const record& entry = g_records[i];
			if (!entry.block)
				continue;
			owner& to = owners[entry.name];
			to.bytes += entry.size * weight(entry);
			to.blocks += weight(entry);
			if (entry.size >= large_block)
			{
				to.exact_bytes += entry.size;
				to.exact_blocks++;
			}
		}
	}

	std::vector<std::uint16_t> order;
	for (std::size_t i = 0; i < name_count; i++)
	{
		if (owners[i].blocks)
		{
			order.push_back(static_cast<std::uint16_t>(i));
			all += owners[i].bytes;
			exact += owners[i].exact_bytes;
		}
	}
	std::sort(order.begin(), order.end(), [&](std::uint16_t a, std::uint16_t b) { return owners[a].bytes > owners[b].bytes; });

	std::string text;
	for (std::size_t i = 0; i < order.size() && i < top; i++)
	{
		const owner& entry = owners[order[i]];
		if (!text.empty())
			text += ", ";
		text += g_names[order[i]];
		text += " ";
		text += blocks_text(entry.bytes, entry.blocks, entry.exact_bytes, entry.exact_blocks);
	}
	if (total)
		*total = static_cast<std::int64_t>(all);
	if (exact_total)
		*exact_total = static_cast<std::int64_t>(exact);
	return text.empty() ? std::string("none recorded") : text;
}

// The live blocks by the code that allocated them (the thread and the return
// addresses, innermost first), the most first. The addresses are where the code
// runs: less ps5_alloc_sites' own address here, plus its address in llvm-pie.elf,
// they are the link's
std::string ps5_alloc_sites(std::uint32_t top)
{
	struct found
	{
		place totals;
		std::uint16_t name;
		std::uint8_t depth;
		std::uintptr_t sites[site_depth];
	};
	std::vector<found> places;
	places.reserve(top);
	std::size_t place_count = 0;
	{
		std::lock_guard lock(g_lock);
		if (!g_records || !g_places)
			return "not recording";
		std::memset(g_places, 0, place_slots * sizeof(place));
		for (std::size_t i = 0; i < record_slots; i++)
		{
			const record& entry = g_records[i];
			if (!entry.block)
				continue;
			std::uint64_t key = mix(entry.name);
			for (unsigned d = 0; d < entry.depth; d++)
				key = mix(key ^ entry.sites[d]);
			for (std::size_t j = key % place_slots, n = 0; n < place_slots; j = (j + 1) % place_slots, n++)
			{
				place& to = g_places[j];
				const bool same = to.first && to.first->name == entry.name && to.first->depth == entry.depth &&
					std::equal(entry.sites, entry.sites + entry.depth, to.first->sites);
				if (!to.first)
				{
					to.first = &entry;
					place_count++;
				}
				else if (!same)
					continue;
				to.bytes += entry.size * weight(entry);
				to.blocks += weight(entry);
				if (entry.size >= large_block)
				{
					to.exact_bytes += entry.size;
					to.exact_blocks++;
				}
				break;
			}
		}
		// The largest top, kept in order (the vector's room was made before the lock)
		for (std::size_t j = 0; j < place_slots; j++)
		{
			const place& at = g_places[j];
			if (!at.first)
				continue;
			if (places.size() == top && (top == 0 || places.back().totals.bytes >= at.bytes))
				continue;
			if (places.size() == top)
				places.pop_back();
			found entry{at, at.first->name, at.first->depth, {}};
			std::copy(at.first->sites, at.first->sites + at.first->depth, entry.sites);
			places.insert(std::find_if(places.begin(), places.end(), [&](const found& f) { return f.totals.bytes < at.bytes; }), entry);
		}
	}

	char head[96];
	std::snprintf(head, sizeof(head), "ps5_alloc_sites at %016zx, %zu places",
		reinterpret_cast<std::size_t>(&ps5_alloc_sites), place_count);
	std::string text = head;
	for (const found& entry : places)
	{
		text += "; ";
		text += blocks_text(entry.totals.bytes, entry.totals.blocks, entry.totals.exact_bytes, entry.totals.exact_blocks);
		text += " by ";
		text += g_names[entry.name];
		text += " at";
		for (unsigned d = 0; d < entry.depth; d++)
		{
			char address[24];
			std::snprintf(address, sizeof(address), " %zx", static_cast<std::size_t>(entry.sites[d]));
			text += address;
		}
	}
	if (g_dropped)
		text += "; " + std::to_string(g_dropped) + " blocks not recorded (the table was full)";
	return text;
}
