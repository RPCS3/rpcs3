// PS5: the title's allocations, counted by the thread that made them.
//
// GTA V ran the title out of memory in 20 minutes (build 138): the heap grew
// about 7 MiB a second while the game streamed the city in, and 2.6 GiB of it
// went back when the emulation stopped. Build 139's memory line ruled out the
// Vulkan driver's own allocations (about 120 MiB, flat), the texture cache,
// the render targets, the shader programs and pipelines (flat once loaded),
// the SPU code (about 50 functions added) and frame generation. Build 141
// sampled operator new by thread: C++ held about 820 MiB, flat, while the
// heap went from 2.7 to 8.6 GiB and the title died of std::bad_alloc in
// rsx::simple_array (malloc). So the growth is malloc's, or blocks too large
// and too few for one-in-256 sampling to see.
//
// So the heap's own wraps are here (the platform's src/heap_wrap.c, the same
// logic): the link's --wrap sends every malloc, calloc, realloc, free and
// aligned allocation of the title (RPCS3, the Vulkan driver, libc++abi's
// operator new) to them, and the platform's are then never pulled from the
// archive. A block of 1 MiB or more is recorded exactly; a smaller one is
// recorded when its address hashes to one in 256, and counts 256 times. Each
// record holds the block's size and the bucket of the thread that allocated
// it: the thread's name, digits folded ("SPU[#] Thread (...)"), registered by
// thread_base::set_name in the thread itself (Utilities/Thread.cpp) and
// dropped when it ends. Nothing here uses thread_local: the console's TLS is
// emulated, and its first use in a thread allocates, inside this allocator.
// Threads RPCS3 did not name (the driver's, libc++'s) share bucket 0. Nothing
// is recorded until the frontend says the title is up (ps5_alloc_track_start).

#include "stdafx.h"

#include <ps5platform/heap.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <pthread.h>

extern "C"
{
	// The platform's counter of allocations libc served (src/heap.c)
	void ps5p_heap_count_fallback(void);

	// libc's allocator, through the linker's --wrap. malloc_usable_size is
	// weak: a title that does not reference it links without its wrap
	void* __real_malloc(size_t bytes);
	void* __real_calloc(size_t count, size_t bytes);
	void* __real_realloc(void* pointer, size_t bytes);
	void __real_free(void* pointer);
	int __real_posix_memalign(void** out, size_t alignment, size_t bytes);
	__attribute__((weak)) size_t __real_malloc_usable_size(const void* pointer);
}

namespace
{
	constexpr u32 c_buckets = 64;
	constexpr u32 c_sample_bits = 8; // one address in 256
	constexpr usz c_exact = usz{1} << 20; // blocks this large are all recorded
	constexpr u64 c_table_size = u64{1} << 20; // 16 MiB of records
	constexpr u32 c_threads = 1024;
	constexpr uptr c_tombstone = 1;

	struct alignas(64) bucket
	{
		std::atomic<s64> sampled_bytes;
		std::atomic<s64> sampled_blocks;
		std::atomic<s64> exact_bytes;
		std::atomic<s64> exact_blocks;
		char name[48];
	};

	struct record
	{
		std::atomic<uptr> address;
		u32 size;
		u16 bucket;
		u16 exact;
	};

	struct thread_slot
	{
		std::atomic<uptr> key;
		std::atomic<u16> bucket;
	};

	// [0]: threads RPCS3 did not name; the last: threads past the others' count
	bucket g_buckets[c_buckets]{};
	std::atomic<u32> g_bucket_count{1};
	std::atomic<bool> g_bucket_lock{false};
	std::atomic<bool> g_started{false};
	std::atomic<u64> g_table_full{0};
	record g_table[c_table_size]{};
	thread_slot g_threads[c_threads]{};

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

	uptr self_key()
	{
		return reinterpret_cast<uptr>(pthread_self());
	}

	// "SPU[0x1000100] Thread (PrimaryCellSpursKernel1)" -> "SPU[#] Thread (PrimaryCellSpursKernel#)"
	void normalise(const char* name, char (&out)[48])
	{
		usz at = 0;
		for (usz i = 0; name[i] && at + 1 < sizeof(out); i++)
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

	u16 bucket_for(const char* name)
	{
		char key[48]{};
		normalise(name, key);

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
		const uptr key = self_key();
		const u64 hash = address_hash(key);
		for (u32 probe = 0; probe < 64; probe++)
		{
			thread_slot& slot = g_threads[(hash + probe) % c_threads];
			const uptr seen = slot.key.load(std::memory_order_acquire);
			if (seen == 0)
			{
				break;
			}
			if (seen == key)
			{
				return slot.bucket.load(std::memory_order_relaxed);
			}
		}
		return 0;
	}

	// The block's size as the heap holds it, and whether it is recorded exactly
	usz block_size(void* memory, usz requested, bool& exact)
	{
		const usz size = ps5_heap_owns(memory) ? ps5_heap_usable_size(memory) : requested;
		exact = size >= c_exact;
		return size;
	}

	void note_allocated(void* memory, usz requested)
	{
		if (!memory || !g_started.load(std::memory_order_relaxed))
		{
			return;
		}
		const u64 hash = address_hash(reinterpret_cast<uptr>(memory));
		bool exact = false;
		const usz size = block_size(memory, requested, exact);
		if (!exact && !sampled(hash))
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
				slot.exact = exact;
				bucket& b = g_buckets[index];
				(exact ? b.exact_bytes : b.sampled_bytes).fetch_add(static_cast<s64>(slot.size), std::memory_order_relaxed);
				(exact ? b.exact_blocks : b.sampled_blocks).fetch_add(1, std::memory_order_relaxed);
				return;
			}
		}
		g_table_full.fetch_add(1, std::memory_order_relaxed);
	}

	// Before the block is freed or moved: another thread may be given its address right after
	void note_freed(void* memory)
	{
		if (!memory)
		{
			return;
		}
		const u64 hash = address_hash(reinterpret_cast<uptr>(memory));
		bool exact = false;
		block_size(memory, 0, exact);
		if (!exact && !sampled(hash))
		{
			return;
		}
		for (u64 probe = 0; probe < 64; probe++)
		{
			record& slot = g_table[(hash + probe) & (c_table_size - 1)];
			const uptr seen = slot.address.load(std::memory_order_acquire);
			if (seen == 0)
			{
				return; // never recorded: allocated before recording started
			}
			if (seen == reinterpret_cast<uptr>(memory))
			{
				const u32 size = slot.size;
				const u16 index = slot.bucket;
				const bool was_exact = slot.exact;
				slot.address.store(c_tombstone, std::memory_order_release);
				bucket& b = g_buckets[index];
				(was_exact ? b.exact_bytes : b.sampled_bytes).fetch_sub(static_cast<s64>(size), std::memory_order_relaxed);
				(was_exact ? b.exact_blocks : b.sampled_blocks).fetch_sub(1, std::memory_order_relaxed);
				return;
			}
		}
	}
}

// The thread names itself (thread_base::set_name), or ends (nullptr)
void ps5_alloc_track_name(const char* name)
{
	const uptr key = self_key();
	const u64 hash = address_hash(key);
	const u16 index = name ? bucket_for(name) : 0;
	for (u32 probe = 0; probe < 64; probe++)
	{
		thread_slot& slot = g_threads[(hash + probe) % c_threads];
		const uptr seen = slot.key.load(std::memory_order_acquire);
		if (seen == key)
		{
			if (name)
			{
				slot.bucket.store(index, std::memory_order_relaxed);
			}
			else
			{
				slot.key.store(c_tombstone, std::memory_order_release);
			}
			return;
		}
		if (seen == 0)
		{
			break;
		}
	}
	if (!name)
	{
		return;
	}
	// Only this thread writes its own key; a slot another thread takes first is passed over
	for (u32 probe = 0; probe < 64; probe++)
	{
		thread_slot& slot = g_threads[(hash + probe) % c_threads];
		uptr seen = slot.key.load(std::memory_order_relaxed);
		if (seen != 0 && seen != c_tombstone)
		{
			continue;
		}
		slot.bucket.store(index, std::memory_order_relaxed);
		if (slot.key.compare_exchange_strong(seen, key, std::memory_order_release))
		{
			return;
		}
	}
}

// Recording starts: the title is up (ps5_frontend.cpp)
void ps5_alloc_track_start()
{
	g_started.store(true, std::memory_order_release);
}

// The buckets' live bytes, the most first: "rsx::thread 2104 MiB in about 81000 blocks (...)"
std::string ps5_alloc_report(u32 top, s64* total, s64* exact_total)
{
	struct entry
	{
		s64 bytes;
		s64 blocks;
		u32 index;
	};
	std::vector<entry> entries;
	s64 sum = 0;
	s64 exact_sum = 0;
	const u32 count = g_bucket_count.load(std::memory_order_acquire);
	for (u32 i = 0; i < c_buckets; i++)
	{
		if (i >= count && i != c_buckets - 1)
		{
			continue;
		}
		const bucket& b = g_buckets[i];
		const s64 exact = b.exact_bytes.load(std::memory_order_relaxed);
		const s64 bytes = (b.sampled_bytes.load(std::memory_order_relaxed) << c_sample_bits) + exact;
		const s64 blocks = (b.sampled_blocks.load(std::memory_order_relaxed) << c_sample_bits) + b.exact_blocks.load(std::memory_order_relaxed);
		sum += bytes;
		exact_sum += exact;
		if (bytes > 0)
		{
			entries.push_back({bytes, blocks, i});
		}
	}
	std::sort(entries.begin(), entries.end(), [](const entry& a, const entry& b) { return a.bytes > b.bytes; });

	std::string result;
	for (usz i = 0; i < entries.size() && i < top; i++)
	{
		const bucket& b = g_buckets[entries[i].index];
		const char* const name = entries[i].index == 0 ? "threads RPCS3 did not name" : b.name;
		fmt::append(result, "%s%s %d MiB in about %d blocks (%d MiB in %d of 1 MiB or more)", result.empty() ? "" : ", ", name,
			static_cast<s32>(entries[i].bytes >> 20), static_cast<s32>(entries[i].blocks),
			static_cast<s32>(b.exact_bytes.load(std::memory_order_relaxed) >> 20), static_cast<s32>(b.exact_blocks.load(std::memory_order_relaxed)));
	}
	if (const u64 full = g_table_full.load(std::memory_order_relaxed))
	{
		fmt::append(result, " (%u blocks not recorded: the table was full)", full);
	}
	if (total)
	{
		*total = sum;
	}
	if (exact_total)
	{
		*exact_total = exact_sum;
	}
	return result;
}

// The platform's wraps (src/heap_wrap.c in the SDK fork), recording
extern "C" void* __wrap_malloc(size_t bytes)
{
	void* pointer = ps5_heap_malloc(bytes);
	if (!pointer)
	{
		ps5p_heap_count_fallback();
		pointer = __real_malloc(bytes);
	}
	note_allocated(pointer, bytes);
	return pointer;
}

extern "C" void* __wrap_calloc(size_t count, size_t bytes)
{
	void* pointer = ps5_heap_calloc(count, bytes);
	if (!pointer)
	{
		ps5p_heap_count_fallback();
		pointer = __real_calloc(count, bytes);
	}
	note_allocated(pointer, count * bytes);
	return pointer;
}

extern "C" void __wrap_free(void* pointer)
{
	if (!pointer)
	{
		return;
	}
	note_freed(pointer);
	if (ps5_heap_owns(pointer))
	{
		ps5_heap_free(pointer);
	}
	else
	{
		__real_free(pointer);
	}
}

extern "C" size_t __wrap_malloc_usable_size(const void* pointer)
{
	if (!pointer)
	{
		return 0;
	}
	if (ps5_heap_owns(pointer))
	{
		return ps5_heap_usable_size(pointer);
	}
	return __real_malloc_usable_size ? __real_malloc_usable_size(pointer) : 0;
}

extern "C" void* __wrap_realloc(void* pointer, size_t bytes)
{
	if (!pointer)
	{
		return __wrap_malloc(bytes);
	}
	// FreeBSD's realloc gives a zero-size request a minimum-size object
	if (bytes == 0)
	{
		bytes = 1;
	}
	note_freed(pointer);
	void* result = nullptr;
	if (ps5_heap_owns(pointer))
	{
		result = ps5_heap_realloc(pointer, bytes);
		if (!result)
		{
			// Direct memory is exhausted: move the block to libc
			result = __real_malloc(bytes);
			if (result)
			{
				ps5p_heap_count_fallback();
				const size_t kept = ps5_heap_usable_size(pointer);
				std::memcpy(result, pointer, kept < bytes ? kept : bytes);
				ps5_heap_free(pointer);
			}
		}
	}
	else
	{
		result = __real_realloc(pointer, bytes);
		if (!result && __real_malloc_usable_size)
		{
			// libc's heap is exhausted: move its block to the title heap
			result = ps5_heap_malloc(bytes);
			if (result)
			{
				const size_t kept = __real_malloc_usable_size(pointer);
				std::memcpy(result, pointer, kept < bytes ? kept : bytes);
				__real_free(pointer);
			}
		}
	}
	// A failed realloc leaves the block as it was, unrecorded now: an undercount, never a crash
	note_allocated(result, bytes);
	return result;
}

extern "C" void* __wrap_reallocf(void* pointer, size_t bytes)
{
	void* const moved = __wrap_realloc(pointer, bytes);
	if (!moved)
	{
		__wrap_free(pointer);
	}
	return moved;
}

extern "C" void* __wrap_reallocarray(void* pointer, size_t count, size_t bytes)
{
	if (bytes != 0 && count > SIZE_MAX / bytes)
	{
		errno = ENOMEM;
		return nullptr;
	}
	return __wrap_realloc(pointer, count * bytes);
}

extern "C" int __wrap_posix_memalign(void** out, size_t alignment, size_t bytes)
{
	if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0)
	{
		return EINVAL;
	}
	if (void* const pointer = ps5_heap_memalign(alignment, bytes))
	{
		*out = pointer;
		note_allocated(pointer, bytes);
		return 0;
	}
	ps5p_heap_count_fallback();
	const int result = __real_posix_memalign(out, alignment, bytes);
	if (result == 0)
	{
		note_allocated(*out, bytes);
	}
	return result;
}

extern "C" void* __wrap_aligned_alloc(size_t alignment, size_t bytes)
{
	void* pointer = nullptr;
	const int result = __wrap_posix_memalign(&pointer, alignment < sizeof(void*) ? sizeof(void*) : alignment, bytes);
	if (result != 0)
	{
		errno = result;
		return nullptr;
	}
	return pointer;
}

extern "C" void* __wrap_memalign(size_t alignment, size_t bytes)
{
	return __wrap_aligned_alloc(alignment, bytes);
}

// libc's getdelim grows the caller's buffer with libc's realloc, which must
// never see a title-heap block: this one grows it with the wrap
extern "C" ssize_t __wrap_getdelim(char** line, size_t* capacity, int delimiter, FILE* stream)
{
	if (!line || !capacity || !stream)
	{
		errno = EINVAL;
		return -1;
	}
	if (!*line)
	{
		*capacity = 0;
	}
	size_t length = 0;
	for (;;)
	{
		const int c = std::fgetc(stream);
		if (c == EOF)
		{
			if (length == 0 || std::ferror(stream))
			{
				return -1;
			}
			break;
		}
		if (length + 2 > *capacity)
		{
			const size_t grown = *capacity < 64 ? 128 : *capacity * 2;
			if (grown > static_cast<size_t>(SSIZE_MAX))
			{
				errno = EOVERFLOW;
				return -1;
			}
			char* const moved = static_cast<char*>(__wrap_realloc(*line, grown));
			if (!moved)
			{
				return -1;
			}
			*line = moved;
			*capacity = grown;
		}
		(*line)[length++] = static_cast<char>(c);
		if (c == static_cast<unsigned char>(delimiter))
		{
			break;
		}
	}
	(*line)[length] = '\0';
	return static_cast<ssize_t>(length);
}

extern "C" ssize_t __wrap_getline(char** line, size_t* capacity, FILE* stream)
{
	return __wrap_getdelim(line, capacity, '\n', stream);
}
