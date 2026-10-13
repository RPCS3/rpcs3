// PS5: the title's C++ allocations, counted by the thread that made them.
//
// GTA V ran the title out of memory in 20 minutes (build 138): the heap grew
// about 7 MiB a second while the game streamed the city in, and 2.6 GiB of it
// went back when the emulation stopped. Build 139's memory line ruled out the
// Vulkan driver's own allocations (about 120 MiB, flat), the texture cache,
// the render targets, the shader programs and pipelines (flat once loaded),
// the SPU code (about 50 functions added) and frame generation (off for the
// half of the run the heap grew most in). What is left is RPCS3's own C++
// memory, and this says whose: every operator new carries a 16-byte header
// with its size and the allocating thread's bucket (its name, numbers made
// '#', so the SPU kernels or the shader compiler's workers share one), and the
// buckets' live bytes go to the trace's memory line (ps5_frontend.cpp).
// libc++abi's operator new and delete are weak; these replace them. C's malloc
// (the Vulkan driver, NIR, FFmpeg, zstd) is not counted: the heap's total
// minus these is that.

#include "stdafx.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

namespace
{
	constexpr u16 c_magic = 0x5a35;
	constexpr u32 c_buckets = 64;

	struct alignas(16) block_header
	{
		u64 size;
		u32 offset; // from what malloc returned to the caller's memory
		u16 bucket;
		u16 magic;
	};
	static_assert(sizeof(block_header) == 16);

	struct alignas(64) bucket
	{
		std::atomic<s64> bytes;
		std::atomic<s64> blocks;
		char name[48];
	};

	// [0]: allocations made while a thread looked its name up; the last: the
	// threads past the others' count
	bucket g_buckets[c_buckets]{};
	std::atomic<u32> g_bucket_count{1};
	std::atomic<bool> g_bucket_lock{false};

	thread_local s32 t_bucket = -1;
	thread_local u32 t_since = 0;
	thread_local bool t_resolving = false;

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
		// Looked up again now and then: a thread can be renamed
		if (t_resolving)
		{
			return 0;
		}
		if (t_bucket < 0 || ++t_since >= 4096)
		{
			t_resolving = true;
			t_bucket = resolve_bucket();
			t_resolving = false;
			t_since = 0;
		}
		return static_cast<u16>(t_bucket);
	}

	void* track(void* base, usz offset, usz size)
	{
		u8* const memory = static_cast<u8*>(base) + offset;
		const u16 index = current_bucket();
		reinterpret_cast<block_header*>(memory)[-1] = {size, static_cast<u32>(offset), index, c_magic};
		g_buckets[index].bytes.fetch_add(static_cast<s64>(size), std::memory_order_relaxed);
		g_buckets[index].blocks.fetch_add(1, std::memory_order_relaxed);
		return memory;
	}

	void* allocate(usz size, usz align) noexcept
	{
		if (align <= alignof(block_header))
		{
			void* const base = std::malloc(size + sizeof(block_header));
			return base ? track(base, sizeof(block_header), size) : nullptr;
		}
		void* base = nullptr;
		if (posix_memalign(&base, align, align + size) != 0)
		{
			return nullptr;
		}
		return track(base, align, size);
	}

	void release(void* memory) noexcept
	{
		if (!memory)
		{
			return;
		}
		const block_header header = reinterpret_cast<block_header*>(memory)[-1];
		if (header.magic != c_magic || header.bucket >= c_buckets)
		{
			// Not one of ours (malloc's, handed to delete): as libc++abi would
			std::free(memory);
			return;
		}
		reinterpret_cast<block_header*>(memory)[-1].magic = 0;
		g_buckets[header.bucket].bytes.fetch_sub(static_cast<s64>(header.size), std::memory_order_relaxed);
		g_buckets[header.bucket].blocks.fetch_sub(1, std::memory_order_relaxed);
		std::free(static_cast<u8*>(memory) - header.offset);
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

// The buckets' live bytes, the most first: "rsx::thread 2104 MiB (812k blocks), ..."
std::string ps5_alloc_report(u32 top, s64* total)
{
	struct entry
	{
		s64 bytes;
		s64 blocks;
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
		const s64 bytes = g_buckets[i].bytes.load(std::memory_order_relaxed);
		sum += bytes;
		if (bytes > 0)
		{
			entries.push_back({bytes, g_buckets[i].blocks.load(std::memory_order_relaxed), i});
		}
	}
	std::sort(entries.begin(), entries.end(), [](const entry& a, const entry& b) { return a.bytes > b.bytes; });

	std::string result;
	for (usz i = 0; i < entries.size() && i < top; i++)
	{
		const char* const name = entries[i].index == 0 ? "a thread naming itself" : g_buckets[entries[i].index].name;
		fmt::append(result, "%s%s %d MiB (%dk blocks)", result.empty() ? "" : ", ", name, static_cast<s32>(entries[i].bytes >> 20), static_cast<s32>(entries[i].blocks / 1000));
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
