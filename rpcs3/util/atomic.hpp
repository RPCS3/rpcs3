#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"
#include <functional>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)

extern "C"
{
	void _ReadWriteBarrier();
	void* _AddressOfReturnAddress();

	uchar _bittest(const long*, long);
	uchar _interlockedbittestandset(volatile long*, long);
	uchar _interlockedbittestandreset(volatile long*, long);

	char _InterlockedCompareExchange8(volatile char*, char, char);
	char _InterlockedExchange8(volatile char*, char);
	char _InterlockedExchangeAdd8(volatile char*, char);
	char _InterlockedAnd8(volatile char*, char);
	char _InterlockedOr8(volatile char*, char);
	char _InterlockedXor8(volatile char*, char);

	short _InterlockedCompareExchange16(volatile short*, short, short);
	short _InterlockedExchange16(volatile short*, short);
	short _InterlockedExchangeAdd16(volatile short*, short);
	short _InterlockedAnd16(volatile short*, short);
	short _InterlockedOr16(volatile short*, short);
	short _InterlockedXor16(volatile short*, short);
	short _InterlockedIncrement16(volatile short*);
	short _InterlockedDecrement16(volatile short*);

	long _InterlockedCompareExchange(volatile long*, long, long);
	long _InterlockedCompareExchange_HLEAcquire(volatile long*, long, long);
	long _InterlockedExchange(volatile long*, long);
	long _InterlockedExchangeAdd(volatile long*, long);
	long _InterlockedExchangeAdd_HLERelease(volatile long*, long);
	long _InterlockedAnd(volatile long*, long);
	long _InterlockedOr(volatile long*, long);
	long _InterlockedXor(volatile long*, long);
	long _InterlockedIncrement(volatile long*);
	long _InterlockedDecrement(volatile long*);

	s64 _InterlockedCompareExchange64(volatile s64*, s64, s64);
	s64 _InterlockedCompareExchange64_HLEAcquire(volatile s64*, s64, s64);
	s64 _InterlockedExchange64(volatile s64*, s64);
	s64 _InterlockedExchangeAdd64(volatile s64*, s64);
	s64 _InterlockedExchangeAdd64_HLERelease(volatile s64*, s64);
	s64 _InterlockedAnd64(volatile s64*, s64);
	s64 _InterlockedOr64(volatile s64*, s64);
	s64 _InterlockedXor64(volatile s64*, s64);
	s64 _InterlockedIncrement64(volatile s64*);
	s64 _InterlockedDecrement64(volatile s64*);

	uchar _InterlockedCompareExchange128(volatile s64*, s64, s64, s64*);
}

namespace utils
{
	u128 __vectorcall atomic_load16(const void*);
	void __vectorcall atomic_store16(void*, u128);
}
#endif

FORCE_INLINE void atomic_fence_consume()
{
#if defined(_M_X64) && defined(_MSC_VER)
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_CONSUME);
#endif
}

FORCE_INLINE void atomic_fence_acquire()
{
#if defined(_M_X64) && defined(_MSC_VER)
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_ACQUIRE);
#endif
}

FORCE_INLINE void atomic_fence_release()
{
#if defined(_M_X64) && defined(_MSC_VER)
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_RELEASE);
#endif
}

FORCE_INLINE void atomic_fence_acq_rel()
{
#if defined(_M_X64) && defined(_MSC_VER)
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_ACQ_REL);
#endif
}

FORCE_INLINE void atomic_fence_seq_cst()
{
#if defined(_M_X64) && defined(_MSC_VER)
	_ReadWriteBarrier();
	_InterlockedOr(static_cast<long*>(_AddressOfReturnAddress()), 0);
	_ReadWriteBarrier();
#elif defined(ARCH_X64)
	__asm__ volatile ("lock orl $0, 0(%%rsp);" ::: "cc", "memory");
#else
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#endif
}

#if defined(_M_X64) && defined(_MSC_VER)
#pragma warning(pop)
#endif

// Wait timeout extension (in nanoseconds)
enum class atomic_wait_timeout : u64
{
	inf = 0xffffffffffffffff,
};

template <typename T>
class lf_queue;

namespace stx
{
	template <typename T>
	class atomic_ptr;
}

// Various extensions for atomic_t::wait
namespace atomic_wait
{
	// Max number of simultaneous atomic variables to wait on (can be extended if really necessary)
	constexpr uint max_list = 8;

	constexpr struct any_value_t
	{
		template <typename T>
		operator T() const noexcept
		{
			return T();
		}
	} any_value;

	struct info
	{
		const void* data;
		u32 old;
	};

	template <uint Max, typename... T>
	class list
	{
		static_assert(Max <= max_list, "Too many elements in the atomic wait list.");

		// Null-terminated list of wait info
		info m_info[Max + 1]{};

	public:
		constexpr list() noexcept = default;

		constexpr list(const list&) noexcept = default;

		constexpr list& operator=(const list&) noexcept = default;

		template <typename... U>
			requires(requires(U& u) { u.wait(any_value); } && ...)
		constexpr list(U&... vars)
			: m_info{{&vars, 0}...}
		{
			static_assert(sizeof...(U) == Max, "Inconsistent amount of atomics.");
		}

		template <typename... U>
		constexpr list& values(U... values)
		{
			static_assert(sizeof...(U) == Max, "Inconsistent amount of values.");

			auto* ptr = m_info;
			(((ptr->old = std::bit_cast<u32>(values)), ptr++), ...);
			return *this;
		}

		template <uint Index, typename T2, typename U>
			requires(requires(T2& t2) { t2.wait(any_value); })
		constexpr void set(T2& var, U value)
		{
			static_assert(Index < Max);

			m_info[Index].data = &var;
			m_info[Index].old = std::bit_cast<u32>(value);
		}

		template <uint Index, typename T2>
		constexpr void set(lf_queue<T2>& var, std::nullptr_t = nullptr)
		{
			static_assert(Index < Max);
			static_assert(sizeof(var) == sizeof(uptr) * 2);

			m_info[Index].data = std::bit_cast<char*>(&var.get_wait_atomic().raw());
			m_info[Index].old = 0;
		}

		template <uint Index, typename T2>
		constexpr void set(stx::atomic_ptr<T2>& var, std::nullptr_t = nullptr)
		{
			static_assert(Index < Max);
			static_assert(sizeof(var) == sizeof(uptr) * 2);

			m_info[Index].data = std::bit_cast<char*>(&var.get_wait_atomic().raw());
			m_info[Index].old = 0;
		}

		// Timeout is discouraged
		void wait(atomic_wait_timeout timeout = atomic_wait_timeout::inf);

		// Same as wait
		void start()
		{
			wait();
		}
	};

	template <typename... T>
		requires(requires(T& t) { t.wait(any_value); } && ...)
	list(T&... vars) -> list<sizeof...(T), T...>;
}

namespace utils
{
	// RDTSC with adjustment for being unique
	u64 get_unique_tsc();
}

// Helper for waitable atomics (as in C++20 std::atomic)
struct atomic_wait_engine
{
private:
	template <typename T, usz Align>
	friend class atomic_t;

	template <uint Max, typename... T>
	friend class atomic_wait::list;

	static void wait(const void* data, u32 old_value, u64 timeout, atomic_wait::info* ext = nullptr);

public:
	static void notify_one(const void* data);
	static void notify_all(const void* data);

	static void set_wait_callback(bool(*cb)(const void* data, u64 attempts, u64 stamp0));
	static void set_one_time_use_wait_callback(bool (*cb)(u64 progress));
};

template <uint Max, typename... T>
void atomic_wait::list<Max, T...>::wait(atomic_wait_timeout timeout)
{
	static_assert(!!Max, "Cannot initiate atomic wait with empty list.");

	atomic_wait_engine::wait(m_info[0].data, m_info[0].old, static_cast<u64>(timeout), m_info + 1);
}

// Helper class, provides access to compiler-specific atomic intrinsics
template <typename T, usz Size = sizeof(T)>
struct atomic_storage
{
	/* First part: Non-MSVC intrinsics */

	using type = get_uint_t<sizeof(T)>;

#if !defined(_MSC_VER) || !defined(_M_X64)

#if defined(__ATOMIC_HLE_ACQUIRE) && defined(__ATOMIC_HLE_RELEASE)
	static constexpr int s_hle_ack = __ATOMIC_SEQ_CST | __ATOMIC_HLE_ACQUIRE;
	static constexpr int s_hle_rel = __ATOMIC_SEQ_CST | __ATOMIC_HLE_RELEASE;
#else
	static constexpr int s_hle_ack = __ATOMIC_SEQ_CST;
	static constexpr int s_hle_rel = __ATOMIC_SEQ_CST;
#endif

// clang often thinks atomics are misaligned, GCC doesn't like reinterpret_cast for breaking strict aliasing
#ifdef __clang__
#define MAYBE_CAST(...) (reinterpret_cast<type*>(__VA_ARGS__))
#else
#define MAYBE_CAST(...) (__VA_ARGS__)
#endif

	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		return __atomic_compare_exchange(MAYBE_CAST(&dest), MAYBE_CAST(&comp), MAYBE_CAST(&exch), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		static_assert(sizeof(T) == 4 || sizeof(T) == 8);
		return __atomic_compare_exchange(MAYBE_CAST(&dest), MAYBE_CAST(&comp), MAYBE_CAST(&exch), false, s_hle_ack, s_hle_ack);
	}

	static inline T load(const T& dest)
	{
#ifdef __clang__
		type result;
		__atomic_load(reinterpret_cast<const type*>(&dest), MAYBE_CAST(&result), __ATOMIC_SEQ_CST);
		return std::bit_cast<T>(result);
#else
		alignas(sizeof(T)) T result;
		__atomic_load(&dest, &result, __ATOMIC_SEQ_CST);
		return result;
#endif
	}

	static inline T observe(const T& dest)
	{
#ifdef __clang__
		type result;
		__atomic_load(reinterpret_cast<const type*>(&dest), MAYBE_CAST(&result), __ATOMIC_RELAXED);
		return std::bit_cast<T>(result);
#else
		alignas(sizeof(T)) T result;
		__atomic_load(&dest, &result, __ATOMIC_RELAXED);
		return result;
#endif
	}

	static inline void store(T& dest, T value)
	{
		static_cast<void>(exchange(dest, value));
	}

	static inline void release(T& dest, T value)
	{
		__atomic_store(MAYBE_CAST(&dest), MAYBE_CAST(&value), __ATOMIC_RELEASE);
	}

	static inline T exchange(T& dest, T value)
	{
		alignas(sizeof(T)) T result;
		__atomic_exchange(MAYBE_CAST(&dest), MAYBE_CAST(&value), MAYBE_CAST(&result), __ATOMIC_SEQ_CST);
		return result;
	}

	static inline T fetch_add(T& dest, T value)
	{
		return __atomic_fetch_add(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T fetch_add_hle_rel(T& dest, T value)
	{
		static_assert(sizeof(T) == 4 || sizeof(T) == 8);
		return __atomic_fetch_add(&dest, value, s_hle_rel);
	}

	static inline T add_fetch(T& dest, T value)
	{
		return __atomic_add_fetch(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T fetch_sub(T& dest, T value)
	{
		return __atomic_fetch_sub(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T sub_fetch(T& dest, T value)
	{
		return __atomic_sub_fetch(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T fetch_and(T& dest, T value)
	{
		return __atomic_fetch_and(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T and_fetch(T& dest, T value)
	{
		return __atomic_and_fetch(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T fetch_xor(T& dest, T value)
	{
		return __atomic_fetch_xor(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T xor_fetch(T& dest, T value)
	{
		return __atomic_xor_fetch(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T fetch_or(T& dest, T value)
	{
		return __atomic_fetch_or(&dest, value, __ATOMIC_SEQ_CST);
	}

	static inline T or_fetch(T& dest, T value)
	{
		return __atomic_or_fetch(&dest, value, __ATOMIC_SEQ_CST);
	}
#endif

	/* Second part: MSVC-specific */

#if defined(_M_X64) && defined(_MSC_VER)
	static inline T add_fetch(T& dest, T value)
	{
		return atomic_storage<T>::fetch_add(dest, value) + value;
	}

	static inline T fetch_sub(T& dest, T value)
	{
		return atomic_storage<T>::fetch_add(dest, 0 - value);
	}

	static inline T sub_fetch(T& dest, T value)
	{
		return atomic_storage<T>::fetch_add(dest, 0 - value) - value;
	}

	static inline T and_fetch(T& dest, T value)
	{
		return atomic_storage<T>::fetch_and(dest, value) & value;
	}

	static inline T or_fetch(T& dest, T value)
	{
		return atomic_storage<T>::fetch_or(dest, value) | value;
	}

	static inline T xor_fetch(T& dest, T value)
	{
		return atomic_storage<T>::fetch_xor(dest, value) ^ value;
	}
#undef MAYBE_CAST
#endif

	/* Third part: fallbacks, may be hidden by subsequent atomic_storage<> specializations */

	static inline T fetch_inc(T& dest)
	{
		return atomic_storage<T>::fetch_add(dest, 1);
	}

	static inline T inc_fetch(T& dest)
	{
		return atomic_storage<T>::add_fetch(dest, 1);
	}

	static inline T fetch_dec(T& dest)
	{
		return atomic_storage<T>::fetch_sub(dest, 1);
	}

	static inline T dec_fetch(T& dest)
	{
		return atomic_storage<T>::sub_fetch(dest, 1);
	}

	static inline bool bts(T& dest, uint bit)
	{
#if defined(ARCH_X64)
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}
#endif

#if defined(_M_X64) && defined(_MSC_VER)
		return _interlockedbittestandset(reinterpret_cast<long*>(dst), bit) != 0;
#elif defined(ARCH_X64)
		bool result;
		__asm__ volatile ("lock btsl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#else
		const T value = static_cast<T>(1) << bit;
		return (__atomic_fetch_or(&dest, value, __ATOMIC_SEQ_CST) & value) != 0;
#endif
	}

	static inline bool btr(T& dest, uint bit)
	{
#if defined(ARCH_X64)
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}
#endif

#if defined(_M_X64) && defined(_MSC_VER)
		return _interlockedbittestandreset(reinterpret_cast<long*>(dst), bit) != 0;
#elif defined(ARCH_X64)
		bool result;
		__asm__ volatile ("lock btrl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#else
		const T value = static_cast<T>(1) << bit;
		return (__atomic_fetch_and(&dest, ~value, __ATOMIC_SEQ_CST) & value) != 0;
#endif
	}

	static inline bool btc(T& dest, uint bit)
	{
#if defined(ARCH_X64)
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}
#endif

#if defined(_M_X64) && defined(_MSC_VER)
		while (true)
		{
			// Keep trying until we actually invert desired bit
			if (!_bittest(reinterpret_cast<const long*>(dst), bit) && !_interlockedbittestandset(reinterpret_cast<long*>(dst), bit))
				return false;
			if (_interlockedbittestandreset(reinterpret_cast<long*>(dst), bit))
				return true;
		}
#elif defined(ARCH_X64)
		bool result;
		__asm__ volatile ("lock btcl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#else
		const T value = static_cast<T>(1) << bit;
		return (__atomic_fetch_xor(&dest, value, __ATOMIC_SEQ_CST) & value) != 0;
#endif
	}
};

/* The rest: ugly MSVC intrinsics + inline asm implementations */

template <typename T>
struct atomic_storage<T, 1> : atomic_storage<T, 0>
{
#if defined(_M_X64) && defined(_MSC_VER)
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		const char v = std::bit_cast<char>(comp);
		const char r = _InterlockedCompareExchange8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline T load(const T& dest)
	{
		atomic_fence_acquire();
		const char value = *reinterpret_cast<const volatile char*>(&dest);
		atomic_fence_acquire();
		return std::bit_cast<T>(value);
	}

	static inline T observe(const T& dest)
	{
		const char value = *reinterpret_cast<const volatile char*>(&dest);
		return std::bit_cast<T>(value);
	}

	static inline void release(T& dest, T value)
	{
		atomic_fence_release();
		*reinterpret_cast<volatile char*>(&dest) = std::bit_cast<char>(value);
		atomic_fence_release();
	}

	static inline T exchange(T& dest, T value)
	{
		const char r = _InterlockedExchange8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(value));
		return std::bit_cast<T>(r);
	}

	static inline void store(T& dest, T value)
	{
		exchange(dest, value);
	}

	static inline T fetch_add(T& dest, T value)
	{
		const char r = _InterlockedExchangeAdd8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_and(T& dest, T value)
	{
		const char r = _InterlockedAnd8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_or(T& dest, T value)
	{
		const char r = _InterlockedOr8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_xor(T& dest, T value)
	{
		const char r = _InterlockedXor8(reinterpret_cast<volatile char*>(&dest), std::bit_cast<char>(value));
		return std::bit_cast<T>(r);
	}
#endif
};

template <typename T>
struct atomic_storage<T, 2> : atomic_storage<T, 0>
{
#if defined(_M_X64) && defined(_MSC_VER)
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		const short v = std::bit_cast<short>(comp);
		const short r = _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline T load(const T& dest)
	{
		atomic_fence_acquire();
		const short value = *reinterpret_cast<const volatile short*>(&dest);
		atomic_fence_acquire();
		return std::bit_cast<T>(value);
	}

	static inline T observe(const T& dest)
	{
		const short value = *reinterpret_cast<const volatile short*>(&dest);
		return std::bit_cast<T>(value);
	}

	static inline void release(T& dest, T value)
	{
		atomic_fence_release();
		*reinterpret_cast<volatile short*>(&dest) = std::bit_cast<short>(value);
		atomic_fence_release();
	}

	static inline T exchange(T& dest, T value)
	{
		const short r = _InterlockedExchange16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(value));
		return std::bit_cast<T>(r);
	}

	static inline void store(T& dest, T value)
	{
		exchange(dest, value);
	}

	static inline T fetch_add(T& dest, T value)
	{
		const short r = _InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_and(T& dest, T value)
	{
		const short r = _InterlockedAnd16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_or(T& dest, T value)
	{
		const short r = _InterlockedOr16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_xor(T& dest, T value)
	{
		const short r = _InterlockedXor16(reinterpret_cast<volatile short*>(&dest), std::bit_cast<short>(value));
		return std::bit_cast<T>(r);
	}

	static inline T inc_fetch(T& dest)
	{
		const short r = _InterlockedIncrement16(reinterpret_cast<volatile short*>(&dest));
		return std::bit_cast<T>(r);
	}

	static inline T dec_fetch(T& dest)
	{
		const short r = _InterlockedDecrement16(reinterpret_cast<volatile short*>(&dest));
		return std::bit_cast<T>(r);
	}
#endif
};

template <typename T>
struct atomic_storage<T, 4> : atomic_storage<T, 0>
{
#if defined(_M_X64) && defined(_MSC_VER)
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		const long v = std::bit_cast<long>(comp);
		const long r = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		const long v = std::bit_cast<long>(comp);
		const long r = _InterlockedCompareExchange_HLEAcquire(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline T load(const T& dest)
	{
		atomic_fence_acquire();
		const long value = *reinterpret_cast<const volatile long*>(&dest);
		atomic_fence_acquire();
		return std::bit_cast<T>(value);
	}

	static inline T observe(const T& dest)
	{
		const long value = *reinterpret_cast<const volatile long*>(&dest);
		return std::bit_cast<T>(value);
	}

	static inline void release(T& dest, T value)
	{
		atomic_fence_release();
		*reinterpret_cast<volatile long*>(&dest) = std::bit_cast<long>(value);
		atomic_fence_release();
	}

	static inline T exchange(T& dest, T value)
	{
		const long r = _InterlockedExchange(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline void store(T& dest, T value)
	{
		exchange(dest, value);
	}

	static inline T fetch_add(T& dest, T value)
	{
		const long r = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_add_hle_rel(T& dest, T value)
	{
		const long r = _InterlockedExchangeAdd_HLERelease(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_and(T& dest, T value)
	{
		long r = _InterlockedAnd(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_or(T& dest, T value)
	{
		const long r = _InterlockedOr(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_xor(T& dest, T value)
	{
		const long r = _InterlockedXor(reinterpret_cast<volatile long*>(&dest), std::bit_cast<long>(value));
		return std::bit_cast<T>(r);
	}

	static inline T inc_fetch(T& dest)
	{
		const long r = _InterlockedIncrement(reinterpret_cast<volatile long*>(&dest));
		return std::bit_cast<T>(r);
	}

	static inline T dec_fetch(T& dest)
	{
		const long r = _InterlockedDecrement(reinterpret_cast<volatile long*>(&dest));
		return std::bit_cast<T>(r);
	}
#endif
};

template <typename T>
struct atomic_storage<T, 8> : atomic_storage<T, 0>
{
#if defined(_M_X64) && defined(_MSC_VER)
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		const llong v = std::bit_cast<llong>(comp);
		const llong r = _InterlockedCompareExchange64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		const llong v = std::bit_cast<llong>(comp);
		const llong r = _InterlockedCompareExchange64_HLEAcquire(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(exch), v);
		comp = std::bit_cast<T>(r);
		return r == v;
	}

	static inline T load(const T& dest)
	{
		atomic_fence_acquire();
		const llong value = *reinterpret_cast<const volatile llong*>(&dest);
		atomic_fence_acquire();
		return std::bit_cast<T>(value);
	}

	static inline T observe(const T& dest)
	{
		const llong value = *reinterpret_cast<const volatile llong*>(&dest);
		return std::bit_cast<T>(value);
	}

	static inline void release(T& dest, T value)
	{
		atomic_fence_release();
		*reinterpret_cast<volatile llong*>(&dest) = std::bit_cast<llong>(value);
		atomic_fence_release();
	}

	static inline T exchange(T& dest, T value)
	{
		const llong r = _InterlockedExchange64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline void store(T& dest, T value)
	{
		exchange(dest, value);
	}

	static inline T fetch_add(T& dest, T value)
	{
		const llong r = _InterlockedExchangeAdd64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_add_hle_rel(T& dest, T value)
	{
		const llong r = _InterlockedExchangeAdd64_HLERelease(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_and(T& dest, T value)
	{
		const llong r = _InterlockedAnd64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_or(T& dest, T value)
	{
		const llong r = _InterlockedOr64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline T fetch_xor(T& dest, T value)
	{
		const llong r = _InterlockedXor64(reinterpret_cast<volatile llong*>(&dest), std::bit_cast<llong>(value));
		return std::bit_cast<T>(r);
	}

	static inline T inc_fetch(T& dest)
	{
		const llong r = _InterlockedIncrement64(reinterpret_cast<volatile llong*>(&dest));
		return std::bit_cast<T>(r);
	}

	static inline T dec_fetch(T& dest)
	{
		const llong r = _InterlockedDecrement64(reinterpret_cast<volatile llong*>(&dest));
		return std::bit_cast<T>(r);
	}
#endif
};

template <typename T>
struct atomic_storage<T, 16> : atomic_storage<T, 0>
{
#if defined(_M_X64) && defined(_MSC_VER)
	static inline T load(const T& dest)
	{
		atomic_fence_acquire();
		u128 val = utils::atomic_load16(&dest);
		atomic_fence_acquire();
		return std::bit_cast<T>(val);
	}

	static inline T observe(const T& dest)
	{
		return load(dest);
	}

	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		struct alignas(16) llong2 { llong ll[2]; };
		const llong2 _exch = std::bit_cast<llong2>(exch);
		return _InterlockedCompareExchange128(reinterpret_cast<volatile llong*>(&dest), _exch.ll[1], _exch.ll[0], reinterpret_cast<llong*>(&comp)) != 0;
	}

	static inline T exchange(T& dest, T value)
	{
		struct alignas(16) llong2 { llong ll[2]; };
		const llong2 _value = std::bit_cast<llong2>(value);

		const auto llptr = reinterpret_cast<volatile llong*>(&dest);
		llong2 cmp{ llptr[0], llptr[1] };
		while (!_InterlockedCompareExchange128(llptr, _value.ll[1], _value.ll[0], cmp.ll));
		return std::bit_cast<T>(cmp);
	}

	static inline void store(T& dest, T value)
	{
		atomic_fence_acq_rel();
		release(dest, value);
		atomic_fence_seq_cst();
	}

	static inline void release(T& dest, T value)
	{
		atomic_fence_release();
		utils::atomic_store16(&dest, std::bit_cast<u128>(value));
		atomic_fence_release();
	}
#elif defined(ARCH_X64)
	static inline T load(const T& dest)
	{
		alignas(16) T r;
#ifdef __AVX__
		__asm__ volatile("vmovdqa %1, %0;" : "=x" (r) : "m" (dest) : "memory");
#else
		__asm__ volatile("movdqa %1, %0;" : "=x" (r) : "m" (dest) : "memory");
#endif
		return r;
	}

	static inline T observe(const T& dest)
	{
		return load(dest);
	}

	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		bool result;
		ullong cmp_lo = 0;
		ullong cmp_hi = 0;
		ullong exc_lo = 0;
		ullong exc_hi = 0;

		if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, s128>)
		{
			cmp_lo = comp;
			cmp_hi = comp >> 64;
			exc_lo = exch;
			exc_hi = exch >> 64;
		}
		else
		{
			std::memcpy(&cmp_lo, reinterpret_cast<char*>(&comp) + 0, 8);
			std::memcpy(&cmp_hi, reinterpret_cast<char*>(&comp) + 8, 8);
			std::memcpy(&exc_lo, reinterpret_cast<char*>(&exch) + 0, 8);
			std::memcpy(&exc_hi, reinterpret_cast<char*>(&exch) + 8, 8);
		}

		__asm__ volatile("lock cmpxchg16b %1;"
			: "=@ccz" (result)
			, "+m" (dest)
			, "+d" (cmp_hi)
			, "+a" (cmp_lo)
			: "c" (exc_hi)
			, "b" (exc_lo)
			: "cc");

		if constexpr (std::is_same_v<T, u128> || std::is_same_v<T, s128>)
		{
			comp = T{cmp_hi} << 64 | cmp_lo;
		}
		else
		{
			std::memcpy(reinterpret_cast<char*>(&comp) + 0, &cmp_lo, 8);
			std::memcpy(reinterpret_cast<char*>(&comp) + 8, &cmp_hi, 8);
		}

		return result;
	}

	static inline T exchange(T& dest, T value)
	{
		__atomic_thread_fence(__ATOMIC_ACQ_REL);
		return std::bit_cast<T>(__sync_lock_test_and_set(reinterpret_cast<u128*>(&dest), std::bit_cast<u128>(value)));
	}

	static inline void store(T& dest, T value)
	{
		release(dest, value);
		atomic_fence_seq_cst();
	}

	static inline void release(T& dest, T value)
	{
		u128 val = std::bit_cast<u128>(value);
#ifdef __AVX__
		__asm__ volatile("vmovdqa %0, %1;" :: "x" (val), "m" (dest) : "memory");
#else
		__asm__ volatile("movdqa %0, %1;" :: "x" (val), "m" (dest) : "memory");
#endif
	}
#elif defined(ARCH_ARM64)

	static inline T load(const T& dest)
	{
#if defined(ARM_FEATURE_LSE2)
		u64 data[2];
		__asm__ volatile("1:\n"
			"ldp %x[data0], %x[data1], %[dest]\n"
			"dmb ish\n"
			: [data0] "=r"(data[0]), [data1] "=r"(data[1])
			: [dest] "Q"(dest)
			: "memory");
		T result;
		std::memcpy(&result, data, 16);
		return result;
#else
		u32 tmp;
		u64 data[2];
		__asm__ volatile("1:\n"
			"ldaxp %x[data0], %x[data1], %[dest]\n"
			"stlxp %w[tmp], %x[data0], %x[data1], %[dest]\n"
			"cbnz %w[tmp], 1b\n"
			: [tmp] "=&r" (tmp), [data0] "=&r" (data[0]), [data1] "=&r" (data[1])
			: [dest] "Q" (dest)
			: "memory"
		);
		T result;
		std::memcpy(&result, data, 16);
		return result;
#endif
	}

	static inline T observe(const T& dest)
	{
		// TODO
		return load(dest);
	}

	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		bool result;
		u64 cmp[2];
		std::memcpy(cmp, &comp, 16);
		u64 data[2];
		std::memcpy(data, &exch, 16);
		u64 prev[2];
		__asm__ volatile("1:\n"
			"ldaxp %x[prev0], %x[prev1], %[storage]\n"
			"cmp %x[prev0], %x[cmp0]\n"
			"ccmp %x[prev1], %x[cmp1], #0, eq\n"
			"b.ne 2f\n"
			"stlxp %w[result], %x[data0], %x[data1], %[storage]\n"
			"cbnz %w[result], 1b\n"
			"2:\n"
			"cset %w[result], eq\n"
			: [result] "=&r" (result), [storage] "+Q" (dest), [prev0] "=&r" (prev[0]), [prev1] "=&r" (prev[1])
			: [data0] "r" (data[0]), [data1] "r" (data[1]), [cmp0] "r" (cmp[0]), [cmp1] "r" (cmp[1])
			: "cc", "memory"
		);

		if (result)
		{
			return true;
		}

		std::memcpy(&comp, prev, 16);
		return false;
	}

	static inline T exchange(T& dest, T value)
	{
		u32 tmp;
		u64 src[2];
		u64 data[2];
		std::memcpy(src, &value, 16);
		__asm__ volatile("1:\n"
			"ldaxp %x[data0], %x[data1], %[dest]\n"
			"stlxp %w[tmp], %x[src0], %x[src1], %[dest]\n"
			"cbnz %w[tmp], 1b\n"
			: [tmp] "=&r" (tmp), [dest] "+Q" (dest), [data0] "=&r" (data[0]), [data1] "=&r" (data[1])
			: [src0] "r" (src[0]), [src1] "r" (src[1])
			: "memory"
		);
		T result;
		std::memcpy(&result, data, 16);
		return result;
	}

	static inline void store(T& dest, T value)
	{
		// TODO
#if defined(ARM_FEATURE_LSE2)
		u64 src[2];
		std::memcpy(src, &value, 16);
		__asm__ volatile("1:\n"
			"dmb ish\n"
			"stp %x[data0], %x[data1], %[dest]\n"
			"dmb ish\n"
			: [dest] "=Q" (dest)
			: [data0] "r" (src[0]), [data1] "r" (src[1])
			: "memory"
		);
#else
		exchange(dest, value);
#endif
	}

	static inline void release(T& dest, T value)
	{
#if defined(ARM_FEATURE_LSE2)
		u64 src[2];
		std::memcpy(src, &value, 16);
		__asm__ volatile("1:\n"
			 "dmb ish\n"
			 "stp %x[data0], %x[data1], %[dest]\n"
			 : [dest] "=Q" (dest)
			 : [data0] "r" (src[0]), [data1] "r" (src[1])
			 : "memory"
		);
#else
		// TODO
		exchange(dest, value);
#endif
	}
#endif

	// TODO
};

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

// Atomic type with lock-free and standard layout guarantees (and appropriate limitations)
template <typename T, usz Align = sizeof(T)>
class atomic_t
{
protected:
	using type = std::remove_cv_t<T>;

	using ptr_rt = std::conditional_t<std::is_pointer_v<type>, ullong, type>;

	static_assert((Align & (Align - 1)) == 0, "atomic_t<> error: unexpected Align parameter (not power of 2).");
	static_assert(Align % sizeof(type) == 0, "atomic_t<> error: invalid type, must be power of 2.");
	static_assert(sizeof(type) <= 16, "atomic_t<> error: invalid type, too big (max supported size is 16).");
	static_assert(Align >= sizeof(type), "atomic_t<> error: bad args, specify bigger alignment if necessary.");

	static_assert(std::is_trivially_copyable_v<type>);
	static_assert(std::is_copy_constructible_v<type>);
	static_assert(std::is_move_constructible_v<type>);
	static_assert(std::is_copy_assignable_v<type>);
	static_assert(std::is_move_assignable_v<type>);

	alignas(Align) type m_data;

public:
	static constexpr usz align = Align;
	ENABLE_BITWISE_SERIALIZATION;

	atomic_t() noexcept = default;

	atomic_t(const atomic_t&) = delete;

	atomic_t& operator =(const atomic_t&) = delete;

	constexpr atomic_t(const type& value) noexcept
		: m_data(value)
	{
	}

	// Unsafe direct access
	type& raw()
	{
		return m_data;
	}

	// Unsafe direct access
	const type& raw() const
	{
		return m_data;
	}

	// Atomically compare data with cmp, replace with exch if equal, return previous data value anyway
	type compare_and_swap(const type& cmp, const type& exch)
	{
		type old = cmp;
		atomic_storage<type>::compare_exchange(m_data, old, exch);
		return old;
	}

	// Atomically compare data with cmp, replace with exch if equal, return true if data was replaced
	bool compare_and_swap_test(const type& cmp, const type& exch)
	{
		type old = cmp;
		return atomic_storage<type>::compare_exchange(m_data, old, exch);
	}

	// As in std::atomic
	bool compare_exchange(type& cmp_and_old, const type& exch)
	{
		return atomic_storage<type>::compare_exchange(m_data, cmp_and_old, exch);
	}

	// Atomic operation; returns old value, or pair of old value and return value (cancel op if evaluates to false)
	template <typename F, typename RT = std::invoke_result_t<F, T&>>
		requires (!std::is_invocable_v<F, const T> && !std::is_invocable_v<F, volatile T>)
	std::conditional_t<std::is_void_v<RT>, type, std::pair<type, RT>> fetch_op(F func)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			_new = old;

			if constexpr (std::is_void_v<RT>)
			{
				std::invoke(func, _new);

				if (atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
				{
					return old;
				}
			}
			else
			{
				RT ret = std::invoke(func, _new);

				if (!ret || atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
				{
					return {old, std::move(ret)};
				}
			}
		}
	}

	// Atomic operation; returns function result value, function is the lambda
	template <typename F, typename RT = std::invoke_result_t<F, T&>>
		requires (!std::is_invocable_v<F, const T> && !std::is_invocable_v<F, volatile T>)
	RT atomic_op(F func)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			_new = old;

			if constexpr (std::is_void_v<RT>)
			{
				std::invoke(func, _new);

				if (atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
				{
					return;
				}
			}
			else
			{
				RT result = std::invoke(func, _new);

				if (atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
				{
					return result;
				}
			}
		}
	}

	// Atomically read data
	type load() const
	{
		return atomic_storage<type>::load(m_data);
	}

	// Atomically read data
	operator std::common_type_t<T>() const
	{
		return atomic_storage<type>::load(m_data);
	}

	// Relaxed load
	type observe() const
	{
		return atomic_storage<type>::observe(m_data);
	}

	// Atomically write data
	void store(const type& rhs)
	{
		atomic_storage<type>::store(m_data, rhs);
	}

	type operator =(const type& rhs)
	{
		atomic_storage<type>::store(m_data, rhs);
		return rhs;
	}

	// Atomically write data with release memory order (faster on x86)
	void release(const type& rhs)
	{
		atomic_storage<type>::release(m_data, rhs);
	}

	// Atomically replace data with value, return previous data value
	type exchange(const type& rhs)
	{
		return atomic_storage<type>::exchange(m_data, rhs);
	}

	auto fetch_add(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_add(m_data, rhs);
		}

		return fetch_op([&](T& v)
		{
			v += rhs;
		});
	}

	auto add_fetch(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::add_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			v += rhs;
			return v;
		});
	}

	auto operator +=(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::add_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			return v += rhs;
		});
	}

	auto fetch_sub(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_sub(m_data, rhs);
		}

		return fetch_op([&](T& v)
		{
			v -= rhs;
		});
	}

	auto sub_fetch(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::sub_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			v -= rhs;
			return v;
		});
	}

	auto operator -=(const ptr_rt& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::sub_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			return v -= rhs;
		});
	}

	auto fetch_and(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_and(m_data, rhs);
		}

		return fetch_op([&](T& v)
		{
			v &= rhs;
		});
	}

	auto and_fetch(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::and_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			v &= rhs;
			return v;
		});
	}

	auto operator &=(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::and_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			return v &= rhs;
		});
	}

	auto fetch_or(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_or(m_data, rhs);
		}

		return fetch_op([&](T& v)
		{
			v |= rhs;
		});
	}

	auto or_fetch(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::or_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			v |= rhs;
			return v;
		});
	}

	auto operator |=(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::or_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			return v |= rhs;
		});
	}

	auto fetch_xor(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_xor(m_data, rhs);
		}

		return fetch_op([&](T& v)
		{
			v ^= rhs;
		});
	}

	auto xor_fetch(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::xor_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			v ^= rhs;
			return v;
		});
	}

	auto operator ^=(const type& rhs)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::xor_fetch(m_data, rhs);
		}

		return atomic_op([&](T& v)
		{
			return v ^= rhs;
		});
	}

	auto operator ++()
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::inc_fetch(m_data);
		}

		return atomic_op([](T& v)
		{
			return ++v;
		});
	}

	auto operator --()
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::dec_fetch(m_data);
		}

		return atomic_op([](T& v)
		{
			return --v;
		});
	}

	auto operator ++(int)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_inc(m_data);
		}

		return atomic_op([](T& v)
		{
			return v++;
		});
	}

	auto operator --(int)
	{
		if constexpr(std::is_integral_v<type>)
		{
			return atomic_storage<type>::fetch_dec(m_data);
		}

		return atomic_op([](T& v)
		{
			return v--;
		});
	}

	// Conditionally decrement
	bool try_dec(std::common_type_t<T> greater_than)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			_new = old;

			if (!(_new > greater_than))
			{
				return false;
			}

			_new -= 1;

			if (atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
			{
				return true;
			}
		}
	}

	// Conditionally increment
	bool try_inc(std::common_type_t<T> less_than)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			_new = old;

			if (!(_new < less_than))
			{
				return false;
			}

			_new += 1;

			if (atomic_storage<type>::compare_exchange(m_data, old, _new)) [[likely]]
			{
				return true;
			}
		}
	}

	bool bit_test_set(uint bit)
	{
		if constexpr (std::is_integral_v<type>)
		{
			return atomic_storage<type>::bts(m_data, bit & (sizeof(T) * 8 - 1));
		}

		return atomic_op([](type& v)
		{
			const auto old = v;
			const auto bit = type(1) << (sizeof(T) * 8 - 1);
			v |= bit;
			return !!(old & bit);
		});
	}

	bool bit_test_reset(uint bit)
	{
		if constexpr (std::is_integral_v<type>)
		{
			return atomic_storage<type>::btr(m_data, bit & (sizeof(T) * 8 - 1));
		}

		return atomic_op([](type& v)
		{
			const auto old = v;
			const auto bit = type(1) << (sizeof(T) * 8 - 1);
			v &= ~bit;
			return !!(old & bit);
		});
	}

	bool bit_test_invert(uint bit)
	{
		if constexpr (std::is_integral_v<type>)
		{
			return atomic_storage<type>::btc(m_data, bit & (sizeof(T) * 8 - 1));
		}

		return atomic_op([](type& v)
		{
			const auto old = v;
			const auto bit = type(1) << (sizeof(T) * 8 - 1);
			v ^= bit;
			return !!(old & bit);
		});
	}

	void wait(type old_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const
		requires(sizeof(type) == 4)
	{
		atomic_wait_engine::wait(&m_data, std::bit_cast<u32>(old_value), static_cast<u64>(timeout));
	}

	[[deprecated]] void wait(type old_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const
		requires(sizeof(type) == 8)
	{
		atomic_wait::info ext[2]{};
		ext[0].data = reinterpret_cast<const char*>(&m_data) + 4;
		ext[0].old = std::bit_cast<u64>(old_value) >> 32;
		atomic_wait_engine::wait(&m_data, static_cast<u32>(std::bit_cast<u64>(old_value)), static_cast<u64>(timeout), ext);
	}

	void notify_one()
		requires(sizeof(type) == 4 || sizeof(type) == 8)
	{
		atomic_wait_engine::notify_one(&m_data);
	}

	void notify_all()
		requires(sizeof(type) == 4 || sizeof(type) == 8)
	{
		atomic_wait_engine::notify_all(&m_data);
	}
};

template <usz Align>
class atomic_t<bool, Align> : private atomic_t<uchar, Align>
{
	using base = atomic_t<uchar, Align>;

public:
	static constexpr usz align = Align;

	atomic_t() noexcept = default;

	atomic_t(const atomic_t&) = delete;

	atomic_t& operator =(const atomic_t&) = delete;

	constexpr atomic_t(bool value) noexcept
		: base(value)
	{
	}

	bool load() const noexcept
	{
		return base::load() != 0;
	}

	// Override implicit conversion from the parent type
	explicit operator uchar() const = delete;

	operator bool() const noexcept
	{
		return base::load() != 0;
	}

	bool observe() const noexcept
	{
		return base::observe() != 0;
	}

	void store(bool value)
	{
		base::store(value);
	}

	bool operator =(bool value)
	{
		base::store(value);
		return value;
	}

	void release(bool value)
	{
		base::release(value);
	}

	bool exchange(bool value)
	{
		return base::exchange(value) != 0;
	}

	bool test_and_set()
	{
		return base::exchange(1) != 0;
	}

	bool test_and_reset()
	{
		return base::exchange(0) != 0;
	}

	bool test_and_invert()
	{
		return base::fetch_xor(1) != 0;
	}
};

// Specializations

template <typename T, usz Align, typename T2, usz Align2>
struct std::common_type<atomic_t<T, Align>, atomic_t<T2, Align2>> : std::common_type<T, T2> {};

template <typename T, usz Align, typename T2>
struct std::common_type<atomic_t<T, Align>, T2> : std::common_type<T, std::common_type_t<T2>> {};

template <typename T, typename T2, usz Align2>
struct std::common_type<T, atomic_t<T2, Align2>> : std::common_type<std::common_type_t<T>, T2> {};

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

namespace utils
{
	template <typename F>
	struct aofn_helper
	{
		F f;

		aofn_helper(F&& f) noexcept
			: f(std::forward<F>(f))
		{
		}

		template <typename Arg> requires (std::is_same_v<std::remove_reference_t<Arg>, std::remove_cvref_t<Arg>> && !std::is_rvalue_reference_v<Arg>)
		auto operator()(Arg& arg) const noexcept
		{
			return f(std::forward<Arg&>(arg));
		}
	};

	template <typename F>
	aofn_helper(F&& f) -> aofn_helper<F>;
}

// Shorter lambda for non-cv qualified L-values
// For use with atomic operations
#define AOFN(...) \
	::utils::aofn_helper([&](auto& x) { return (__VA_ARGS__); })
