// PS5: the title's C++ allocations, sampled by the thread that made them.
//
// GTA V ran the title out of memory in 20 minutes (build 138): the heap grew
// about 7 MiB a second while the game streamed the city in, and 2.6 GiB of it
// went back when the emulation stopped. Build 139's memory line ruled out the
// Vulkan driver's own allocations (about 120 MiB, flat), the texture cache,
// the render targets, the shader programs and pipelines (flat once loaded),
// the SPU code (about 50 functions added) and frame generation (off for the
// half of the run the heap grew most in). What is left is RPCS3's own C++
// memory, and this says whose.
//
// operator new and delete replace libc++abi's weak ones, and hand out exactly
// what malloc does, as libc++abi's did: build 140 put a 16-byte header before
// each block and gave out the address past it, and the title no longer
// started (a black screen, on my console), most likely memory from new
// reaching free() somewhere, which then freed the middle of a block. Instead,
// an allocation whose address hashes to one in 256 is recorded in a table
// beside the heap (its address, size and the allocating thread's bucket), and
// its delete takes it out; the others cost one hash. Each bucket's sampled
// bytes times 256 estimate its live bytes. Nothing is sampled until the
// frontend says the title is up (ps5_alloc_track_start), so no thread-local
// or thread name is touched while the title starts. C's malloc (the Vulkan
// driver's C, NIR, FFmpeg, zstd) is not counted.

#include "stdafx.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

namespace
{
	constexpr u32 c_buckets = 64;
	constexpr u32 c_sample_bits = 8; // one address in 256
	constexpr u64 c_table_size = u64{1} << 20; // 16 MiB of records
	constexpr uptr c_tombstone = 1;

	struct alignas(64) bucket
	{
		std::atomic<s64> bytes;
		std::atomic<s64> blocks;
		char name[48];
	};

	struct record
	{
		std::atomic<uptr> address;
		u32 size;
		u16 bucket;
		u16 unused;
	};

	// [0]: the threads looking their name up; the last: the threads past the others' count
	bucket g_buckets[c_buckets]{};
	std::atomic<u32> g_bucket_count{1};
	std::atomic<bool> g_bucket_lock{false};
	std::atomic<bool> g_started{false};
	std::atomic<u64> g_table_full{0};
	record g_table[c_table_size]{};

	thread_local s32 t_bucket = -1;
	thread_local bool t_resolving = false;

	u64 address_hash(uptr address)
	{
		u64 x = static_cast<u64>(address) >> 4;
		x ^= x >> 33;
		x *= 0xff51afd7ed558ccdull;
		x ^= x >> 33;
		return x;
	}

	bool sampled(u64 hash)
	{
		return (hash >> (64 - c_sample_bits)) == 0;
	}

	// "SPU[0x1000100] Thread (PrimaryCellSpursKernel1)" -> "SPU[#] Thread (PrimaryCellSpursKernel#)"
	void normalise(const std::string& name, char (&out)[48])
	{
		usz at = 0;
		for (usz i = 0; i < name.size() && at + 1 < sizeof(out); i++)
		{
			const char c = name[i];
			const bool digit = c >= '0' && c <= '9';
			if (digit || (c == 'x' && i && name[i - 1] == '0'))
			{
				if (!at || out[at - 1] != '#')
				{
					out[at++] = '#';
				}
				continue;
			}
			if (at && out[at - 1] == '#' && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) && i && name[i - 1] != ' ')
			{
				continue; // a hex number's letters
			}
			out[at++] = c;
		}
		out[at] = '\0';
	}

	u16 resolve_bucket()
	{
		char key[48]{};
		normalise(thread_ctrl::get_name(), key);

		while (g_bucket_lock.exchange(true, std::memory_order_acquire))
		{
		}
		const u32 count = g_bucket_count.load(std::memory_order_relaxed);
		u16 found = 0;
		for (u32 i = 1; i < count; i++)
		{
			if (std::strcmp(g_buckets[i].name, key) == 0)
			{
				found = static_cast<u16>(i);
				break;
			}
		}
		if (!found)
		{
			if (count < c_buckets - 1)
			{
				std::memcpy(g_buckets[count].name, key, sizeof(key));
				g_bucket_count.store(count + 1, std::memory_order_release);
				found = static_cast<u16>(count);
			}
			else
			{
				std::strcpy(g_buckets[c_buckets - 1].name, "other threads");
				found = c_buckets - 1;
			}
		}
		g_bucket_lock.store(false, std::memory_order_release);
		return found;
	}

	u16 current_bucket()
	{
		// Looked up once, at the thread's first sampled block: a thread's
		// name is set before it runs (named_thread), and a later look could
		// meet its thread_base already destroyed as the thread ends
		// (thread_base::finalize leaves g_tls_this_thread set)
		if (t_resolving)
		{
			return 0;
		}
		if (t_bucket < 0)
		{
			t_resolving = true;
			t_bucket = resolve_bucket();
			t_resolving = false;
		}
		return static_cast<u16>(t_bucket);
	}

	void note_allocated(void* memory, usz size)
	{
		if (!memory || !g_started.load(std::memory_order_relaxed))
		{
			return;
		}
		const u64 hash = address_hash(reinterpret_cast<uptr>(memory));
		if (!sampled(hash))
		{
			return;
		}
		const u16 index = current_bucket();
		for (u64 probe = 0; probe < 64; probe++)
		{
			record& slot = g_table[(hash + probe) & (c_table_size - 1)];
			uptr seen = slot.address.load(std::memory_order_relaxed);
			if ((seen == 0 || seen == c_tombstone) && slot.address.compare_exchange_strong(seen, reinterpret_cast<uptr>(memory), std::memory_order_acquire))
			{
				slot.size = static_cast<u32>(std::min<usz>(size, u32{umax}));
				slot.bucket = index;
				g_buckets[index].bytes.fetch_add(static_cast<s64>(slot.size), std::memory_order_relaxed);
				g_buckets[index].blocks.fetch_add(1, std::memory_order_relaxed);
				return;
			}
		}
		g_table_full.fetch_add(1, std::memory_order_relaxed);
	}

	void note_freed(void* memory)
	{
		if (!memory)
		{
			return;
		}
		const u64 hash = address_hash(reinterpret_cast<uptr>(memory));
		if (!sampled(hash))
		{
			return;
		}
		for (u64 probe = 0; probe < 64; probe++)
		{
			record& slot = g_table[(hash + probe) & (c_table_size - 1)];
			const uptr seen = slot.address.load(std::memory_order_acquire);
			if (seen == 0)
			{
				return; // never recorded: allocated before sampling started
			}
			if (seen == reinterpret_cast<uptr>(memory))
			{
				const u32 size = slot.size;
				const u16 index = slot.bucket;
				slot.address.store(c_tombstone, std::memory_order_release);
				g_buckets[index].bytes.fetch_sub(static_cast<s64>(size), std::memory_order_relaxed);
				g_buckets[index].blocks.fetch_sub(1, std::memory_order_relaxed);
				return;
			}
		}
	}

	void* allocate(usz size, usz align) noexcept
	{
		void* memory = nullptr;
		if (align <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
		{
			memory = std::malloc(size);
		}
		else if (posix_memalign(&memory, align, size) != 0)
		{
			memory = nullptr;
		}
		note_allocated(memory, size);
		return memory;
	}

	void release(void* memory) noexcept
	{
		note_freed(memory);
		std::free(memory);
	}

	void* allocate_or_throw(usz size, usz align)
	{
		size = size ? size : 1;
		while (true)
		{
			if (void* const memory = allocate(size, align))
			{
				return memory;
			}
			if (const std::new_handler handler = std::get_new_handler())
			{
				handler();
				continue;
			}
			throw std::bad_alloc();
		}
	}
}

// Sampling starts: the title is up, its threads named (ps5_frontend.cpp)
void ps5_alloc_track_start()
{
	g_started.store(true, std::memory_order_release);
}

// The buckets' estimated live bytes, the most first: "rsx::thread 2104 MiB, ..."
std::string ps5_alloc_report(u32 top, s64* total)
{
	struct entry
	{
		s64 bytes;
		u32 index;
	};
	std::vector<entry> entries;
	s64 sum = 0;
	const u32 count = g_bucket_count.load(std::memory_order_acquire);
	for (u32 i = 0; i < c_buckets; i++)
	{
		if (i >= count && i != c_buckets - 1)
		{
			continue;
		}
		const s64 bytes = g_buckets[i].bytes.load(std::memory_order_relaxed) << c_sample_bits;
		sum += bytes;
		if (bytes > 0)
		{
			entries.push_back({bytes, i});
		}
	}
	std::sort(entries.begin(), entries.end(), [](const entry& a, const entry& b) { return a.bytes > b.bytes; });

	std::string result;
	for (usz i = 0; i < entries.size() && i < top; i++)
	{
		const char* const name = entries[i].index == 0 ? "a thread naming itself" : g_buckets[entries[i].index].name;
		fmt::append(result, "%s%s %d MiB", result.empty() ? "" : ", ", name, static_cast<s32>(entries[i].bytes >> 20));
	}
	if (const u64 full = g_table_full.load(std::memory_order_relaxed))
	{
		fmt::append(result, " (%u samples not kept: the table was full)", full);
	}
	if (total)
	{
		*total = sum;
	}
	return result;
}

void* operator new(usz size) { return allocate_or_throw(size, 0); }
void* operator new[](usz size) { return allocate_or_throw(size, 0); }
void* operator new(usz size, const std::nothrow_t&) noexcept { return allocate(size ? size : 1, 0); }
void* operator new[](usz size, const std::nothrow_t&) noexcept { return allocate(size ? size : 1, 0); }
void* operator new(usz size, std::align_val_t align) { return allocate_or_throw(size, static_cast<usz>(align)); }
void* operator new[](usz size, std::align_val_t align) { return allocate_or_throw(size, static_cast<usz>(align)); }
void* operator new(usz size, std::align_val_t align, const std::nothrow_t&) noexcept { return allocate(size ? size : 1, static_cast<usz>(align)); }
void* operator new[](usz size, std::align_val_t align, const std::nothrow_t&) noexcept { return allocate(size ? size : 1, static_cast<usz>(align)); }

void operator delete(void* memory) noexcept { release(memory); }
void operator delete[](void* memory) noexcept { release(memory); }
void operator delete(void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete(void* memory, usz) noexcept { release(memory); }
void operator delete[](void* memory, usz) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }
void operator delete(void* memory, usz, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, usz, std::align_val_t) noexcept { release(memory); }
