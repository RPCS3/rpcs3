#include "atomic.hpp"

#if defined(__linux__)
#define USE_FUTEX
#elif !defined(_WIN32)
#define USE_STD
#endif

#ifdef _MSC_VER

#include "emmintrin.h"
#include "immintrin.h"

namespace utils
{
	u128 __vectorcall atomic_load16(const void* ptr)
	{
		return std::bit_cast<u128>(_mm_load_si128(static_cast<const __m128i*>(ptr)));
	}

	void __vectorcall atomic_store16(void* ptr, u128 value)
	{
		_mm_store_si128(static_cast<__m128i*>(ptr), std::bit_cast<__m128i>(value));
	}
}
#endif

#include "Utilities/sync.h"
#include "Utilities/StrFmt.h"

#ifdef __linux__
static bool has_waitv()
{
	static const bool s_has_waitv = []
	{
#ifndef ANDROID
		// FIXME: it produces SIGSYS signal
		syscall(SYS_futex_waitv, 0, 0, 0, 0, 0);
		return errno != ENOSYS;
#endif
		return false;
	}();

	return s_has_waitv;
}
#endif

#include <utility>
#include <cstdint>
#include <array>
#include <random>

#include "asm.hpp"
#include "endian.hpp"
#include "tsc.hpp"

// Total number of entries.
static constexpr usz s_hashtable_size = 1u << 17;

// Reference counter mask
static constexpr uptr s_ref_mask = 0xffff'ffff;

// Fix for silly on-first-use initializer
static bool s_null_wait_cb(const void*, u64, u64){ return true; };

// Callback for wait() function, returns false if wait should return
static thread_local bool(*s_tls_wait_cb)(const void* data, u64 attempts, u64 stamp0) = s_null_wait_cb;

// Callback for wait() function for a second custon condition, commonly passed with timeout
static thread_local bool(*s_tls_one_time_wait_cb)(u64 attempts) = nullptr;

// Compare data in memory with old value, and return true if they are equal
static NEVER_INLINE bool ptr_cmp(const void* data, u32 old, atomic_wait::info* ext = nullptr)
{
	// Check other wait variables if provided
	if (reinterpret_cast<const atomic_t<u32>*>(data)->load() == old)
	{
		if (ext) [[unlikely]]
		{
			for (auto e = ext; e->data; e++)
			{
				if (!ptr_cmp(e->data, e->old))
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
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
	struct alignas(64) cond_handle
	{
		struct fat_ptr
		{
			u64 ptr{};
			u32 reserved{};
			u32 ref_ctr{};

			auto operator<=>(const fat_ptr& other) const = default;
		};

		atomic_t<fat_ptr> ptr_ref;
		u64 tid;
		u32 oldv;

		u64 tsc0;
		u16 link;
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
#elif defined(ANDROID)
			tid = pthread_self();
#else
			tid = reinterpret_cast<u64>(pthread_self());
#endif

#ifdef USE_STD
			cv.init(cv);
			mtx.init(mtx);
#endif

			ensure(ptr_ref.exchange(fat_ptr{iptr, 0, 1}) == fat_ptr{});
		}

		void destroy()
		{
			tid = 0;
			tsc0 = 0;
			link = 0;
			sync.release(0);
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
static atomic_t<u64> s_cond_bits[65536 / 64]{1};

// Max allowed thread number is chosen to fit in 16 bits
static cond_handle s_cond_list[65536]{};

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
					s_cond_list[cond_id].ptr_ref.release(cond_handle::fat_ptr{0, 0, 1});
					cond_free(cond_id, -1);
				}
			}
		}
	};
}

// TLS storage for few allocaded "semaphores" to allow skipping initialization
static thread_local tls_cond_handler s_tls_conds{};

static u32 cond_alloc(uptr iptr, u32 tls_slot = -1)
{
	// Try to get cond from tls slot instead
	u16* ptls = tls_slot >= std::size(s_tls_conds.cond) ? nullptr : s_tls_conds.cond + tls_slot;

	if (ptls && *ptls) [[likely]]
	{
		// Fast reinitialize
		const u32 id = std::exchange(*ptls, 0);
		s_cond_list[id].ptr_ref.release(cond_handle::fat_ptr{iptr, 0, 1});
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

		// Set lowest clear bit
		const u64 bits = s_cond_bits[level3].fetch_op(AOFN(x |= x + 1, void()));

		// Find lowest clear bit (before it was set in fetch_op)
		const u32 id = level3 * 64 + std::countr_one(bits);

		// Initialize new "semaphore"
		s_cond_list[id].init(iptr);
		return id;
	}

	fmt::throw_exception("Thread semaphore limit (65535) reached in atomic wait.");
}

static void cond_free(u32 cond_id, u32 tls_slot = -1)
{
	if (cond_id - 1 >= u16{umax}) [[unlikely]]
	{
		fmt::throw_exception("bad id %u", cond_id);
	}

	const auto cond = s_cond_list + cond_id;

	// Dereference, destroy on last ref
	const bool last = cond->ptr_ref.atomic_op([](cond_handle::fat_ptr& val)
	{
		ensure(val.ref_ctr);

		val.ref_ctr--;

		if (val.ref_ctr == 0)
		{
			val = cond_handle::fat_ptr{};
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

	s_cond_sem3[level2].atomic_op(AOFN(x -= u128{1} << (level3 * 7)));
	s_cond_sem2[level1].atomic_op(AOFN(x -= u128{1} << (level2 * 11)));
	s_cond_sem1.atomic_op(AOFN(x -= u128{1} << (level1 * 14)));
}

static cond_handle* cond_id_lock(u32 cond_id, uptr iptr = 0)
{
	bool did_ref = false;

	if (cond_id - 1 >= u16{umax})
	{
		return nullptr;
	}

	const auto cond = s_cond_list + cond_id;

	while (true)
	{
		const auto [old, ok] = cond->ptr_ref.fetch_op([&](cond_handle::fat_ptr& val)
		{
			if (val == cond_handle::fat_ptr{} || val.ref_ctr == s_ref_mask)
			{
				// Don't reference already deallocated semaphore
				return false;
			}

			if (iptr && val.ptr != iptr)
			{
				// Pointer mismatch
				return false;
			}

			const u32 sync_val = cond->sync;

			if (sync_val == 0 || sync_val == 3)
			{
				return false;
			}

			if (!did_ref)
			{
				val.ref_ctr++;
			}

			return true;
		});

		if (ok)
		{
			// Check other fields again
			if (const u32 sync_val = cond->sync; sync_val == 0 || sync_val == 3)
			{
				did_ref = true;
				continue;
			}

			return cond;
		}

		if (old.ref_ctr == s_ref_mask)
		{
			fmt::throw_exception("Reference count limit (%u) reached in an atomic notifier.", s_ref_mask);
		}

		break;
	}

	if (did_ref)
	{
		cond_free(cond_id, -1);
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
		u64 prio: 8; // Reserved

		u64 ref : 16; // Ref counter
		u64 iptr: 64; // First pointer to use slot (to count used slots)
	};

	static_assert(sizeof(slot_allocator) == 16);

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
		static auto slot_search(uptr iptr, F func) noexcept;
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
		void advance() noexcept
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

u64 utils::get_unique_tsc()
{
	const u64 stamp0 = utils::get_tsc();

	if (!s_min_tsc.fetch_op([=](u64& tsc)
	{
		if (stamp0 <= tsc)
		{
			// Add 1 if new stamp is too old
			return false;
		}
		else
		{
			// Update last tsc with new stamp otherwise
			tsc = stamp0;
			return true;
		}
	}).second)
	{
		// Add 1 if new stamp is too old
		// Avoid doing it in the atomic operaion because, if it gets here it means there is already much cntention
		// So break the race (at least on x86)
		return s_min_tsc.add_fetch(1);
	}

	return stamp0;
}

atomic_t<u16>* root_info::slot_alloc(uptr ptr) noexcept
{
	atomic_t<u16>* slot = nullptr;

	u32 limit = 0;

	for (hash_engine _this(ptr);; _this.advance())
	{
		slot = _this->bits.atomic_op([&](slot_allocator& bits) -> atomic_t<u16>*
		{
			// Increment reference counter on every hashtable slot we attempt to allocate on
			if (bits.ref == u16{umax})
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

	for (hash_engine curr(iptr);; curr.advance())
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
FORCE_INLINE auto root_info::slot_search(uptr iptr, F func) noexcept
{
	u32 index = 0;
	[[maybe_unused]] u32 total = 0;

	for (hash_engine _this(iptr);; _this.advance())
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
			if (cond_id_lock(cond_ids[i], iptr))
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

SAFE_BUFFERS(void)
atomic_wait_engine::wait(const void* data, u32 old_value, u64 timeout, atomic_wait::info* ext)
{
	uint ext_size = 0;

#ifdef __linux__
	::timespec ts{};
	if (timeout + 1)
	{
		if (ext && ext->data) [[unlikely]]
		{
			// futex_waitv uses absolute timeout
			::clock_gettime(CLOCK_MONOTONIC, &ts);
		}

		ts.tv_sec += timeout / 1'000'000'000;
		ts.tv_nsec += timeout % 1'000'000'000;
		if (ts.tv_nsec >= 1'000'000'000)
		{
			ts.tv_sec++;
			ts.tv_nsec -= 1'000'000'000;
		}
	}

	futex_waitv vec[atomic_wait::max_list]{};
	vec[0].flags = FUTEX_32 | FUTEX_PRIVATE_FLAG;
	vec[0].uaddr = reinterpret_cast<__u64>(data);
	vec[0].val = old_value;

	if (ext) [[unlikely]]
	{
		for (auto e = ext; e->data; e++)
		{
			ext_size++;
			vec[ext_size].flags = FUTEX_32 | FUTEX_PRIVATE_FLAG;
			vec[ext_size].uaddr = reinterpret_cast<__u64>(e->data);
			vec[ext_size].val = e->old;
		}
	}

	if (ext_size && has_waitv()) [[unlikely]]
	{
		if (syscall(SYS_futex_waitv, +vec, ext_size + 1, 0, timeout + 1 ? &ts : nullptr, CLOCK_MONOTONIC) == -1)
		{
			if (errno == ENOSYS)
			{
				fmt::throw_exception("futex_waitv is not supported (Linux kernel is too old)");
			}
			if (errno == EINVAL)
			{
				fmt::throw_exception("futex_waitv: bad param");
			}
		}
		return;
	}
	else if (has_waitv())
	{
		if (futex(const_cast<void*>(data), FUTEX_WAIT_PRIVATE, old_value, timeout + 1 ? &ts : nullptr) == -1)
		{
			if (errno == EINVAL)
			{
				fmt::throw_exception("futex: bad param");
			}
		}
		return;
	}

	ext_size = 0;
#endif

	if (!s_tls_wait_cb(data, 0, 0))
	{
		return;
	}

	const auto stamp0 = utils::get_unique_tsc();

	const uptr iptr = reinterpret_cast<uptr>(data);

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

			iptr_ext[ext_size] = reinterpret_cast<uptr>(e->data);
			ext_size++;
		}
	}

	const u32 cond_id = cond_alloc(iptr, 0);

	u32 cond_id_ext[atomic_wait::max_list - 1]{};

	for (u32 i = 0; i < ext_size; i++)
	{
		cond_id_ext[i] = cond_alloc(iptr_ext[i], i + 1);
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
	cond->oldv = old_value;
	cond->tsc0 = stamp0;

	cond->sync.release(1);

	for (u32 i = 0; i < ext_size; i++)
	{
		// Extensions point to original cond_id, copy remaining info
		cond_ext[i]->link = cond_id;
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

	while (ptr_cmp(data, old_value, ext))
	{
		if (s_tls_one_time_wait_cb)
		{
			if (!s_tls_one_time_wait_cb(attempts))
			{
				break;
			}
		}

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
		if (!s_tls_wait_cb(data, ++attempts, stamp0))
		{
			break;
		}

		if (timeout + 1)
		{
			if (s_tls_one_time_wait_cb)
			{
				// The condition of the callback overrides timeout escape because it makes little sense to do so when a custom condition is passed
				continue;
			}

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
		root_info::slot_free(iptr_ext[i], slot_ext[i], i + 1);
	}

	root_info::slot_free(iptr, slot, 0);

	s_tls_wait_cb(data, -1, stamp0);
	s_tls_one_time_wait_cb = nullptr;
}

template <bool NoAlert = false>
static u32 alert_sema(u32 cond_id, u32 size)
{
	ensure(cond_id);

	const auto cond = s_cond_list + cond_id;

	u32 ok = 0;

	if (true)
	{
		// Redirect if necessary
		const auto _old = cond;
		const auto _new = _old->link ? cond_id_lock(_old->link) : _old;

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
			else if (_new->wakeup(size ? 1 : 2))
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

void atomic_wait_engine::set_one_time_use_wait_callback(bool(*cb)(u64 progress))
{
	s_tls_one_time_wait_cb = cb;
}

void atomic_wait_engine::notify_one(const void* data)
{
#ifdef __linux__
	if (has_waitv())
	{
		futex(const_cast<void*>(data), FUTEX_WAKE_PRIVATE, 1);
		return;
	}
#endif
	const uptr iptr = reinterpret_cast<uptr>(data);

	root_info::slot_search(iptr, [&](u32 cond_id)
	{
		if (alert_sema(cond_id, 4))
		{
			return true;
		}

		return false;
	});
}

SAFE_BUFFERS(void)
atomic_wait_engine::notify_all(const void* data)
{
#ifdef __linux__
	if (has_waitv())
	{
		futex(const_cast<void*>(data), FUTEX_WAKE_PRIVATE, INT_MAX);
		return;
	}
#endif
	const uptr iptr = reinterpret_cast<uptr>(data);

	// Array count for batch notification
	u32 count = 0;

	// Array itself.
	u32 cond_ids[128];

	root_info::slot_search(iptr, [&](u32 cond_id)
	{
		if (count >= 128)
		{
			// Unusual big amount of sema: fallback to notify_one alg
			alert_sema(cond_id, 4);
			return false;
		}

		u32 res = alert_sema<true>(cond_id, 4);

		if (~res <= u16{umax})
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

			if (cond_id <= u16{umax})
			{
				if (s_cond_list[cond_id].try_alert_native())
				{
					*(std::end(cond_ids) - i - 1) = ~cond_id;
				}
			}
		}
	}

	// Final stage and cleanup
	for (u32 i = 0; i < count; i++)
	{
		const u32 cond_id = *(std::end(cond_ids) - i - 1);

		if (cond_id <= u16{umax})
		{
			s_cond_list[cond_id].alert_native();
			*(std::end(cond_ids) - i - 1) = ~cond_id;
		}
	}

	for (u32 i = 0; i < count; i++)
	{
		cond_free(~*(std::end(cond_ids) - i - 1));
	}
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
