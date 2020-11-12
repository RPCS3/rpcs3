#include "atomic.hpp"

#if defined(__linux__)
#define USE_FUTEX
#elif !defined(_WIN32)
#define USE_STD
#endif

#include "Utilities/sync.h"

#include <utility>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iterator>
#include <memory>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <random>

#include "asm.hpp"
#include "endian.hpp"

// Total number of entries, should be a power of 2.
static constexpr std::size_t s_hashtable_size = 1u << 17;

// Reference counter combined with shifted pointer (which is assumed to be 47 bit)
static constexpr std::uintptr_t s_ref_mask = (1u << 17) - 1;

// Fix for silly on-first-use initializer
static constexpr auto s_null_wait_cb = [](const void*){ return true; };

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data) = s_null_wait_cb;

// Fix for silly on-first-use initializer
static constexpr auto s_null_notify_cb = [](const void*, u64){};

// Callback for notification functions for optimizations
static thread_local void(*s_tls_notify_cb)(const void* data, u64 progress) = s_null_notify_cb;

static inline bool operator &(atomic_wait::op lhs, atomic_wait::op_flag rhs)
{
	return !!(static_cast<u8>(lhs) & static_cast<u8>(rhs));
}

// Compare data in memory with old value, and return true if they are equal
template <bool CheckCb = true>
static NEVER_INLINE bool
#ifdef _WIN32
__vectorcall
#endif
ptr_cmp(const void* data, u32 _size, __m128i old128, __m128i mask128, atomic_wait::info* ext = nullptr)
{
	if constexpr (CheckCb)
	{
		if (!s_tls_wait_cb(data))
		{
			return false;
		}
	}

	using atomic_wait::op;
	using atomic_wait::op_flag;

	const u8 size = static_cast<u8>(_size);
	const op flag{static_cast<u8>(_size >> 8)};

	bool result = false;

	if (size <= 8)
	{
		u64 new_value = 0;
		u64 old_value = _mm_cvtsi128_si64(old128);
		u64 mask = _mm_cvtsi128_si64(mask128) & (UINT64_MAX >> ((64 - size * 8) & 63));

		// Don't load memory on empty mask
		switch (mask ? size : 0)
		{
		case 0: break;
		case 1: new_value = reinterpret_cast<const atomic_t<u8>*>(data)->load(); break;
		case 2: new_value = reinterpret_cast<const atomic_t<u16>*>(data)->load(); break;
		case 4: new_value = reinterpret_cast<const atomic_t<u32>*>(data)->load(); break;
		case 8: new_value = reinterpret_cast<const atomic_t<u64>*>(data)->load(); break;
		default:
		{
			fprintf(stderr, "ptr_cmp(): bad size (arg=0x%x)" HERE "\n", _size);
			std::abort();
		}
		}

		if (flag & op_flag::bit_not)
		{
			new_value = ~new_value;
		}

		if (!mask) [[unlikely]]
		{
			new_value = 0;
			old_value = 0;
		}
		else
		{
			if (flag & op_flag::byteswap)
			{
				switch (size)
				{
				case 2:
				{
					new_value = stx::se_storage<u16>::swap(static_cast<u16>(new_value));
					old_value = stx::se_storage<u16>::swap(static_cast<u16>(old_value));
					mask = stx::se_storage<u16>::swap(static_cast<u16>(mask));
					break;
				}
				case 4:
				{
					new_value = stx::se_storage<u32>::swap(static_cast<u32>(new_value));
					old_value = stx::se_storage<u32>::swap(static_cast<u32>(old_value));
					mask = stx::se_storage<u32>::swap(static_cast<u32>(mask));
					break;
				}
				case 8:
				{
					new_value = stx::se_storage<u64>::swap(new_value);
					old_value = stx::se_storage<u64>::swap(old_value);
					mask = stx::se_storage<u64>::swap(mask);
				}
				default:
				{
					break;
				}
				}
			}

			// Make most significant bit sign bit
			const auto shv = std::countl_zero(mask);
			new_value &= mask;
			old_value &= mask;
			new_value <<= shv;
			old_value <<= shv;
		}

		s64 news = new_value;
		s64 olds = old_value;

		u64 newa = news < 0 ? (0ull - new_value) : new_value;
		u64 olda = olds < 0 ? (0ull - old_value) : old_value;

		switch (op{static_cast<u8>(static_cast<u8>(flag) & 0xf)})
		{
		case op::eq: result = old_value == new_value; break;
		case op::slt: result = olds < news; break;
		case op::sgt: result = olds > news; break;
		case op::ult: result = old_value < new_value; break;
		case op::ugt: result = old_value > new_value; break;
		case op::alt: result = olda < newa; break;
		case op::agt: result = olda > newa; break;
		case op::pop:
		{
			// Count is taken from least significant byte and ignores some flags
			const u64 count = _mm_cvtsi128_si64(old128) & 0xff;

			u64 bitc = new_value;
			bitc = (bitc & 0xaaaaaaaaaaaaaaaa) / 2 + (bitc & 0x5555555555555555);
			bitc = (bitc & 0xcccccccccccccccc) / 4 + (bitc & 0x3333333333333333);
			bitc = (bitc & 0xf0f0f0f0f0f0f0f0) / 16 + (bitc & 0x0f0f0f0f0f0f0f0f);
			bitc = (bitc & 0xff00ff00ff00ff00) / 256 + (bitc & 0x00ff00ff00ff00ff);
			bitc = ((bitc & 0xffff0000ffff0000) >> 16) + (bitc & 0x0000ffff0000ffff);
			bitc = (bitc >> 32) + bitc;

			result = count < bitc;
			break;
		}
		default:
		{
			fmt::raw_error("ptr_cmp(): unrecognized atomic wait operation.");
		}
		}
	}
	else if (size == 16 && (flag == op::eq || flag == (op::eq | op_flag::inverse)))
	{
		u128 new_value = 0;
		u128 old_value = std::bit_cast<u128>(old128);
		u128 mask = std::bit_cast<u128>(mask128);

		// Don't load memory on empty mask
		if (mask) [[likely]]
		{
			new_value = atomic_storage<u128>::load(*reinterpret_cast<const u128*>(data));
		}

		// TODO
		result = !((old_value ^ new_value) & mask);
	}
	else if (size == 16)
	{
		fmt::raw_error("ptr_cmp(): no alternative operations are supported for 16-byte atomic wait yet.");
	}

	if (flag & op_flag::inverse)
	{
		result = !result;
	}

	// Check other wait variables if provided
	if (result)
	{
		if (ext) [[unlikely]]
		{
			for (auto e = ext; e->data; e++)
			{
				if (!ptr_cmp<false>(e->data, e->size, e->old, e->mask))
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

// Returns true if mask overlaps, or the argument is invalid
static bool
#ifdef _WIN32
__vectorcall
#endif
cmp_mask(u32 size1, __m128i mask1, __m128i val1, u32 size2, __m128i mask2, __m128i val2)
{
	// Compare only masks, new value is not available in this mode
	if (size1 == umax)
	{
		// Simple mask overlap
		const auto v0 = _mm_and_si128(mask1, mask2);
		const auto v1 = _mm_packs_epi16(v0, v0);
		return !!_mm_cvtsi128_si64(v1);
	}

	// Generate masked value inequality bits
	const auto v0 = _mm_and_si128(_mm_and_si128(mask1, mask2), _mm_xor_si128(val1, val2));

	using atomic_wait::op;
	using atomic_wait::op_flag;

	const u8 size = std::min<u8>(static_cast<u8>(size2), static_cast<u8>(size1));
	const op flag{static_cast<u8>(size2 >> 8)};

	if (flag != op::eq && flag != (op::eq | op_flag::inverse))
	{
		fmt::raw_error("cmp_mask(): no operations are supported for notification with forced value yet.");
	}

	if (size <= 8)
	{
		// Generate sized mask
		const u64 mask = UINT64_MAX >> ((64 - size * 8) & 63);

		if (!(_mm_cvtsi128_si64(v0) & mask))
		{
			return !!(flag & op_flag::inverse);
		}
	}
	else if (size == 16)
	{
		if (!_mm_cvtsi128_si64(_mm_packs_epi16(v0, v0)))
		{
			return !!(flag & op_flag::inverse);
		}
	}
	else
	{
		fprintf(stderr, "cmp_mask(): bad size (size1=%u, size2=%u)" HERE "\n", size1, size2);
		std::abort();
	}

	return !(flag & op_flag::inverse);
}

static atomic_t<u64> s_min_tsc{0};

namespace atomic_wait
{
#ifdef USE_STD
	// Just madness to keep some members uninitialized and get zero initialization otherwise
	template <typename T>
	struct un_t
	{
		alignas(T) std::byte data[sizeof(T)]{};

		constexpr un_t() noexcept = default;

		un_t(const un_t&) = delete;

		un_t& operator =(const un_t&) = delete;

		~un_t() = default;

		T* get() noexcept
		{
			return std::launder(reinterpret_cast<T*>(+data));
		}

		const T* get() const noexcept
		{
			return std::launder(reinterpret_cast<const T*>(+data));
		}

		T& operator =(const T& r) noexcept
		{
			return *get() = r;
		}

		T* operator ->() noexcept
		{
			return get();
		}

		const T* operator ->() const noexcept
		{
			return get();
		}

		operator T&() noexcept
		{
			return *get();
		}

		operator const T&() const noexcept
		{
			return *get();
		}

		static void init(un_t& un)
		{
			new (un.data) T();
		}

		void destroy()
		{
			get()->~T();
		}
	};
#endif

	// Essentially a fat semaphore
	struct alignas(64) cond_handle
	{
		constexpr cond_handle() noexcept = default;

		// Combined pointer (most significant 47 bits) and ref counter (17 least significant bits)
		atomic_t<u64> ptr_ref{};
		u64 tid{};
		__m128i mask{};
		__m128i oldv{};

		u64 tsc0{};
		u16 link{};
		u8 size{};
		u8 flag{};
		atomic_t<u32> sync{};

#ifdef USE_STD
		// Standard CV/mutex pair (often contains pthread_cond_t/pthread_mutex_t)
		un_t<std::condition_variable> cv{};
		un_t<std::mutex> mtx{};
#endif

		void init(std::uintptr_t iptr)
		{
#ifdef _WIN32
			tid = GetCurrentThreadId();
#else
			tid = reinterpret_cast<u64>(pthread_self());
#endif

#ifdef USE_STD
			cv.init(cv);
			mtx.init(mtx);
#endif

			verify(HERE), !ptr_ref.exchange((iptr << 17) | 1);
		}

		void destroy()
		{
			tid = 0;
			tsc0 = 0;
			link = 0;
			size = 0;
			flag = 0;
			sync.release(0);
			mask = _mm_setzero_si128();
			oldv = _mm_setzero_si128();

#ifdef USE_STD
			mtx.destroy();
			cv.destroy();
#endif
		}

		bool forced_wakeup()
		{
			const auto [_old, ok] = sync.fetch_op([](u32& val)
			{
				if (val - 1 <= 1)
				{
					val = 3;
					return true;
				}

				return false;
			});

			// Prevent collision between normal wake-up and forced one
			return ok && _old == 1;
		}

		bool wakeup(u32 cmp_res)
		{
			if (cmp_res == 1) [[likely]]
			{
				const auto [_old, ok] = sync.fetch_op([](u32& val)
				{
					if (val == 1)
					{
						val = 2;
						return true;
					}

					return false;
				});

				return ok;
			}

			if (cmp_res > 1) [[unlikely]]
			{
				// TODO.
				// Used when notify function is provided with enforced new value.
				return forced_wakeup();
			}

			return false;
		}

		bool set_sleep()
		{
			const auto [_old, ok] = sync.fetch_op([](u32& val)
			{
				if (val == 2)
				{
					val = 1;
					return true;
				}

				return false;
			});

			return ok;
		}

		void alert_native()
		{
#ifdef USE_FUTEX
			// Use "wake all" arg for robustness, only 1 thread is expected
			futex(&sync, FUTEX_WAKE_PRIVATE, 0x7fff'ffff);
#elif defined(USE_STD)
			// Not super efficient: locking is required to avoid lost notifications
			mtx->lock();
			mtx->unlock();
			cv->notify_all();
#elif defined(_WIN32)
			if (NtWaitForAlertByThreadId)
			{
				// Sets some sticky alert bit, at least I believe so
				NtAlertThreadByThreadId(tid);
			}
			else
			{
				// Can wait in rare cases, which is its annoying weakness
				NtReleaseKeyedEvent(nullptr, &sync, 1, nullptr);
			}
#endif
		}

		bool try_alert_native()
		{
#if defined(USE_FUTEX)
			return false;
#elif defined(USE_STD)
			// Optimistic non-blocking path
			if (mtx->try_lock())
			{
				mtx->unlock();
				cv->notify_all();
				return true;
			}

			return false;
#elif defined(_WIN32)
			if (NtAlertThreadByThreadId)
			{
				// Don't notify prematurely with this API
				return false;
			}

			static LARGE_INTEGER instant{};

			if (NtReleaseKeyedEvent(nullptr, &sync, 1, &instant) != NTSTATUS_SUCCESS)
			{
				// Failed to notify immediately
				return false;
			}

			return true;
#endif
		}
	};

#ifndef USE_STD
	static_assert(sizeof(cond_handle) == 64);
#endif
}

// Max allowed thread number is chosen to fit in 16 bits
static atomic_wait::cond_handle s_cond_list[UINT16_MAX + 1]{};

// Allocation bits
static atomic_t<u64, 64> s_cond_bits[(UINT16_MAX + 1) / 64]{};

// Allocation semaphore
static atomic_t<u32, 64> s_cond_sema{0};

static u32
#ifdef _WIN32
__vectorcall
#endif
cond_alloc(std::uintptr_t iptr, __m128i mask)
{
	// Determine whether there is a free slot or not
	if (!s_cond_sema.try_inc(UINT16_MAX + 1))
	{
		// Temporarily placed here
		fmt::raw_error("Thread semaphore limit " STRINGIZE(UINT16_MAX) " reached in atomic wait.");
		return 0;
	}

	// Diversify search start points to reduce contention and increase immediate success chance
#ifdef _WIN32
	const u32 start = GetCurrentProcessorNumber();
#elif __linux__
	const u32 start = sched_getcpu();
#else
	const u32 start = __rdtsc();
#endif

	for (u32 i = start;; i++)
	{
		const u32 group = i % ::size32(s_cond_bits);

		const auto [bits, ok] = s_cond_bits[group].fetch_op([](u64& bits)
		{
			if (~bits)
			{
				// Set lowest clear bit
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (ok) [[likely]]
		{
			// Find lowest clear bit
			const u32 id = group * 64 + std::countr_one(bits);

			if (id == 0) [[unlikely]]
			{
				// Special case, set bit and continue
				continue;
			}

			// Initialize new "semaphore"
			s_cond_list[id].mask = mask;
			s_cond_list[id].init(iptr);

			return id;
		}
	}

	// Unreachable
	std::abort();
	return 0;
}

static void cond_free(u32 cond_id)
{
	if (cond_id - 1 >= u32{UINT16_MAX})
	{
		fprintf(stderr, "cond_free(): bad id %u" HERE "\n", cond_id);
		std::abort();
	}

	const auto cond = s_cond_list + cond_id;

	// Dereference, destroy on last ref
	const bool last = cond->ptr_ref.atomic_op([](u64& val)
	{
		verify(HERE), val & s_ref_mask;

		val--;

		if ((val & s_ref_mask) == 0)
		{
			val = 0;
			return true;
		}

		return false;
	});

	if (!last)
	{
		return;
	}

	// Call the destructor if necessary
	cond->destroy();

	// Remove the allocation bit
	s_cond_bits[cond_id / 64] &= ~(1ull << (cond_id % 64));

	// Release the semaphore
	verify(HERE), s_cond_sema--;
}

static atomic_wait::cond_handle*
#ifdef _WIN32
__vectorcall
#endif
cond_id_lock(u32 cond_id, u32 size, __m128i mask, u64 thread_id = 0, std::uintptr_t iptr = 0)
{
	if (cond_id - 1 < u32{UINT16_MAX})
	{
		const auto cond = s_cond_list + cond_id;

		const auto [old, ok] = cond->ptr_ref.fetch_op([&](u64& val)
		{
			if (!val || (val & s_ref_mask) == s_ref_mask)
			{
				// Don't reference already deallocated semaphore
				return false;
			}

			if (iptr && (val >> 17) != iptr)
			{
				// Pointer mismatch
				return false;
			}

			const u32 sync_val = cond->sync;

			if (sync_val == 0 || sync_val == 3)
			{
				return false;
			}

			const __m128i mask12 = _mm_and_si128(mask, _mm_load_si128(&cond->mask));

			if (thread_id)
			{
				if (atomic_storage<u64>::load(cond->tid) != thread_id)
				{
					return false;
				}
			}
			else if (size && _mm_cvtsi128_si64(_mm_packs_epi16(mask12, mask12)) == 0)
			{
				return false;
			}

			val++;
			return true;
		});

		if (ok)
		{
			return cond;
		}

		if ((old & s_ref_mask) == s_ref_mask)
		{
			fmt::raw_error("Reference count limit (131071) reached in an atomic notifier.");
		}
	}

	return nullptr;
}

namespace
{
	struct alignas(16) slot_allocator
	{
		u64 ref : 16 = 0;
		u64 low : 48 = 0;
		u64 high = 0;

		constexpr slot_allocator() noexcept = default;
	};
}

namespace atomic_wait
{
	// Need to spare 16 bits for ref counter
	static constexpr u64 max_threads = 112;

	// Can only allow extended allocations go as far as this (about 585)
	static constexpr u64 max_distance = UINT16_MAX / max_threads;

	// Thread list
	struct alignas(64) root_info
	{
		constexpr root_info() noexcept = default;

		// Allocation bits (least significant)
		atomic_t<slot_allocator> bits{};

		// Allocation pool, pointers to allocated semaphores
		atomic_t<u16> slots[max_threads]{};

		// For collision statistics (32 middle bits)
		atomic_t<u32> first_ptr{};

		// For collision statistics (bit difference stick flags)
		atomic_t<u32> diff_lz{}, diff_tz{}, diff_pop{};

		atomic_t<u16>* slot_alloc(std::uintptr_t ptr) noexcept;

		root_info* slot_free(atomic_t<u16>* slot) noexcept;

		template <typename F>
		auto slot_search(std::uintptr_t iptr, u32 size, u64 thread_id, __m128i mask, F func) noexcept;
	};

	static_assert(sizeof(root_info) == 256);
}

// Main hashtable for atomic wait.
alignas(64) static atomic_wait::root_info s_hashtable[s_hashtable_size]{};

u64 atomic_wait::get_unique_tsc()
{
	const u64 stamp0 = __rdtsc();

	return s_min_tsc.atomic_op([&](u64& tsc)
	{
		if (stamp0 <= s_min_tsc)
		{
			// Add 1 if new stamp is too old
			return ++tsc;
		}
		else
		{
			// Update last tsc with new stamp otherwise
			return ((tsc = stamp0));
		}
	});
}

atomic_t<u16>* atomic_wait::root_info::slot_alloc(std::uintptr_t ptr) noexcept
{
	auto slot = bits.atomic_op([this](slot_allocator& bits) -> atomic_t<u16>*
	{
		// Increment reference counter
		if (bits.ref == UINT16_MAX)
		{
			fmt::raw_error("Thread limit " STRINGIZE(UINT16_MAX) " reached for a single hashtable slot.");
			return nullptr;
		}

		bits.ref++;

		// Check free slots
		if (~bits.high)
		{
			// Set lowest clear bit
			const u32 id = std::countr_one(bits.high);
			bits.high |= bits.high + 1;
			return this->slots + id;
		}

		if (~bits.low << 16)
		{
			const u32 id = std::countr_one(bits.low);
			bits.low |= bits.low + 1;
			return this->slots + 64 + id;
		}

		return nullptr;
	});

	u32 limit = 0;

	for (auto* _this = this + 1; !slot;)
	{
		auto [_old, slot2] = _this->bits.fetch_op([_this](slot_allocator& bits) -> atomic_t<u16>*
		{
			if (~bits.high)
			{
				const u32 id = std::countr_one(bits.high);
				bits.high |= bits.high + 1;
				return _this->slots + id;
			}

			if (~bits.low << 16)
			{
				const u32 id = std::countr_one(bits.low);
				bits.low |= bits.low + 1;
				return _this->slots + 64 + id;
			}

			return nullptr;
		});

		if (slot2)
		{
			slot = slot2;
			break;
		}

		// Keep trying adjacent slots in the hashtable, they are often free due to alignment.
		_this++;
		limit++;

		if (limit == max_distance) [[unlikely]]
		{
			fmt::raw_error("Distance limit (585) exceeded for the atomic wait hashtable.");
			return nullptr;
		}

		if (_this == std::end(s_hashtable)) [[unlikely]]
		{
			_this = s_hashtable;
		}
	}

	u32 ptr32 = static_cast<u32>(ptr >> 16);
	u32 first = first_ptr.load();

	if (!first && first != ptr32)
	{
		// Register first used pointer
		first = first_ptr.compare_and_swap(0, ptr32);
	}

	if (first && first != ptr32)
	{
		// Difference bits between pointers
		u32 diff = first ^ ptr32;

		// The most significant different bit
		u32 diff1 = std::countl_zero(diff);

		if (diff1 < 32)
		{
			diff_lz |= 1u << diff1;
		}

		u32 diff2 = std::countr_zero(diff);

		if (diff2 < 32)
		{
			diff_tz |= 1u << diff2;
		}

		diff = (diff & 0xaaaaaaaa) / 2 + (diff & 0x55555555);
		diff = (diff & 0xcccccccc) / 4 + (diff & 0x33333333);
		diff = (diff & 0xf0f0f0f0) / 16 + (diff & 0x0f0f0f0f);
		diff = (diff & 0xff00ff00) / 256 + (diff & 0x00ff00ff);

		diff_pop |= 1u << static_cast<u8>((diff >> 16) + diff - 1);
	}

	return slot;
}

atomic_wait::root_info* atomic_wait::root_info::slot_free(atomic_t<u16>* slot) noexcept
{
	const auto begin = reinterpret_cast<std::uintptr_t>(std::begin(s_hashtable));

	const auto end = reinterpret_cast<std::uintptr_t>(std::end(s_hashtable));

	const auto ptr = reinterpret_cast<std::uintptr_t>(slot) - begin;

	if (ptr >= sizeof(s_hashtable))
	{
		fmt::raw_error("Failed to find slot in hashtable slot deallocation." HERE);
		return nullptr;
	}

	root_info* _this = &s_hashtable[ptr / sizeof(root_info)];

	if (!(slot >= _this->slots && slot < std::end(_this->slots)))
	{
		fmt::raw_error("Failed to find slot in hashtable slot deallocation." HERE);
		return nullptr;
	}

	const u32 diff = static_cast<u32>(slot - _this->slots);

	verify(HERE), slot == &_this->slots[diff];

	const u32 cond_id = slot->exchange(0);

	if (cond_id)
	{
		cond_free(cond_id);
	}

	if (_this != this)
	{
		// Reset allocation bit in the adjacent hashtable slot
		_this->bits.atomic_op([diff](slot_allocator& bits)
		{
			if (diff < 64)
			{
				bits.high &= ~(1ull << diff);
			}
			else
			{
				bits.low &= ~(1ull << (diff - 64));
			}
		});
	}

	// Reset reference counter
	bits.atomic_op([&](slot_allocator& bits)
	{
		verify(HERE), bits.ref--;

		if (_this == this)
		{
			if (diff < 64)
			{
				bits.high &= ~(1ull << diff);
			}
			else
			{
				bits.low &= ~(1ull << (diff - 64));
			}
		}
	});

	return _this;
}

template <typename F>
FORCE_INLINE auto atomic_wait::root_info::slot_search(std::uintptr_t iptr, u32 size, u64 thread_id, __m128i mask, F func) noexcept
{
	u32 index = 0;
	u32 total = 0;
	u64 limit = 0;

	for (auto* _this = this;;)
	{
		const auto bits = _this->bits.load();

		u16 cond_ids[max_threads];
		u32 cond_count = 0;

		u64 high_val = bits.high;
		u64 low_val = bits.low;

		if (_this == this)
		{
			limit = bits.ref;
		}

		for (u64 bits = high_val; bits; bits &= bits - 1)
		{
			if (u16 cond_id = _this->slots[std::countr_zero(bits)])
			{
				utils::prefetch_read(s_cond_list + cond_id);
				cond_ids[cond_count++] = cond_id;
			}
		}

		for (u64 bits = low_val; bits; bits &= bits - 1)
		{
			if (u16 cond_id = _this->slots[std::countr_zero(bits)])
			{
				utils::prefetch_read(s_cond_list + cond_id);
				cond_ids[cond_count++] = cond_id;
			}
		}

		for (u32 i = 0; i < cond_count; i++)
		{
			if (cond_id_lock(cond_ids[i], size, mask, thread_id, iptr))
			{
				if (func(cond_ids[i]))
				{
					return;
				}
			}
		}

		total += cond_count;

		if (total >= limit)
		{
			return;
		}

		_this++;
		index++;

		if (index == max_distance)
		{
			return;
		}

		if (_this >= std::end(s_hashtable))
		{
			_this = s_hashtable;
		}
	}
}

SAFE_BUFFERS void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::wait(const void* data, u32 size, __m128i old_value, u64 timeout, __m128i mask, atomic_wait::info* ext)
{
	const auto stamp0 = atomic_wait::get_unique_tsc();

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data) & (~s_ref_mask >> 17);

	const auto root = &s_hashtable[iptr % s_hashtable_size];

	uint ext_size = 0;

	std::uintptr_t iptr_ext[atomic_wait::max_list - 1]{};

	atomic_wait::root_info* root_ext[atomic_wait::max_list - 1]{};

	if (ext) [[unlikely]]
	{
		for (auto e = ext; e->data; e++)
		{
			if (data == e->data)
			{
				fmt::raw_error("Address duplication in atomic_wait::list" HERE);
			}

			for (u32 j = 0; j < ext_size; j++)
			{
				if (e->data == ext[j].data)
				{
					fmt::raw_error("Address duplication in atomic_wait::list" HERE);
				}
			}

			iptr_ext[ext_size] = reinterpret_cast<std::uintptr_t>(e->data) & (~s_ref_mask >> 17);
			root_ext[ext_size] = &s_hashtable[iptr & s_hashtable_size];
			ext_size++;
		}
	}

	const u32 cond_id = cond_alloc(iptr, mask);

	u32 cond_id_ext[atomic_wait::max_list - 1]{};

	for (u32 i = 0; i < ext_size; i++)
	{
		cond_id_ext[i] = cond_alloc(iptr_ext[i], ext[i].mask);
	}

	const auto slot = root->slot_alloc(iptr);

	std::array<atomic_t<u16>*, atomic_wait::max_list - 1> slot_ext{};

	std::array<atomic_wait::cond_handle*, atomic_wait::max_list - 1> cond_ext{};

	for (u32 i = 0; i < ext_size; i++)
	{
		// Allocate slot for cond id location
		slot_ext[i] = root_ext[i]->slot_alloc(iptr_ext[i]);

		// Get pointers to the semaphores
		cond_ext[i] = s_cond_list + cond_id_ext[i];
	}

	// Save for notifiers
	const auto cond = s_cond_list + cond_id;

	// Store some info for notifiers (some may be unused)
	cond->link = 0;
	cond->size = static_cast<u8>(size);
	cond->flag = static_cast<u8>(size >> 8);
	cond->oldv = old_value;
	cond->tsc0 = stamp0;

	cond->sync.release(1);

	for (u32 i = 0; i < ext_size; i++)
	{
		// Extensions point to original cond_id, copy remaining info
		cond_ext[i]->link = cond_id;
		cond_ext[i]->size = static_cast<u8>(ext[i].size);
		cond_ext[i]->flag = static_cast<u8>(ext[i].size >> 8);
		cond_ext[i]->oldv = ext[i].old;
		cond_ext[i]->tsc0 = stamp0;

		// Cannot be notified, should be redirected to main semaphore
		cond_ext[i]->sync.release(4);
	}

	// Final deployment
	slot->store(static_cast<u16>(cond_id));

	for (u32 i = 0; i < ext_size; i++)
	{
		slot_ext[i]->release(static_cast<u16>(cond_id_ext[i]));
	}

#ifdef USE_STD
	// Lock mutex
	std::unique_lock lock(*cond->mtx.get());
#else
	if (ext_size)
		_mm_mfence();
#endif

	// Can skip unqueue process if true
#if defined(USE_FUTEX) || defined(USE_STD)
	constexpr bool fallback = true;
#else
	bool fallback = false;
#endif

	while (ptr_cmp(data, size, old_value, mask, ext))
	{
#ifdef USE_FUTEX
		struct timespec ts;
		ts.tv_sec  = timeout / 1'000'000'000;
		ts.tv_nsec = timeout % 1'000'000'000;

		const u32 val = cond->sync;

		if (val > 1) [[unlikely]]
		{
			// Signaled prematurely
			if (!cond->set_sleep())
			{
				break;
			}
		}
		else
		{
			futex(&cond->sync, FUTEX_WAIT_PRIVATE, val, timeout + 1 ? &ts : nullptr);
		}
#elif defined(USE_STD)
		if (cond->sync > 1) [[unlikely]]
		{
			if (!cond->set_sleep())
			{
				break;
			}
		}

		if (timeout + 1)
		{
			cond->cv->wait_for(lock, std::chrono::nanoseconds(timeout));
		}
		else
		{
			cond->cv->wait(lock);
		}
#elif defined(_WIN32)
		LARGE_INTEGER qw;
		qw.QuadPart = -static_cast<s64>(timeout / 100);

		if (timeout % 100)
		{
			// Round up to closest 100ns unit
			qw.QuadPart -= 1;
		}

		if (fallback) [[unlikely]]
		{
			if (!cond->set_sleep())
			{
				fallback = false;
				break;
			}

			fallback = false;
		}

		if (NtWaitForAlertByThreadId)
		{
			switch (DWORD status = NtWaitForAlertByThreadId(cond, timeout + 1 ? &qw : nullptr))
			{
			case NTSTATUS_ALERTED: fallback = true; break;
			case NTSTATUS_TIMEOUT: break;
			default:
			{
				SetLastError(status);
				fmt::raw_verify_error("Unexpected NtWaitForAlertByThreadId result.", nullptr, 0);
			}
			}
		}
		else
		{
			if (NtWaitForKeyedEvent(nullptr, &cond->sync, false, timeout + 1 ? &qw : nullptr) == NTSTATUS_SUCCESS)
			{
				// Error code assumed to be timeout
				fallback = true;
			}
		}
#endif

		if (timeout + 1)
		{
			// TODO: reduce timeout instead
			break;
		}
	}

	while (!fallback)
	{
#if defined(_WIN32)
		static LARGE_INTEGER instant{};

		if (cond->wakeup(1))
		{
			// Succeeded in self-notifying
			break;
		}

		if (NtWaitForAlertByThreadId)
		{
			if (NtWaitForAlertByThreadId(cond, &instant) == NTSTATUS_ALERTED)
			{
				break;
			}

			continue;
		}

		if (!NtWaitForKeyedEvent(nullptr, &cond->sync, false, &instant))
		{
			// Succeeded in obtaining an event without waiting
			break;
		}

		continue;
#endif
	}

#ifdef USE_STD
	if (lock)
	{
		lock.unlock();
	}
#endif

	// Release resources in reverse order
	for (u32 i = ext_size - 1; i != umax; i--)
	{
		verify(HERE), root_ext[i] == root_ext[i]->slot_free(slot_ext[i]);
	}

	verify(HERE), root == root->slot_free(slot);

	s_tls_wait_cb(nullptr);
}

template <bool TryAlert = false>
static u32
#ifdef _WIN32
__vectorcall
#endif
alert_sema(u32 cond_id, const void* data, u64 tid, u32 size, __m128i mask, __m128i new_value)
{
	verify(HERE), cond_id;

	const auto cond = s_cond_list + cond_id;

	u32 ok = 0;

	if (!size ? (!tid || cond->tid == tid) : cmp_mask(size, mask, new_value, cond->size | (cond->flag << 8), cond->mask, cond->oldv))
	{
		// Redirect if necessary
		const auto _old = cond;
		const auto _new = _old->link ? cond_id_lock(_old->link, 0, _mm_set1_epi64x(-1)) : _old;

		if (_new && _new->tsc0 == _old->tsc0)
		{
			if ((!size && _new->forced_wakeup()) || (size && _new->wakeup(size == umax ? 1 : 2)))
			{
				ok = cond_id;

				if constexpr (TryAlert)
				{
					if (!_new->try_alert_native())
					{
						if (_new != _old)
						{
							// Keep base cond for another attempt, free only secondary cond
							ok = ~_old->link;
							cond_free(cond_id);
							return ok;
						}
						else
						{
							// Keep cond for another attempt
							ok = ~cond_id;
							return ok;
						}
					}
				}
				else
				{
					_new->alert_native();
				}
			}
		}

		if (_new && _new != _old)
		{
			cond_free(_old->link);
		}
	}

	// Remove lock, possibly deallocate cond
	cond_free(cond_id);
	return ok;
}

void atomic_wait_engine::set_wait_callback(bool(*cb)(const void* data))
{
	if (cb)
	{
		s_tls_wait_cb = cb;
	}
	else
	{
		s_tls_wait_cb = [](const void*){ return true; };
	}
}

void atomic_wait_engine::set_notify_callback(void(*cb)(const void*, u64))
{
	if (cb)
	{
		s_tls_notify_cb = cb;
	}
	else
	{
		s_tls_notify_cb = [](const void*, u64){};
	}
}

bool atomic_wait_engine::raw_notify(const void* data, u64 thread_id)
{
	// Special operation mode. Note that this is not atomic.
	if (!data)
	{
		// Special path: search thread_id without pointer information
		for (u32 i = 1; i <= UINT16_MAX; i++)
		{
			const auto cond = s_cond_list + i;

			const auto [old, ok] = cond->ptr_ref.fetch_op([&](u64& val)
			{
				if (!val)
				{
					// Skip dead semaphores
					return false;
				}

				u32 sync_val = cond->sync.load();

				if (sync_val == 0 || sync_val >= 3)
				{
					// Skip forced signaled or secondary semaphores
					return false;
				}

				if (thread_id)
				{
					// Check thread if provided
					if (atomic_storage<u64>::load(cond->tid) != thread_id)
					{
						return false;
					}
				}

				if ((val & s_ref_mask) < s_ref_mask)
				{
					val++;
					return true;
				}

				return true;
			});

			if (ok) [[unlikely]]
			{
				const auto cond = s_cond_list + i;

				if (!thread_id || cond->tid == thread_id)
				{
					if (!cond->link && cond->forced_wakeup())
					{
						cond->alert_native();

						if (thread_id)
						{
							// Only if thread_id is speficied, stop only one and return true.
							if ((old & s_ref_mask) < s_ref_mask)
							{
								cond_free(i);
							}

							return true;
						}
					}
				}

				if ((old & s_ref_mask) < s_ref_mask)
				{
					cond_free(i);
				}
			}
		}

		return false;
	}

	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data) & (~s_ref_mask >> 17);

	auto* const root = &s_hashtable[iptr % s_hashtable_size];

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	root->slot_search(iptr, 0, thread_id, _mm_set1_epi64x(-1), [&](u32 cond_id)
	{
		// Forced notification
		if (alert_sema(cond_id, data, thread_id, 0, _mm_setzero_si128(), _mm_setzero_si128()))
		{
			s_tls_notify_cb(data, ++progress);

			if (thread_id == 0)
			{
				// Works like notify_all in this case
				return false;
			}

			return true;
		}

		return false;
	});

	s_tls_notify_cb(data, -1);

	return progress != 0;
}

void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::notify_one(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data) & (~s_ref_mask >> 17);

	auto* const root = &s_hashtable[iptr % s_hashtable_size];

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	root->slot_search(iptr, size, 0, mask, [&](u32 cond_id)
	{
		if (alert_sema(cond_id, data, -1, size, mask, new_value))
		{
			s_tls_notify_cb(data, ++progress);
			return true;
		}

		return false;
	});

	s_tls_notify_cb(data, -1);
}

SAFE_BUFFERS void
#ifdef _WIN32
__vectorcall
#endif
atomic_wait_engine::notify_all(const void* data, u32 size, __m128i mask, __m128i new_value)
{
	const std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(data) & (~s_ref_mask >> 17);

	auto* const root = &s_hashtable[iptr % s_hashtable_size];

	s_tls_notify_cb(data, 0);

	u64 progress = 0;

	// Array count for batch notification
	u32 count = 0;

	// Array itself.
	u16 cond_ids[UINT16_MAX + 1];

	root->slot_search(iptr, size, 0, mask, [&](u32 cond_id)
	{
		u32 res = alert_sema<true>(cond_id, data, -1, size, mask, new_value);

		if (res <= UINT16_MAX)
		{
			if (res)
			{
				s_tls_notify_cb(data, ++progress);
			}
		}
		else
		{
			// Add to the end of the "stack"
			cond_ids[UINT16_MAX - count++] = ~res;
		}

		return false;
	});

	// Cleanup
	for (u32 i = 0; i < count; i++)
	{
		const u32 cond_id = cond_ids[UINT16_MAX - i];
		s_cond_list[cond_id].alert_native();
		s_tls_notify_cb(data, ++progress);
		cond_free(cond_id);
	}

	s_tls_notify_cb(data, -1);
}
