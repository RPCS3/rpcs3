#include "atomic.hpp"

#if defined(__linux__)
#define USE_FUTEX
#elif !defined(_WIN32)
#define USE_STD
#endif

#include "Utilities/sync.h"
#include "Utilities/StrFmt.h"

#include <utility>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <random>

#include "asm.hpp"
#include "endian.hpp"

// Total number of entries.
static constexpr usz s_hashtable_size = 1u << 17;

// Reference counter combined with shifted pointer (which is assumed to be 47 bit)
static constexpr uptr s_ref_mask = (1u << 17) - 1;

// Fix for silly on-first-use initializer
static bool s_null_wait_cb(const void*, u64, u64){ return true; };

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data, u64 attempts, u64 stamp0) = s_null_wait_cb;

// Callback for notification functions for optimizations
static thread_local void(*s_tls_notify_cb)(const void* data, u64 progress) = nullptr;

static inline bool operator &(atomic_wait::op lhs, atomic_wait::op_flag rhs)
{
	return !!(static_cast<u8>(lhs) & static_cast<u8>(rhs));
}

// Compare data in memory with old value, and return true if they are equal
static NEVER_INLINE bool ptr_cmp(const void* data, u32 _size, u128 old128, u128 mask128, atomic_wait::info* ext = nullptr)
{
	using atomic_wait::op;
	using atomic_wait::op_flag;

	const u8 size = static_cast<u8>(_size);
	const op flag{static_cast<u8>(_size >> 8)};

	bool result = false;

	if (size <= 8)
	{
		u64 new_value = 0;
		u64 old_value = static_cast<u64>(old128);
		u64 mask = static_cast<u64>(mask128) & (UINT64_MAX >> ((64 - size * 8) & 63));

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
			fmt::throw_exception("Bad size (arg=0x%x)", _size);
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
			const u64 count = static_cast<u64>(old128) & 0xff;

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
			fmt::throw_exception("ptr_cmp(): unrecognized atomic wait operation.");
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
		fmt::throw_exception("ptr_cmp(): no alternative operations are supported for 16-byte atomic wait yet.");
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
				if (!ptr_cmp(e->data, e->size, e->old, e->mask))
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
static bool cmp_mask(u32 size1, u128 mask1, u128 val1, u32 size2, u128 mask2, u128 val2)
{
	// Compare only masks, new value is not available in this mode
	if (size1 == umax)
	{
		// Simple mask overlap
		const u128 v0 = mask1 & mask2;
		return !!(v0);
	}

	// Generate masked value inequality bits
	const u128 v0 = (mask1 & mask2) & (val1 ^ val2);

	using atomic_wait::op;
	using atomic_wait::op_flag;

	const u8 size = std::min<u8>(static_cast<u8>(size2), static_cast<u8>(size1));
	const op flag{static_cast<u8>(size2 >> 8)};

	if (flag != op::eq && flag != (op::eq | op_flag::inverse))
	{
		fmt::throw_exception("cmp_mask(): no operations are supported for notification with forced value yet.");
	}

	if (size <= 8)
	{
		// Generate sized mask
		const u64 mask = UINT64_MAX >> ((64 - size * 8) & 63);

		if (!(static_cast<u64>(v0) & mask))
		{
			return !!(flag & op_flag::inverse);
		}
	}
	else if (size == 16)
	{
		if (!v0)
		{
			return !!(flag & op_flag::inverse);
		}
	}
	else
	{
		fmt::throw_exception("bad size (size1=%u, size2=%u)", size1, size2);
	}

	return !(flag & op_flag::inverse);
}

static atomic_t<u64> s_min_tsc{0};

namespace
{
#ifdef USE_STD
	// Just madness to keep some members uninitialized and get zero initialization otherwise
	template <typename T>
	struct alignas(T) un_t
	{
		std::byte data[sizeof(T)];

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
	struct cond_handle
	{
		// Combined pointer (most significant 47 bits) and ref counter (17 least significant bits)
		atomic_t<u64> ptr_ref;
		u64 tid;
		u128 mask;
		u128 oldv;

		u64 tsc0;
		u16 link;
		u8 size;
		u8 flag;
		atomic_t<u32> sync;

#ifdef USE_STD
		// Standard CV/mutex pair (often contains pthread_cond_t/pthread_mutex_t)
		un_t<std::condition_variable> cv;
		un_t<std::mutex> mtx;
#endif

		void init(uptr iptr)
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

			ensure(!ptr_ref.exchange((iptr << 17) | 1));
		}

		void destroy()
		{
			tid = 0;
			tsc0 = 0;
			link = 0;
			size = 0;
			flag = 0;
			sync.release(0);
			mask = 0;
			oldv = 0;

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

// Produce u128 value that repeats val 8 times
static constexpr u128 dup8(u32 val)
{
	const u32 shift = 32 - std::countl_zero(val);

	const u128 it0 = u128{val};
	const u128 it1 = it0 | (it0 << shift);
	const u128 it2 = it1 | (it1 << (shift * 2));
	const u128 it3 = it2 | (it2 << (shift * 4));

	return it3;
}

// Free or put in specified tls slot
static void cond_free(u32 cond_id, u32 tls_slot);

// Semaphore tree root (level 1) - split in 8 parts (8192 in each)
static atomic_t<u128> s_cond_sem1{1};

// Semaphore tree (level 2) - split in 8 parts (1024 in each)
static atomic_t<u128> s_cond_sem2[8]{{1}};

// Semaphore tree (level 3) - split in 16 parts (128 in each)
static atomic_t<u128> s_cond_sem3[64]{{1}};

// Allocation bits (level 4) - guarantee 1 free bit
static atomic_t<u64> s_cond_bits[(UINT16_MAX + 1) / 64]{1};

// Max allowed thread number is chosen to fit in 16 bits
static cond_handle s_cond_list[UINT16_MAX + 1]{};

namespace
{
	struct tls_cond_handler
	{
		u16 cond[4]{};

		constexpr tls_cond_handler() noexcept = default;

		~tls_cond_handler()
		{
			for (u32 cond_id : cond)
			{
				if (cond_id)
				{
					// Set fake refctr
					s_cond_list[cond_id].ptr_ref.release(1);
					cond_free(cond_id, -1);
				}
			}
		}
	};
}

// TLS storage for few allocaded "semaphores" to allow skipping initialization
static thread_local tls_cond_handler s_tls_conds{};

static u32 cond_alloc(uptr iptr, u128 mask, u32 tls_slot = -1)
{
	// Try to get cond from tls slot instead
	u16* ptls = tls_slot >= std::size(s_tls_conds.cond) ? nullptr : s_tls_conds.cond + tls_slot;

	if (ptls && *ptls) [[likely]]
	{
		// Fast reinitialize
		const u32 id = std::exchange(*ptls, 0);
		s_cond_list[id].mask = mask;
		s_cond_list[id].ptr_ref.release((iptr << 17) | 1);
		return id;
	}

	const u32 level1 = s_cond_sem1.atomic_op([](u128& val) -> u32
	{
		constexpr u128 max_mask = dup8(8192);

		// Leave only bits indicating sub-semaphore is full, find free one
		const u32 pos = utils::ctz128(~val & max_mask);

		if (pos == 128) [[unlikely]]
		{
			// No free space
			return -1;
		}

		val += u128{1} << (pos / 14 * 14);

		return pos / 14;
	});

	// Determine whether there is a free slot or not
	if (level1 < 8) [[likely]]
	{
		const u32 level2 = level1 * 8 + s_cond_sem2[level1].atomic_op([](u128& val)
		{
			constexpr u128 max_mask = dup8(1024);

			const u32 pos = utils::ctz128(~val & max_mask);

			val += u128{1} << (pos / 11 * 11);

			return pos / 11;
		});

		const u32 level3 = level2 * 16 + s_cond_sem3[level2].atomic_op([](u128& val)
		{
			constexpr u128 max_mask = dup8(64) | (dup8(64) << 56);

			const u32 pos = utils::ctz128(~val & max_mask);

			val += u128{1} << (pos / 7 * 7);

			return pos / 7;
		});

		const u64 bits = s_cond_bits[level3].fetch_op([](u64& bits)
		{
			// Set lowest clear bit
			bits |= bits + 1;
		});

		// Find lowest clear bit (before it was set in fetch_op)
		const u32 id = level3 * 64 + std::countr_one(bits);

		// Initialize new "semaphore"
		s_cond_list[id].mask = mask;
		s_cond_list[id].init(iptr);
		return id;
	}

	fmt::throw_exception("Thread semaphore limit (65535) reached in atomic wait.");
}

static void cond_free(u32 cond_id, u32 tls_slot = -1)
{
	if (cond_id - 1 >= u32{UINT16_MAX}) [[unlikely]]
	{
		fmt::throw_exception("bad id %u", cond_id);
	}

	const auto cond = s_cond_list + cond_id;

	// Dereference, destroy on last ref
	const bool last = cond->ptr_ref.atomic_op([](u64& val)
	{
		ensure(val & s_ref_mask);

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

	u16* ptls = tls_slot >= std::size(s_tls_conds.cond) ? nullptr : s_tls_conds.cond + tls_slot;

	if (ptls && !*ptls) [[likely]]
	{
		// Fast finalization
		cond->sync.release(0);
		cond->mask = 0;
		*ptls = static_cast<u16>(cond_id);
		return;
	}

	// Call the destructor if necessary
	utils::prefetch_write(s_cond_bits + cond_id / 64);

	const u32 level3 = cond_id / 64 % 16;
	const u32 level2 = cond_id / 1024 % 8;
	const u32 level1 = cond_id / 8192 % 8;

	utils::prefetch_write(s_cond_sem3 + level2);
	utils::prefetch_write(s_cond_sem2 + level1);
	utils::prefetch_write(&s_cond_sem1);

	cond->destroy();

	// Release the semaphore tree in the reverse order
	s_cond_bits[cond_id / 64] &= ~(1ull << (cond_id % 64));

	s_cond_sem3[level2].atomic_op([&](u128& val)
	{
		val -= u128{1} << (level3 * 7);
	});

	s_cond_sem2[level1].atomic_op([&](u128& val)
	{
		val -= u128{1} << (level2 * 11);
	});

	s_cond_sem1.atomic_op([&](u128& val)
	{
		val -= u128{1} << (level1 * 14);
	});
}

static cond_handle* cond_id_lock(u32 cond_id, u32 size, u128 mask, u64 thread_id = 0, uptr iptr = 0)
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

			const u128 mask12 = mask & cond->mask;

			if (thread_id)
			{
				if (atomic_storage<u64>::load(cond->tid) != thread_id)
				{
					return false;
				}
			}
			else if (size && !mask12)
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
			fmt::throw_exception("Reference count limit (131071) reached in an atomic notifier.");
		}
	}

	return nullptr;
}

namespace
{
	struct alignas(16) slot_allocator
	{
		u64 maxc: 5; // Collision counter
		u64 maxd: 11; // Distance counter
		u64 bits: 24; // Allocated bits
		u64 prio: 24; // Reserved

		u64 ref : 17; // Ref counter
		u64 iptr: 47; // First pointer to use slot (to count used slots)
	};

	// Need to spare 16 bits for ref counter
	static constexpr u64 max_threads = 24;

	// (Arbitrary, not justified) Can only allow extended allocations go as far as this
	static constexpr u64 max_distance = 500;

	// Thread list
	struct alignas(64) root_info
	{
		// Allocation bits (least significant)
		atomic_t<slot_allocator> bits;

		// Allocation pool, pointers to allocated semaphores
		atomic_t<u16> slots[max_threads];

		static atomic_t<u16>* slot_alloc(uptr ptr) noexcept;

		static void slot_free(uptr ptr, atomic_t<u16>* slot, u32 tls_slot) noexcept;

		template <typename F>
		static auto slot_search(uptr iptr, u32 size, u64 thread_id, u128 mask, F func) noexcept;
	};

	static_assert(sizeof(root_info) == 64);
}

// Main hashtable for atomic wait.
static root_info s_hashtable[s_hashtable_size]{};

namespace
{
	struct hash_engine
	{
		// Pseudo-RNG, seeded with input pointer
		using rng = std::linear_congruential_engine<u64, 2862933555777941757, 3037000493, 0>;

		const u64 init;

		// Subpointers
		u16 r0;
		u16 r1;

		// Pointer to the current hashtable slot
		u32 id;

		// Initialize: PRNG on iptr, split into two 16 bit chunks, choose first chunk
		explicit hash_engine(uptr iptr)
			: init(rng(iptr)())
			, r0(static_cast<u16>(init >> 48))
			, r1(static_cast<u16>(init >> 32))
			, id(static_cast<u32>(init) >> 31 ? r0 : r1 + 0x10000)
		{
		}

		// Advance: linearly to prevent self-collisions, but always switch between two big 2^16 chunks
		void operator++(int) noexcept
		{
			if (id >= 0x10000)
			{
				id = r0++;
			}
			else
			{
				id = r1++ + 0x10000;
			}
		}

		root_info* current() const noexcept
		{
			return &s_hashtable[id];
		}

		root_info* operator ->() const noexcept
		{
			return current();
		}
	};
}

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

atomic_t<u16>* root_info::slot_alloc(uptr ptr) noexcept
{
	atomic_t<u16>* slot = nullptr;

	u32 limit = 0;

	for (hash_engine _this(ptr);; _this++)
	{
		slot = _this->bits.atomic_op([&](slot_allocator& bits) -> atomic_t<u16>*
		{
			// Increment reference counter on every hashtable slot we attempt to allocate on
			if (bits.ref == UINT16_MAX)
			{
				fmt::throw_exception("Thread limit (65535) reached for a single hashtable slot.");
				return nullptr;
			}

			if (bits.iptr == 0)
				bits.iptr = ptr;
			if (bits.maxc == 0 && bits.iptr != ptr && bits.ref)
				bits.maxc = 1;
			if (bits.maxd < limit)
				bits.maxd = limit;

			bits.ref++;

			if (bits.bits != (1ull << max_threads) - 1)
			{
				const u32 id = std::countr_one(bits.bits);
				bits.bits |= bits.bits + 1;
				return _this->slots + id;
			}

			return nullptr;
		});

		if (slot)
		{
			break;
		}

		// Keep trying adjacent slots in the hashtable, they are often free due to alignment.
		limit++;

		if (limit == max_distance) [[unlikely]]
		{
			fmt::throw_exception("Distance limit (500) exceeded for the atomic wait hashtable.");
			return nullptr;
		}
	}

	return slot;
}

void root_info::slot_free(uptr iptr, atomic_t<u16>* slot, u32 tls_slot) noexcept
{
	const auto begin = reinterpret_cast<uptr>(std::begin(s_hashtable));

	const auto end = reinterpret_cast<uptr>(std::end(s_hashtable));

	const auto ptr = reinterpret_cast<uptr>(slot) - begin;

	if (ptr >= sizeof(s_hashtable))
	{
		fmt::throw_exception("Failed to find slot in hashtable slot deallocation.");
		return;
	}

	root_info* _this = &s_hashtable[ptr / sizeof(root_info)];

	if (!(slot >= _this->slots && slot < std::end(_this->slots)))
	{
		fmt::throw_exception("Failed to find slot in hashtable slot deallocation.");
		return;
	}

	const u32 diff = static_cast<u32>(slot - _this->slots);

	ensure(slot == &_this->slots[diff]);

	const u32 cond_id = slot->exchange(0);

	if (cond_id)
	{
		cond_free(cond_id, tls_slot);
	}

	for (hash_engine curr(iptr);; curr++)
	{
		// Reset reference counter and allocation bit in every slot
		curr->bits.atomic_op([&](slot_allocator& bits)
		{
			ensure(bits.ref--);

			if (_this == curr.current())
			{
				bits.bits &= ~(1ull << diff);
			}
		});

		if (_this == curr.current())
		{
			break;
		}
	}
}

template <typename F>
FORCE_INLINE auto root_info::slot_search(uptr iptr, u32 size, u64 thread_id, u128 mask, F func) noexcept
{
	u32 index = 0;
	u32 total = 0;

	for (hash_engine _this(iptr);; _this++)
	{
		const auto bits = _this->bits.load();

		if (bits.ref == 0) [[likely]]
		{
			return;
		}

		u16 cond_ids[max_threads];
		u32 cond_count = 0;

		u64 bits_val = bits.bits;

		for (u64 bits = bits_val; bits; bits &= bits - 1)
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

		index++;

		if (index == max_distance)
		{
			return;
		}
	}
}

SAFE_BUFFERS void atomic_wait_engine::wait(const void* data, u32 size, u128 old_value, u64 timeout, u128 mask, atomic_wait::info* ext)
{
	const auto stamp0 = atomic_wait::get_unique_tsc();

	if (!s_tls_wait_cb(data, 0, stamp0))
	{
		return;
	}

	const uptr iptr = reinterpret_cast<uptr>(data) & (~s_ref_mask >> 17);

	uint ext_size = 0;

	uptr iptr_ext[atomic_wait::max_list - 1]{};

	if (ext) [[unlikely]]
	{
		for (auto e = ext; e->data; e++)
		{
			if (data == e->data)
			{
				fmt::throw_exception("Address duplication in atomic_wait::list");
			}

			for (u32 j = 0; j < ext_size; j++)
			{
				if (e->data == ext[j].data)
				{
					fmt::throw_exception("Address duplication in atomic_wait::list");
				}
			}

			iptr_ext[ext_size] = reinterpret_cast<uptr>(e->data) & (~s_ref_mask >> 17);
			ext_size++;
		}
	}

	const u32 cond_id = cond_alloc(iptr, mask, 0);

	u32 cond_id_ext[atomic_wait::max_list - 1]{};

	for (u32 i = 0; i < ext_size; i++)
	{
		cond_id_ext[i] = cond_alloc(iptr_ext[i], ext[i].mask, i + 1);
	}

	const auto slot = root_info::slot_alloc(iptr);

	std::array<atomic_t<u16>*, atomic_wait::max_list - 1> slot_ext{};

	std::array<cond_handle*, atomic_wait::max_list - 1> cond_ext{};

	for (u32 i = 0; i < ext_size; i++)
	{
		// Allocate slot for cond id location
		slot_ext[i] = root_info::slot_alloc(iptr_ext[i]);

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
		atomic_fence_seq_cst();
#endif

	// Can skip unqueue process if true
#if defined(USE_FUTEX) || defined(USE_STD)
	constexpr bool fallback = true;
#else
	bool fallback = false;
#endif

	u64 attempts = 0;

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
		else if (timeout + 1)
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
				if (cond->sync == 3)
				{
					break;
				}
			}

			fallback = false;
		}
		else if (NtWaitForAlertByThreadId)
		{
			switch (DWORD status = NtWaitForAlertByThreadId(cond, timeout + 1 ? &qw : nullptr))
			{
			case NTSTATUS_ALERTED: fallback = true; break;
			case NTSTATUS_TIMEOUT: break;
			default:
			{
				SetLastError(status);
				ensure(false); // Unexpected result
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

		if (!s_tls_wait_cb(data, ++attempts, stamp0))
		{
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
		root_info::slot_free(iptr_ext[i], slot_ext[i], i + 1);
	}

	root_info::slot_free(iptr, slot, 0);

	s_tls_wait_cb(data, -1, stamp0);
}

template <bool NoAlert = false>
static u32 alert_sema(u32 cond_id, const void* data, u64 tid, u32 size, u128 mask, u128 phantom)
{
	ensure(cond_id);

	const auto cond = s_cond_list + cond_id;

	u32 ok = 0;

	if (!size ? (!tid || cond->tid == tid) : cmp_mask(size, mask, phantom, cond->size | (cond->flag << 8), cond->mask, cond->oldv))
	{
		// Redirect if necessary
		const auto _old = cond;
		const auto _new = _old->link ? cond_id_lock(_old->link, 0, u128(-1)) : _old;

		if (_new && _new->tsc0 == _old->tsc0)
		{
			if constexpr (NoAlert)
			{
				if (_new != _old)
				{
					// Keep base cond for actual alert attempt, free only secondary cond
					ok = ~_old->link;
					cond_free(cond_id);
					return ok;
				}
				else
				{
					ok = ~cond_id;
					return ok;
				}
			}
			else if ((!size && _new->forced_wakeup()) || (size && _new->wakeup(size == umax ? 1 : 2)))
			{
				ok = cond_id;
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

void atomic_wait_engine::set_wait_callback(bool(*cb)(const void*, u64, u64))
{
	if (cb)
	{
		s_tls_wait_cb = cb;
	}
	else
	{
		s_tls_wait_cb = s_null_wait_cb;
	}
}

void atomic_wait_engine::set_notify_callback(void(*cb)(const void*, u64))
{
	s_tls_notify_cb = cb;
}

bool atomic_wait_engine::raw_notify(const void* data, u64 thread_id)
{
	// Special operation mode. Note that this is not atomic.
	if (!data)
	{
		// Extract total amount of allocated bits (but hard to tell which level4 slots are occupied)
		const auto sem = s_cond_sem1.load();

		u32 total = 0;

		for (u32 i = 0; i < 8; i++)
		{
			if ((sem >> (i * 14)) & (8192 + 8191))
			{
				total = (i + 1) * 8192;
			}
		}

		// Special path: search thread_id without pointer information
		for (u32 i = 1; i <= total; i++)
		{
			if ((i & 63) == 0)
			{
				for (u64 bits = s_cond_bits[i / 64]; bits; bits &= bits - 1)
				{
					utils::prefetch_read(s_cond_list + i + std::countr_zero(bits));
				}
			}

			if (!s_cond_bits[i / 64])
			{
				i |= 63;
				continue;
			}

			if (~s_cond_bits[i / 64] & (1ull << i))
			{
				continue;
			}

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

	const uptr iptr = reinterpret_cast<uptr>(data) & (~s_ref_mask >> 17);

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, 0);

	u64 progress = 0;

	root_info::slot_search(iptr, 0, thread_id, u128(-1), [&](u32 cond_id)
	{
		// Forced notification
		if (alert_sema(cond_id, data, thread_id, 0, 0, 0))
		{
			if (s_tls_notify_cb)
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

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, -1);

	return progress != 0;
}

void atomic_wait_engine::notify_one(const void* data, u32 size, u128 mask, u128 new_value)
{
	const uptr iptr = reinterpret_cast<uptr>(data) & (~s_ref_mask >> 17);

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, 0);

	u64 progress = 0;

	root_info::slot_search(iptr, size, 0, mask, [&](u32 cond_id)
	{
		if (alert_sema(cond_id, data, -1, size, mask, new_value))
		{
			if (s_tls_notify_cb)
				s_tls_notify_cb(data, ++progress);
			return true;
		}

		return false;
	});

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, -1);
}

SAFE_BUFFERS void atomic_wait_engine::notify_all(const void* data, u32 size, u128 mask)
{
	const uptr iptr = reinterpret_cast<uptr>(data) & (~s_ref_mask >> 17);

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, 0);

	u64 progress = 0;

	// Array count for batch notification
	u32 count = 0;

	// Array itself.
	u32 cond_ids[max_threads * max_distance + 128];

	root_info::slot_search(iptr, size, 0, mask, [&](u32 cond_id)
	{
		u32 res = alert_sema<true>(cond_id, data, -1, size, mask, 0);

		if (res && ~res <= UINT16_MAX)
		{
			// Add to the end of the "stack"
			*(std::end(cond_ids) - ++count) = ~res;
		}

		return false;
	});

	// Try alert
	for (u32 i = 0; i < count; i++)
	{
		const u32 cond_id = *(std::end(cond_ids) - i - 1);

		if (!s_cond_list[cond_id].wakeup(1))
		{
			*(std::end(cond_ids) - i - 1) = ~cond_id;
		}
	}

	// Second stage (non-blocking alert attempts)
	if (count > 1)
	{
		for (u32 i = 0; i < count; i++)
		{
			const u32 cond_id = *(std::end(cond_ids) - i - 1);

			if (cond_id <= UINT16_MAX)
			{
				if (s_cond_list[cond_id].try_alert_native())
				{
					if (s_tls_notify_cb)
						s_tls_notify_cb(data, ++progress);
					*(std::end(cond_ids) - i - 1) = ~cond_id;
				}
			}
		}
	}

	// Final stage and cleanup
	for (u32 i = 0; i < count; i++)
	{
		const u32 cond_id = *(std::end(cond_ids) - i - 1);

		if (cond_id <= UINT16_MAX)
		{
			s_cond_list[cond_id].alert_native();
			if (s_tls_notify_cb)
				s_tls_notify_cb(data, ++progress);
			*(std::end(cond_ids) - i - 1) = ~cond_id;
		}
	}

	for (u32 i = 0; i < count; i++)
	{
		cond_free(~*(std::end(cond_ids) - i - 1));
	}

	if (s_tls_notify_cb)
		s_tls_notify_cb(data, -1);
}

namespace atomic_wait
{
	extern void parse_hashtable(bool(*cb)(u64 id, u32 refs, u64 ptr, u32 max_coll))
	{
		for (u64 i = 0; i < s_hashtable_size; i++)
		{
			const auto root = &s_hashtable[i];
			const auto slot = root->bits.load();

			if (cb(i, static_cast<u32>(slot.ref), slot.iptr, static_cast<u32>(slot.maxc)))
			{
				break;
			}
		}
	}
}
