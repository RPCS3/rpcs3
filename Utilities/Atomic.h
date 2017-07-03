#pragma once

#include "types.h"

// Helper class, provides access to compiler-specific atomic intrinsics
template<typename T, std::size_t Size>
struct atomic_storage
{
	static_assert(sizeof(T) <= 16 && sizeof(T) == alignof(T), "atomic_storage<> error: invalid type");

	/* First part: Non-MSVC intrinsics */

#ifndef _MSC_VER
	static inline bool compare_exchange(T& dest, T& comp, T exch)
	{
		return __atomic_compare_exchange(&dest, &comp, &exch, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
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

template<typename T>
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
		_ReadWriteBarrier();
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange8((volatile char*)&dest, (char&)value);
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

template<typename T>
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
		_ReadWriteBarrier();
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange16((volatile short*)&dest, (short&)value);
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
		ushort _bit = (ushort)bit;
		__asm__("lock btsw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btr(T& dest, uint bit)
	{
		bool result;
		ushort _bit = (ushort)bit;
		__asm__("lock btrw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}

	static inline bool btc(T& dest, uint bit)
	{
		bool result;
		ushort _bit = (ushort)bit;
		__asm__("lock btcw %2, %0\n" "setc %1" : "+m" (dest), "=r" (result) : "Ir" (_bit) : "cc");
		return result;
	}
#endif
};

template<typename T>
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

	static inline T load(const T& dest)
	{
		long value = *(const volatile long*)&dest;
		_ReadWriteBarrier();
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange((volatile long*)&dest, (long&)value);
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

template<typename T>
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

	static inline T load(const T& dest)
	{
		llong value = *(const volatile llong*)&dest;
		_ReadWriteBarrier();
		return (T&)value;
	}

	static inline void store(T& dest, T value)
	{
		_InterlockedExchange64((volatile llong*)&dest, (llong&)value);
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

template<typename T>
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

template<typename T1, typename T2, typename>
struct atomic_add
{
	auto operator()(T1& lhs, const T2& rhs) const
	{
		return lhs += rhs;
	}
};

template<typename T1, typename T2>
struct atomic_add<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::fetch_add;
	static constexpr auto op_fetch = &atomic_storage<T1>::add_fetch;
	static constexpr auto atomic_op = &atomic_storage<T1>::add_fetch;
};

template<typename T1, typename T2, typename>
struct atomic_sub
{
	auto operator()(T1& lhs, const T2& rhs) const
	{
		return lhs -= rhs;
	}
};

template<typename T1, typename T2>
struct atomic_sub<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::fetch_sub;
	static constexpr auto op_fetch = &atomic_storage<T1>::sub_fetch;
	static constexpr auto atomic_op = &atomic_storage<T1>::sub_fetch;
};

template<typename T, typename>
struct atomic_pre_inc
{
	auto operator()(T& v) const
	{
		return ++v;
	}
};

template<typename T>
struct atomic_pre_inc<T, std::enable_if_t<std::is_integral<T>::value>>
{
	static constexpr auto atomic_op = &atomic_storage<T>::inc_fetch;
};

template<typename T, typename>
struct atomic_post_inc
{
	auto operator()(T& v) const
	{
		return v++;
	}
};

template<typename T>
struct atomic_post_inc<T, std::enable_if_t<std::is_integral<T>::value>>
{
	static constexpr auto atomic_op = &atomic_storage<T>::fetch_inc;
};

template<typename T, typename>
struct atomic_pre_dec
{
	auto operator()(T& v) const
	{
		return --v;
	}
};

template<typename T>
struct atomic_pre_dec<T, std::enable_if_t<std::is_integral<T>::value>>
{
	static constexpr auto atomic_op = &atomic_storage<T>::dec_fetch;
};

template<typename T, typename>
struct atomic_post_dec
{
	auto operator()(T& v) const
	{
		return v--;
	}
};

template<typename T>
struct atomic_post_dec<T, std::enable_if_t<std::is_integral<T>::value>>
{
	static constexpr auto atomic_op = &atomic_storage<T>::fetch_dec;
};

template<typename T1, typename T2, typename>
struct atomic_and
{
	auto operator()(T1& lhs, const T2& rhs) const
	{
		return lhs &= rhs;
	}
};

template<typename T1, typename T2>
struct atomic_and<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::fetch_and;
	static constexpr auto op_fetch = &atomic_storage<T1>::and_fetch;
	static constexpr auto atomic_op = &atomic_storage<T1>::and_fetch;
};

template<typename T1, typename T2, typename>
struct atomic_or
{
	auto operator()(T1& lhs, const T2& rhs) const
	{
		return lhs |= rhs;
	}
};

template<typename T1, typename T2>
struct atomic_or<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::fetch_or;
	static constexpr auto op_fetch = &atomic_storage<T1>::or_fetch;
	static constexpr auto atomic_op = &atomic_storage<T1>::or_fetch;
};

template<typename T1, typename T2, typename>
struct atomic_xor
{
	auto operator()(T1& lhs, const T2& rhs) const
	{
		return lhs ^= rhs;
	}
};

template<typename T1, typename T2>
struct atomic_xor<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::fetch_xor;
	static constexpr auto op_fetch = &atomic_storage<T1>::xor_fetch;
	static constexpr auto atomic_op = &atomic_storage<T1>::xor_fetch;
};

template<typename T1, typename T2, typename>
struct atomic_test_and_set
{
	bool operator()(T1& lhs, const T2& rhs) const
	{
		return test_and_set(lhs, rhs);
	}
};

template<typename T1, typename T2>
struct atomic_test_and_set<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::test_and_set;
	static constexpr auto op_fetch = &atomic_storage<T1>::test_and_set;
	static constexpr auto atomic_op = &atomic_storage<T1>::test_and_set;
};

template<typename T1, typename T2, typename>
struct atomic_test_and_reset
{
	bool operator()(T1& lhs, const T2& rhs) const
	{
		return test_and_reset(lhs, rhs);
	}
};

template<typename T1, typename T2>
struct atomic_test_and_reset<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::test_and_reset;
	static constexpr auto op_fetch = &atomic_storage<T1>::test_and_reset;
	static constexpr auto atomic_op = &atomic_storage<T1>::test_and_reset;
};

template<typename T1, typename T2, typename>
struct atomic_test_and_complement
{
	bool operator()(T1& lhs, const T2& rhs) const
	{
		return test_and_complement(lhs, rhs);
	}
};

template<typename T1, typename T2>
struct atomic_test_and_complement<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_convertible<T2, T1>::value>>
{
	static constexpr auto fetch_op = &atomic_storage<T1>::test_and_complement;
	static constexpr auto op_fetch = &atomic_storage<T1>::test_and_complement;
	static constexpr auto atomic_op = &atomic_storage<T1>::test_and_complement;
};

// Atomic type with lock-free and standard layout guarantees (and appropriate limitations)
template<typename T>
class atomic_t
{
	using type = typename std::remove_cv<T>::type;

	static_assert(alignof(type) == sizeof(type), "atomic_t<> error: unexpected alignment, use alignas() if necessary");

	type m_data;

public:
	atomic_t() = default;

	atomic_t(const atomic_t&) = delete;

	atomic_t& operator =(const atomic_t&) = delete;

	// Define simple type
	using simple_type = simple_t<T>;

	explicit constexpr atomic_t(const type& value)
		: m_data(value)
	{
	}

	// Unsafe direct access
	type& raw()
	{
		return m_data;
	}

	// Atomically compare data with cmp, replace with exch if equal, return previous data value anyway
	simple_type compare_and_swap(const type& cmp, const type& exch)
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

	// Atomic operation; returns old value, discards function result value
	template<typename F, typename... Args, typename RT = std::result_of_t<F(T&, const Args&...)>>
	type fetch_op(F&& func, const Args&... args)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			func((_new = old), args...);

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new))) return old;
		}
	}

	// Helper overload for calling optimized implementation
	template<typename F, typename... Args, typename FT = decltype(F::fetch_op), typename RT = std::result_of_t<FT(T&, const Args&...)>>
	type fetch_op(F&&, const Args&... args)
	{
		return F::fetch_op(m_data, args...);
	}

	// Atomic operation; returns new value, discards function result value
	template<typename F, typename... Args, typename RT = std::result_of_t<F(T&, const Args&...)>>
	type op_fetch(F&& func, const Args&... args)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			func((_new = old), args...);

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new))) return _new;
		}
	}

	// Helper overload for calling optimized implementation
	template<typename F, typename... Args, typename FT = decltype(F::op_fetch), typename RT = std::result_of_t<FT(T&, const Args&...)>>
	type op_fetch(F&&, const Args&... args)
	{
		return F::op_fetch(m_data, args...);
	}

	// Atomic operation; returns function result value
	template<typename F, typename... Args, typename RT = std::result_of_t<F(T&, const Args&...)>, typename = std::enable_if_t<!std::is_void<RT>::value>>
	RT atomic_op(F&& func, const Args&... args)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			RT&& result = func((_new = old), args...);

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new))) return std::move(result);
		}
	}

	// Overload for void return type
	template<typename F, typename... Args, typename RT = std::result_of_t<F(T&, const Args&...)>, typename = std::enable_if_t<std::is_void<RT>::value>>
	void atomic_op(F&& func, const Args&... args)
	{
		type _new, old = atomic_storage<type>::load(m_data);

		while (true)
		{
			func((_new = old), args...);

			if (LIKELY(atomic_storage<type>::compare_exchange(m_data, old, _new))) return;
		}
	}

	// Helper overload for calling optimized implementation
	template<typename F, typename... Args, typename FT = decltype(F::atomic_op), typename RT = std::result_of_t<FT(T&, const Args&...)>>
	auto atomic_op(F&&, const Args&... args)
	{
		return F::atomic_op(m_data, args...);
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

	// Atomically replace data with value, return previous data value
	type exchange(const type& rhs)
	{
		return atomic_storage<type>::exchange(m_data, rhs);
	}

	template<typename T2>
	type fetch_add(const T2& rhs)
	{
		return fetch_op(atomic_add<type, T2>{}, rhs);
	}
	
	template<typename T2>
	type add_fetch(const T2& rhs)
	{
		return op_fetch(atomic_add<type, T2>{}, rhs);
	}

	template<typename T2>
	auto operator +=(const T2& rhs)
	{
		return atomic_op(atomic_add<type, T2>{}, rhs);
	}

	template<typename T2>
	type fetch_sub(const T2& rhs)
	{
		return fetch_op(atomic_sub<type, T2>{}, rhs);
	}

	template<typename T2>
	type sub_fetch(const T2& rhs)
	{
		return op_fetch(atomic_sub<type, T2>{}, rhs);
	}

	template<typename T2>
	auto operator -=(const T2& rhs)
	{
		return atomic_op(atomic_sub<type, T2>{}, rhs);
	}

	template<typename T2>
	type fetch_and(const T2& rhs)
	{
		return fetch_op(atomic_and<type, T2>{}, rhs);
	}

	template<typename T2>
	type and_fetch(const T2& rhs)
	{
		return op_fetch(atomic_and<type, T2>{}, rhs);
	}

	template<typename T2>
	auto operator &=(const T2& rhs)
	{
		return atomic_op(atomic_and<type, T2>{}, rhs);
	}

	template<typename T2>
	type fetch_or(const T2& rhs)
	{
		return fetch_op(atomic_or<type, T2>{}, rhs);
	}

	template<typename T2>
	type or_fetch(const T2& rhs)
	{
		return op_fetch(atomic_or<type, T2>{}, rhs);
	}

	template<typename T2>
	auto operator |=(const T2& rhs)
	{
		return atomic_op(atomic_or<type, T2>{}, rhs);
	}

	template<typename T2>
	type fetch_xor(const T2& rhs)
	{
		return fetch_op(atomic_xor<type, T2>{}, rhs);
	}

	template<typename T2>
	type xor_fetch(const T2& rhs)
	{
		return op_fetch(atomic_xor<type, T2>{}, rhs);
	}

	template<typename T2>
	auto operator ^=(const T2& rhs)
	{
		return atomic_op(atomic_xor<type, T2>{}, rhs);
	}

	auto operator ++()
	{
		return atomic_op(atomic_pre_inc<type>{});
	}

	auto operator --()
	{
		return atomic_op(atomic_pre_dec<type>{});
	}

	auto operator ++(int)
	{
		return atomic_op(atomic_post_inc<type>{});
	}

	auto operator --(int)
	{
		return atomic_op(atomic_post_dec<type>{});
	}

	template<typename T2 = T>
	auto test_and_set(const T2& rhs)
	{
		return atomic_op(atomic_test_and_set<type, T2>{}, rhs);
	}

	template<typename T2 = T>
	auto test_and_reset(const T2& rhs)
	{
		return atomic_op(atomic_test_and_reset<type, T2>{}, rhs);
	}

	template<typename T2 = T>
	auto test_and_complement(const T2& rhs)
	{
		return atomic_op(atomic_test_and_complement<type, T2>{}, rhs);
	}

	// Minimal pointer support (TODO: must forward operator ->())
	type operator ->() const
	{
		return load();
	}

	// Minimal array support
	template<typename I = std::size_t>
	auto operator [](const I& index) const -> decltype(std::declval<const type>()[std::declval<I>()])
	{
		return load()[index];
	}
};
