#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"
#include <functional>
#include <mutex>

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
#ifdef _MSC_VER
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_CONSUME);
#endif
}

FORCE_INLINE void atomic_fence_acquire()
{
#ifdef _MSC_VER
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_ACQUIRE);
#endif
}

FORCE_INLINE void atomic_fence_release()
{
#ifdef _MSC_VER
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_RELEASE);
#endif
}

FORCE_INLINE void atomic_fence_acq_rel()
{
#ifdef _MSC_VER
	_ReadWriteBarrier();
#else
	__atomic_thread_fence(__ATOMIC_ACQ_REL);
#endif
}

FORCE_INLINE void atomic_fence_seq_cst()
{
#ifdef _MSC_VER
	_ReadWriteBarrier();
	_InterlockedOr(static_cast<long*>(_AddressOfReturnAddress()), 0);
	_ReadWriteBarrier();
#else
	__asm__ volatile ("lock orl $0, 0(%%rsp);" ::: "cc", "memory");
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Wait timeout extension (in nanoseconds)
enum class atomic_wait_timeout : u64
{
	inf = 0xffffffffffffffff,
};

// Various extensions for atomic_t::wait
namespace atomic_wait
{
	// Max number of simultaneous atomic variables to wait on (can be extended if really necessary)
	constexpr uint max_list = 8;

	enum class op : u8
	{
		eq, // Wait while value is bitwise equal to
		slt, // Wait while signed value is less than
		sgt, // Wait while signed value is greater than
		ult, // Wait while unsigned value is less than
		ugt, // Wait while unsigned value is greater than
		alt, // Wait while absolute value is less than
		agt, // Wait while absolute value is greater than
		pop, // Wait while set bit count of the value is less than
		__max
	};

	static_assert(static_cast<u8>(op::__max) == 8);

	enum class op_flag : u8
	{
		inverse = 1 << 4, // Perform inverse operation (negate the result)
		bit_not = 1 << 5, // Perform bitwise NOT on loaded value before operation
		byteswap = 1 << 6, // Perform byteswap on both arguments and masks when applicable
	};

	constexpr op_flag op_be = std::endian::native == std::endian::little ? op_flag::byteswap : op_flag{0};
	constexpr op_flag op_le = std::endian::native == std::endian::little ? op_flag{0} : op_flag::byteswap;

	constexpr op operator |(op_flag lhs, op_flag rhs)
	{
		return op{static_cast<u8>(static_cast<u8>(lhs) | static_cast<u8>(rhs))};
	}

	constexpr op operator |(op_flag lhs, op rhs)
	{
		return op{static_cast<u8>(static_cast<u8>(lhs) | static_cast<u8>(rhs))};
	}

	constexpr op operator |(op lhs, op_flag rhs)
	{
		return op{static_cast<u8>(static_cast<u8>(lhs) | static_cast<u8>(rhs))};
	}

	constexpr op op_ne = op::eq | op_flag::inverse;

	constexpr struct any_value_t
	{
		template <typename T>
		operator T() const noexcept
		{
			return T();
		}
	} any_value;

	template <typename X, typename T = decltype(std::declval<X>().observe())>
	constexpr u128 default_mask = sizeof(T) <= 8 ? u128{UINT64_MAX >> ((64 - sizeof(T) * 8) & 63)} : u128(-1);

	template <typename X, typename T = decltype(std::declval<X>().observe())>
	constexpr u128 get_value(X&, T value = T{}, ...)
	{
		static_assert((sizeof(T) & (sizeof(T) - 1)) == 0);
		static_assert(sizeof(T) <= 16);
		return std::bit_cast<get_uint_t<sizeof(T)>, T>(value);
	}

	struct info
	{
		const void* data;
		u32 size;
		u128 old;
		u128 mask;

		template <typename X, typename T = decltype(std::declval<X>().observe())>
		constexpr void set_value(X& a, T value = T{})
		{
			old = get_value(a, value);
		}

		template <typename X, typename T = decltype(std::declval<X>().observe())>
		constexpr void set_mask(T value)
		{
			static_assert((sizeof(T) & (sizeof(T) - 1)) == 0);
			static_assert(sizeof(T) <= 16);
			mask = std::bit_cast<get_uint_t<sizeof(T)>, T>(value);
		}

		template <typename X, typename T = decltype(std::declval<X>().observe())>
		constexpr void set_mask()
		{
			mask = default_mask<X>;
		}
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

		template <typename... U, typename = std::void_t<decltype(std::declval<U>().template wait<op::eq>(any_value))...>>
		constexpr list(U&... vars)
			: m_info{{&vars, sizeof(vars.observe()), get_value(vars), default_mask<U>}...}
		{
			static_assert(sizeof...(U) == Max, "Inconsistent amount of atomics.");
		}

		template <typename... U>
		constexpr list& values(U... values)
		{
			static_assert(sizeof...(U) == Max, "Inconsistent amount of values.");

			auto* ptr = m_info;
			((ptr->template set_value<T>(*static_cast<T*>(ptr->data), values), ptr++), ...);
			return *this;
		}

		template <typename... U>
		constexpr list& masks(U... masks)
		{
			static_assert(sizeof...(U) <= Max, "Too many masks.");

			auto* ptr = m_info;
			((ptr++)->template set_mask<T>(masks), ...);
			return *this;
		}

		template <uint Index, op Flags = op::eq, typename T2, typename U, typename = std::void_t<decltype(std::declval<T2>().template wait<op::eq>(any_value))>>
		constexpr void set(T2& var, U value)
		{
			static_assert(Index < Max);

			m_info[Index].data = &var;
			m_info[Index].size = sizeof(var.observe()) | (static_cast<u8>(Flags) << 8);
			m_info[Index].template set_value<T2>(var, value);
			m_info[Index].template set_mask<T2>();
		}

		template <uint Index, op Flags = op::eq, typename T2, typename U, typename V, typename = std::void_t<decltype(std::declval<T2>().template wait<op::eq>(any_value))>>
		constexpr void set(T2& var, U value, V mask)
		{
			static_assert(Index < Max);

			m_info[Index].data = &var;
			m_info[Index].size = sizeof(var.observe()) | (static_cast<u8>(Flags) << 8);
			m_info[Index].template set_value<T2>(var, value);
			m_info[Index].template set_mask<T2>(mask);
		}

		// Timeout is discouraged
		void wait(atomic_wait_timeout timeout = atomic_wait_timeout::inf);

		// Same as wait
		void start()
		{
			wait();
		}
	};

	template <typename... T, typename = std::void_t<decltype(std::declval<T>().template wait<op::eq>(any_value))...>>
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

	static void wait(const void* data, u32 size, u128 old_value, u64 timeout, u128 mask, atomic_wait::info* ext = nullptr);
	static void notify_one(const void* data, u32 size, u128 mask128);
	static void notify_all(const void* data, u32 size, u128 mask128);

public:
	static void set_wait_callback(bool(*cb)(const void* data, u64 attempts, u64 stamp0));
	static void set_notify_callback(void(*cb)(const void* data, u64 progress));

	static void notify_all(const void* data)
	{
		notify_all(data, 0, u128(-1));
	}
};

template <uint Max, typename... T>
void atomic_wait::list<Max, T...>::wait(atomic_wait_timeout timeout)
{
	static_assert(Max, "Cannot initiate atomic wait with empty list.");

	atomic_wait_engine::wait(m_info[0].data, m_info[0].size, m_info[0].old, static_cast<u64>(timeout), m_info[0].mask, m_info + 1);
}

// Helper class, provides access to compiler-specific atomic intrinsics
template <typename T, usz Size = sizeof(T)>
struct atomic_storage
{
	/* First part: Non-MSVC intrinsics */

	using type = get_uint_t<sizeof(T)>;

#ifndef _MSC_VER

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

#ifdef _MSC_VER
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
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}

#ifdef _MSC_VER
		return _interlockedbittestandset((long*)dst, bit) != 0;
#else
		bool result;
		__asm__ volatile ("lock btsl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#endif
	}

	static inline bool btr(T& dest, uint bit)
	{
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}

#ifdef _MSC_VER
		return _interlockedbittestandreset((long*)dst, bit) != 0;
#else
		bool result;
		__asm__ volatile ("lock btrl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#endif
	}

	static inline bool btc(T& dest, uint bit)
	{
		uchar* dst = reinterpret_cast<uchar*>(&dest);

		if constexpr (sizeof(T) < 4)
		{
			const uptr ptr = reinterpret_cast<uptr>(dst);

			// Align the bit up and pointer down
			bit = bit + (ptr & 3) * 8;
			dst = reinterpret_cast<T*>(ptr & -4);
		}

#ifdef _MSC_VER
		while (true)
		{
			// Keep trying until we actually invert desired bit
			if (!_bittest((long*)dst, bit) && !_interlockedbittestandset((long*)dst, bit))
				return false;
			if (_interlockedbittestandreset((long*)dst, bit))
				return true;
		}
#else
		bool result;
		__asm__ volatile ("lock btcl %2, 0(%1)\n" : "=@ccc" (result) : "r" (dst), "Ir" (bit) : "cc", "memory");
		return result;
#endif
	}
};

/* The rest: ugly MSVC intrinsics + inline asm implementations */

template <typename T>
struct atomic_storage<T, 1> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
#ifdef _MSC_VER
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
#else
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
	using type = typename std::remove_cv<T>::type;

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

	atomic_t() noexcept = default;

	atomic_t(const atomic_t&) = delete;

	atomic_t& operator =(const atomic_t&) = delete;

	// Define simple type
	using simple_type = simple_t<T>;

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
	operator simple_type() const
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
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
		if constexpr(std::is_integral<type>::value)
		{
			return atomic_storage<type>::fetch_dec(m_data);
		}

		return atomic_op([](T& v)
		{
			return v--;
		});
	}

	// Conditionally decrement
	bool try_dec(simple_type greater_than)
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
	bool try_inc(simple_type less_than)
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
		return atomic_storage<type>::bts(m_data, bit & (sizeof(T) * 8 - 1));
	}

	bool bit_test_reset(uint bit)
	{
		return atomic_storage<type>::btr(m_data, bit & (sizeof(T) * 8 - 1));
	}

	bool bit_test_invert(uint bit)
	{
		return atomic_storage<type>::btc(m_data, bit & (sizeof(T) * 8 - 1));
	}

	// Timeout is discouraged
	template <atomic_wait::op Flags = atomic_wait::op::eq>
	void wait(type old_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const noexcept
	{
		const u128 old = std::bit_cast<get_uint_t<sizeof(T)>>(old_value);
		const u128 mask = atomic_wait::default_mask<atomic_t>;
		atomic_wait_engine::wait(&m_data, sizeof(T) | (static_cast<u8>(Flags) << 8), old, static_cast<u64>(timeout), mask);
	}

	// Overload with mask (only selected bits are checked), timeout is discouraged
	template <atomic_wait::op Flags = atomic_wait::op::eq>
	void wait(type old_value, type mask_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const noexcept
	{
		const u128 old = std::bit_cast<get_uint_t<sizeof(T)>>(old_value);
		const u128 mask = std::bit_cast<get_uint_t<sizeof(T)>>(mask_value);
		atomic_wait_engine::wait(&m_data, sizeof(T) | (static_cast<u8>(Flags) << 8), old, static_cast<u64>(timeout), mask);
	}

	void notify_one() noexcept
	{
		atomic_wait_engine::notify_one(&m_data, sizeof(T), atomic_wait::default_mask<atomic_t>);
	}

	// Notify with mask, allowing to not wake up thread which doesn't wait on this mask
	void notify_one(type mask_value) noexcept
	{
		const u128 mask = std::bit_cast<get_uint_t<sizeof(T)>>(mask_value);
		atomic_wait_engine::notify_one(&m_data, sizeof(T), mask);
	}

	void notify_all() noexcept
	{
		atomic_wait_engine::notify_all(&m_data, sizeof(T), atomic_wait::default_mask<atomic_t>);
	}

	// Notify all threads with mask, allowing to not wake up threads which don't wait on them
	void notify_all(type mask_value) noexcept
	{
		const u128 mask = std::bit_cast<get_uint_t<sizeof(T)>>(mask_value);
		atomic_wait_engine::notify_all(&m_data, sizeof(T), mask);
	}
};

template <usz Align>
class atomic_t<bool, Align> : private atomic_t<uchar, Align>
{
	using base = atomic_t<uchar, Align>;

public:
	static constexpr usz align = Align;

	using simple_type = bool;

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

	// Timeout is discouraged
	template <atomic_wait::op Flags = atomic_wait::op::eq>
	void wait(bool old_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const noexcept
	{
		base::template wait<Flags>(old_value, 1, timeout);
	}

	void notify_one() noexcept
	{
		base::notify_one(1);
	}

	void notify_all() noexcept
	{
		base::notify_all(1);
	}
};

namespace atomic_wait
{
	template <usz Align>
	constexpr u128 default_mask<atomic_t<bool, Align>> = 1;
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
