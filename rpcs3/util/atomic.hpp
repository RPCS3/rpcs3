#pragma once

#include "Utilities/types.h"
#include <functional>

#ifdef _MSC_VER
#include <atomic>
#endif

// Wait timeout extension (in nanoseconds)
enum class atomic_wait_timeout : u64
{
	inf = 0xffff'ffff'ffff'ffff,
};

// Helper for waitable atomics (as in C++20 std::atomic)
struct atomic_storage_futex
{
private:
	template <typename T>
	friend class atomic_t;

	static void wait(const void* data, std::size_t size, u64 old_value, u64 timeout, u64 mask);
	static void notify_one(const void* data);
	static void notify_all(const void* data);

public:
	static void set_wait_callback(bool(*)(const void* data));
	static void raw_notify(const void* data);
};

// Helper class, provides access to compiler-specific atomic intrinsics
template <typename T, std::size_t Size = sizeof(T)>
struct atomic_storage
{
	static_assert(sizeof(T) <= 16 && sizeof(T) == alignof(T), "atomic_storage<> error: invalid type");

	/* First part: Non-MSVC intrinsics */

#ifndef _MSC_VER

#if defined(__ATOMIC_HLE_ACQUIRE) && defined(__ATOMIC_HLE_RELEASE)
	static constexpr int s_hle_ack = __ATOMIC_SEQ_CST | __ATOMIC_HLE_ACQUIRE;
	static constexpr int s_hle_rel = __ATOMIC_SEQ_CST | __ATOMIC_HLE_RELEASE;
#else
	static constexpr int s_hle_ack = __ATOMIC_SEQ_CST;
	static constexpr int s_hle_rel = __ATOMIC_SEQ_CST;
#endif

	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		return __atomic_compare_exchange(&dest, &comp, &exch, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		static_assert(sizeof(T) == 4 || sizeof(T) == 8);
		return __atomic_compare_exchange(&dest, &comp, &exch, false, s_hle_ack, s_hle_ack);
	}

	static inline T load(const T& dest)
	{
		T result;
		__atomic_load(&dest, &result, __ATOMIC_SEQ_CST);
		return result;
	}

	static inline void store(T& dest, T value)
	{
		__atomic_store(&dest, &value, __ATOMIC_SEQ_CST);
	}

	static inline void release(T& dest, T value)
	{
		__atomic_store(&dest, &value, __ATOMIC_RELEASE);
	}

	static inline T exchange(T& dest, T value)
	{
		T result;
		__atomic_exchange(&dest, &value, &result, __ATOMIC_SEQ_CST);
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

	static inline bool test_and_set(T& dest, T mask)
	{
		return (atomic_storage<T>::fetch_or(dest, mask) & mask) != 0;
	}

	static inline bool test_and_reset(T& dest, T mask)
	{
		return (atomic_storage<T>::fetch_and(dest, ~mask) & mask) != 0;
	}

	static inline bool test_and_complement(T& dest, T mask)
	{
		return (atomic_storage<T>::fetch_xor(dest, mask) & mask) != 0;
	}

	static inline bool bts(T& dest, uint bit)
	{
		return atomic_storage<T>::test_and_set(dest, static_cast<T>(1) << bit);
	}

	static inline bool btr(T& dest, uint bit)
	{
		return atomic_storage<T>::test_and_reset(dest, static_cast<T>(1) << bit);
	}

	static inline bool btc(T& dest, uint bit)
	{
		return atomic_storage<T>::test_and_complement(dest, static_cast<T>(1) << bit);
	}
};

/* The rest: ugly MSVC intrinsics + inline asm implementations */

template <typename T>
struct atomic_storage<T, 1> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		char v = *(char*)&comp;
		char r = _InterlockedCompareExchange8((volatile char*)&dest, (char&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline T load(const T& dest)
	{
		char value = *(const volatile char*)&dest;
		std::atomic_thread_fence(std::memory_order_acquire);
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange8((volatile char*)&dest, (char&)value);
	}

	static inline void release(T& dest, T value)
	{
		std::atomic_thread_fence(std::memory_order_release);
		*(volatile char*)&dest = (char&)value;
	}

	static inline T exchange(T& dest, T value)
	{
		char r = _InterlockedExchange8((volatile char*)&dest, (char&)value);
		return (T&)r;
	}

	static inline T fetch_add(T& dest, T value)
	{
		char r = _InterlockedExchangeAdd8((volatile char*)&dest, (char&)value);
		return (T&)r;
	}

	static inline T fetch_and(T& dest, T value)
	{
		char r = _InterlockedAnd8((volatile char*)&dest, (char&)value);
		return (T&)r;
	}

	static inline T fetch_or(T& dest, T value)
	{
		char r = _InterlockedOr8((volatile char*)&dest, (char&)value);
		return (T&)r;
	}

	static inline T fetch_xor(T& dest, T value)
	{
		char r = _InterlockedXor8((volatile char*)&dest, (char&)value);
		return (T&)r;
	}
#endif
};

template <typename T>
struct atomic_storage<T, 2> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		short v = *(short*)&comp;
		short r = _InterlockedCompareExchange16((volatile short*)&dest, (short&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline T load(const T& dest)
	{
		short value = *(const volatile short*)&dest;
		std::atomic_thread_fence(std::memory_order_acquire);
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange16((volatile short*)&dest, (short&)value);
	}

	static inline void release(T& dest, T value)
	{
		std::atomic_thread_fence(std::memory_order_release);
		*(volatile short*)&dest = (short&)value;
	}

	static inline T exchange(T& dest, T value)
	{
		short r = _InterlockedExchange16((volatile short*)&dest, (short&)value);
		return (T&)r;
	}

	static inline T fetch_add(T& dest, T value)
	{
		short r = _InterlockedExchangeAdd16((volatile short*)&dest, (short&)value);
		return (T&)r;
	}

	static inline T fetch_and(T& dest, T value)
	{
		short r = _InterlockedAnd16((volatile short*)&dest, (short&)value);
		return (T&)r;
	}

	static inline T fetch_or(T& dest, T value)
	{
		short r = _InterlockedOr16((volatile short*)&dest, (short&)value);
		return (T&)r;
	}

	static inline T fetch_xor(T& dest, T value)
	{
		short r = _InterlockedXor16((volatile short*)&dest, (short&)value);
		return (T&)r;
	}

	static inline T inc_fetch(T& dest)
	{
		short r = _InterlockedIncrement16((volatile short*)&dest);
		return (T&)r;
	}

	static inline T dec_fetch(T& dest)
	{
		short r = _InterlockedDecrement16((volatile short*)&dest);
		return (T&)r;
	}
#else
	static inline bool bts(T& dest, uint bit)
	{
		bool result;
		ushort _bit = static_cast<ushort>(bit);
		__asm__("lock btsw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btr(T& dest, uint bit)
	{
		bool result;
		ushort _bit = static_cast<ushort>(bit);
		__asm__("lock btrw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btc(T& dest, uint bit)
	{
		bool result;
		ushort _bit = static_cast<ushort>(bit);
		__asm__("lock btcw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}
#endif
};

template <typename T>
struct atomic_storage<T, 4> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		long v = *(long*)&comp;
		long r = _InterlockedCompareExchange((volatile long*)&dest, (long&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		long v = *(long*)&comp;
		long r = _InterlockedCompareExchange_HLEAcquire((volatile long*)&dest, (long&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline T load(const T& dest)
	{
		long value = *(const volatile long*)&dest;
		std::atomic_thread_fence(std::memory_order_acquire);
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange((volatile long*)&dest, (long&)value);
	}

	static inline void release(T& dest, T value)
	{
		std::atomic_thread_fence(std::memory_order_release);
		*(volatile long*)&dest = (long&)value;
	}

	static inline T exchange(T& dest, T value)
	{
		long r = _InterlockedExchange((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T fetch_add(T& dest, T value)
	{
		long r = _InterlockedExchangeAdd((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T fetch_add_hle_rel(T& dest, T value)
	{
		long r = _InterlockedExchangeAdd_HLERelease((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T fetch_and(T& dest, T value)
	{
		long r = _InterlockedAnd((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T fetch_or(T& dest, T value)
	{
		long r = _InterlockedOr((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T fetch_xor(T& dest, T value)
	{
		long r = _InterlockedXor((volatile long*)&dest, (long&)value);
		return (T&)r;
	}

	static inline T inc_fetch(T& dest)
	{
		long r = _InterlockedIncrement((volatile long*)&dest);
		return (T&)r;
	}

	static inline T dec_fetch(T& dest)
	{
		long r = _InterlockedDecrement((volatile long*)&dest);
		return (T&)r;
	}

	static inline bool bts(T& dest, uint bit)
	{
		return _interlockedbittestandset((volatile long*)&dest, bit) != 0;
	}

	static inline bool btr(T& dest, uint bit)
	{
		return _interlockedbittestandreset((volatile long*)&dest, bit) != 0;
	}
#else
	static inline bool bts(T& dest, uint bit)
	{
		bool result;
		__asm__("lock btsl %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (bit) : "cc");
		return result;
	}

	static inline bool btr(T& dest, uint bit)
	{
		bool result;
		__asm__("lock btrl %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (bit) : "cc");
		return result;
	}

	static inline bool btc(T& dest, uint bit)
	{
		bool result;
		__asm__("lock btcl %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (bit) : "cc");
		return result;
	}
#endif
};

template <typename T>
struct atomic_storage<T, 8> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		llong v = *(llong*)&comp;
		llong r = _InterlockedCompareExchange64((volatile llong*)&dest, (llong&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline bool compare_exchange_hle_acq(T& dest, T& comp, T exch)
	{
		llong v = *(llong*)&comp;
		llong r = _InterlockedCompareExchange64_HLEAcquire((volatile llong*)&dest, (llong&)exch, v);
		comp = (T&)r;
		return r == v;
	}

	static inline T load(const T& dest)
	{
		llong value = *(const volatile llong*)&dest;
		std::atomic_thread_fence(std::memory_order_acquire);
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange64((volatile llong*)&dest, (llong&)value);
	}

	static inline void release(T& dest, T value)
	{
		std::atomic_thread_fence(std::memory_order_release);
		*(volatile llong*)&dest = (llong&)value;
	}

	static inline T exchange(T& dest, T value)
	{
		llong r = _InterlockedExchange64((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T fetch_add(T& dest, T value)
	{
		llong r = _InterlockedExchangeAdd64((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T fetch_add_hle_rel(T& dest, T value)
	{
		llong r = _InterlockedExchangeAdd64_HLERelease((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T fetch_and(T& dest, T value)
	{
		llong r = _InterlockedAnd64((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T fetch_or(T& dest, T value)
	{
		llong r = _InterlockedOr64((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T fetch_xor(T& dest, T value)
	{
		llong r = _InterlockedXor64((volatile llong*)&dest, (llong&)value);
		return (T&)r;
	}

	static inline T inc_fetch(T& dest)
	{
		llong r = _InterlockedIncrement64((volatile llong*)&dest);
		return (T&)r;
	}

	static inline T dec_fetch(T& dest)
	{
		llong r = _InterlockedDecrement64((volatile llong*)&dest);
		return (T&)r;
	}

	static inline bool bts(T& dest, uint bit)
	{
		return _interlockedbittestandset64((volatile llong*)&dest, bit) != 0;
	}

	static inline bool btr(T& dest, uint bit)
	{
		return _interlockedbittestandreset64((volatile llong*)&dest, bit) != 0;
	}
#else
	static inline bool bts(T& dest, uint bit)
	{
		bool result;
		ullong _bit = bit;
		__asm__("lock btsq %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btr(T& dest, uint bit)
	{
		bool result;
		ullong _bit = bit;
		__asm__("lock btrq %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btc(T& dest, uint bit)
	{
		bool result;
		ullong _bit = bit;
		__asm__("lock btcq %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}
#endif
};

template <typename T>
struct atomic_storage<T, 16> : atomic_storage<T, 0>
{
#ifdef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		llong* _exch = (llong*)&exch;
		return _InterlockedCompareExchange128((volatile llong*)&dest, _exch[1], _exch[0], (llong*)&comp) != 0;
	}

	static inline T load(const T& dest)
	{
		llong result[2]{0, 0};
		_InterlockedCompareExchange128((volatile llong*)&dest, 0, 0, result);
		return *(T*)+result;
	}

	static inline void store(T& dest, T value)
	{
		llong lo = *(llong*)&value;
		llong hi = *((llong*)&value + 1);
		llong cmp[2]{ *(volatile llong*)&dest, *((volatile llong*)&dest + 1) };
		while (!_InterlockedCompareExchange128((volatile llong*)&dest, hi, lo, cmp));
	}

	static inline void release(T& dest, T value)
	{
		llong lo = *(llong*)&value;
		llong hi = *((llong*)&value + 1);
		llong cmp[2]{ *(volatile llong*)&dest, *((volatile llong*)&dest + 1) };
		while (!_InterlockedCompareExchange128((volatile llong*)&dest, hi, lo, cmp));
	}

	static inline T exchange(T& dest, T value)
	{
		llong lo = *(llong*)&value;
		llong hi = *((llong*)&value + 1);
		llong cmp[2]{ *(volatile llong*)&dest, *((volatile llong*)&dest + 1) };
		while (!_InterlockedCompareExchange128((volatile llong*)&dest, hi, lo, cmp));
		return *(T*)+cmp;
	}
#endif

	// TODO
};

// Atomic type with lock-free and standard layout guarantees (and appropriate limitations)
template <typename T>
class atomic_t
{
protected:
	using type = typename std::remove_cv<T>::type;

	static_assert(alignof(type) == sizeof(type), "atomic_t<> error: unexpected alignment, use alignas() if necessary");

	type m_data;

public:
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

				if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new)))
				{
					return old;
				}
			}
			else
			{
				RT ret = std::invoke(func, _new);

				if (LIKELY(!ret || atomic_storage<type>::compare_exchange(m_data, old, _new)))
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

				if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new)))
				{
					return;
				}
			}
			else
			{
				RT result = std::invoke(func, _new);

				if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new)))
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

	type fetch_add(const type& rhs)
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

	type add_fetch(const type& rhs)
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

	auto operator +=(const type& rhs)
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

	type fetch_sub(const type& rhs)
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

	type sub_fetch(const type& rhs)
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

	auto operator -=(const type& rhs)
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

	type fetch_and(const type& rhs)
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

	type and_fetch(const type& rhs)
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

	type fetch_or(const type& rhs)
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

	type or_fetch(const type& rhs)
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

	type fetch_xor(const type& rhs)
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

	type xor_fetch(const type& rhs)
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
	bool try_dec(simple_type greater_than = std::numeric_limits<simple_type>::min())
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

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new)))
			{
				return true;
			}
		}
	}

	// Conditionally increment
	bool try_inc(simple_type less_than = std::numeric_limits<simple_type>::max())
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

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new)))
			{
				return true;
			}
		}
	}

	bool bts(uint bit)
	{
		return atomic_storage<type>::bts(m_data, bit);
	}

	bool btr(uint bit)
	{
		return atomic_storage<type>::btr(m_data, bit);
	}

	template <u64 Mask = 0xffff'ffff'ffff'ffff>
	void wait(type old_value, atomic_wait_timeout timeout = atomic_wait_timeout::inf) const noexcept
	{
		atomic_storage_futex::wait(&m_data, sizeof(T), std::bit_cast<get_uint_t<sizeof(T)>>(old_value), static_cast<u64>(timeout), Mask);
	}

	void notify_one() noexcept
	{
		atomic_storage_futex::notify_one(&m_data);
	}

	void notify_all() noexcept
	{
		atomic_storage_futex::notify_all(&m_data);
	}
};
