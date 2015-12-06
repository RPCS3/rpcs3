#pragma once

#ifdef _MSC_VER
#define NOMINMAX
#include <Windows.h>
#endif

#include "u128.h"
#include <cstddef>
#include <type_traits>
#include "endianness.h"
#include <atomic>

namespace common
{
#if defined(__GNUG__) && !defined(_MSC_VER)
	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_val_compare_and_swap(volatile T* dest, T2 comp, T2 exch)
	{
		return __sync_val_compare_and_swap(dest, comp, exch);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), bool> sync_bool_compare_and_swap(volatile T* dest, T2 comp, T2 exch)
	{
		return __sync_bool_compare_and_swap(dest, comp, exch);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_lock_test_and_set(volatile T* dest, T2 value)
	{
		return __sync_lock_test_and_set(dest, value);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_fetch_and_add(volatile T* dest, T2 value)
	{
		return __sync_fetch_and_add(dest, value);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_fetch_and_sub(volatile T* dest, T2 value)
	{
		return __sync_fetch_and_sub(dest, value);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_fetch_and_or(volatile T* dest, T2 value)
	{
		return __sync_fetch_and_or(dest, value);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_fetch_and_and(volatile T* dest, T2 value)
	{
		return __sync_fetch_and_and(dest, value);
	}

	template<typename T, typename T2> inline std::enable_if_t<IS_INTEGRAL(T), T> sync_fetch_and_xor(volatile T* dest, T2 value)
	{
		return __sync_fetch_and_xor(dest, value);
	}

#elif defined(_MSC_VER)
	// atomic compare and swap functions

	inline u8 sync_val_compare_and_swap(volatile u8* dest, u8 comp, u8 exch)
	{
		return _InterlockedCompareExchange8((volatile char*)dest, exch, comp);
	}

	inline u16 sync_val_compare_and_swap(volatile u16* dest, u16 comp, u16 exch)
	{
		return _InterlockedCompareExchange16((volatile short*)dest, exch, comp);
	}

	inline u32 sync_val_compare_and_swap(volatile u32* dest, u32 comp, u32 exch)
	{
		return _InterlockedCompareExchange((volatile long*)dest, exch, comp);
	}

	inline u64 sync_val_compare_and_swap(volatile u64* dest, u64 comp, u64 exch)
	{
		return _InterlockedCompareExchange64((volatile long long*)dest, exch, comp);
	}

	inline u128 sync_val_compare_and_swap(volatile u128* dest, u128 comp, u128 exch)
	{
		_InterlockedCompareExchange128((volatile long long*)dest, exch.hi, exch.lo, (long long*)&comp);
		return comp;
	}

	inline bool sync_bool_compare_and_swap(volatile u8* dest, u8 comp, u8 exch)
	{
		return (u8)_InterlockedCompareExchange8((volatile char*)dest, exch, comp) == comp;
	}

	inline bool sync_bool_compare_and_swap(volatile u16* dest, u16 comp, u16 exch)
	{
		return (u16)_InterlockedCompareExchange16((volatile short*)dest, exch, comp) == comp;
	}

	inline bool sync_bool_compare_and_swap(volatile u32* dest, u32 comp, u32 exch)
	{
		return (u32)_InterlockedCompareExchange((volatile long*)dest, exch, comp) == comp;
	}

	inline bool sync_bool_compare_and_swap(volatile u64* dest, u64 comp, u64 exch)
	{
		return (u64)_InterlockedCompareExchange64((volatile long long*)dest, exch, comp) == comp;
	}

	inline bool sync_bool_compare_and_swap(volatile u128* dest, u128 comp, u128 exch)
	{
		return _InterlockedCompareExchange128((volatile long long*)dest, exch.hi, exch.lo, (long long*)&comp) != 0;
	}

	// atomic exchange functions

	inline u8 sync_lock_test_and_set(volatile u8* dest, u8 value)
	{
		return _InterlockedExchange8((volatile char*)dest, value);
	}

	inline u16 sync_lock_test_and_set(volatile u16* dest, u16 value)
	{
		return _InterlockedExchange16((volatile short*)dest, value);
	}

	inline u32 sync_lock_test_and_set(volatile u32* dest, u32 value)
	{
		return _InterlockedExchange((volatile long*)dest, value);
	}

	inline u64 sync_lock_test_and_set(volatile u64* dest, u64 value)
	{
		return _InterlockedExchange64((volatile long long*)dest, value);
	}

	inline u128 sync_lock_test_and_set(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, value)) return old;
		}
	}

	// atomic add functions

	inline u8 sync_fetch_and_add(volatile u8* dest, u8 value)
	{
		return _InterlockedExchangeAdd8((volatile char*)dest, value);
	}

	inline u16 sync_fetch_and_add(volatile u16* dest, u16 value)
	{
		return _InterlockedExchangeAdd16((volatile short*)dest, value);
	}

	inline u32 sync_fetch_and_add(volatile u32* dest, u32 value)
	{
		return _InterlockedExchangeAdd((volatile long*)dest, value);
	}

	inline u64 sync_fetch_and_add(volatile u64* dest, u64 value)
	{
		return _InterlockedExchangeAdd64((volatile long long*)dest, value);
	}

	inline u128 sync_fetch_and_add(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, old + value)) return old;
		}
	}

	// atomic sub functions

	inline u8 sync_fetch_and_sub(volatile u8* dest, u8 value)
	{
		return _InterlockedExchangeAdd8((volatile char*)dest, -(char)value);
	}

	inline u16 sync_fetch_and_sub(volatile u16* dest, u16 value)
	{
		return _InterlockedExchangeAdd16((volatile short*)dest, -(short)value);
	}

	inline u32 sync_fetch_and_sub(volatile u32* dest, u32 value)
	{
		return _InterlockedExchangeAdd((volatile long*)dest, -(long)value);
	}

	inline u64 sync_fetch_and_sub(volatile u64* dest, u64 value)
	{
		return _InterlockedExchangeAdd64((volatile long long*)dest, -(long long)value);
	}

	inline u128 sync_fetch_and_sub(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, old - value)) return old;
		}
	}

	// atomic `bitwise or` functions

	inline u8 sync_fetch_and_or(volatile u8* dest, u8 value)
	{
		return _InterlockedOr8((volatile char*)dest, value);
	}

	inline u16 sync_fetch_and_or(volatile u16* dest, u16 value)
	{
		return _InterlockedOr16((volatile short*)dest, value);
	}

	inline u32 sync_fetch_and_or(volatile u32* dest, u32 value)
	{
		return _InterlockedOr((volatile long*)dest, value);
	}

	inline u64 sync_fetch_and_or(volatile u64* dest, u64 value)
	{
		return _InterlockedOr64((volatile long long*)dest, value);
	}

	inline u128 sync_fetch_and_or(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, old | value)) return old;
		}
	}

	// atomic `bitwise and` functions

	inline u8 sync_fetch_and_and(volatile u8* dest, u8 value)
	{
		return _InterlockedAnd8((volatile char*)dest, value);
	}

	inline u16 sync_fetch_and_and(volatile u16* dest, u16 value)
	{
		return _InterlockedAnd16((volatile short*)dest, value);
	}

	inline u32 sync_fetch_and_and(volatile u32* dest, u32 value)
	{
		return _InterlockedAnd((volatile long*)dest, value);
	}

	inline u64 sync_fetch_and_and(volatile u64* dest, u64 value)
	{
		return _InterlockedAnd64((volatile long long*)dest, value);
	}

	inline u128 sync_fetch_and_and(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, old & value)) return old;
		}
	}

	// atomic `bitwise xor` functions

	inline u8 sync_fetch_and_xor(volatile u8* dest, u8 value)
	{
		return _InterlockedXor8((volatile char*)dest, value);
	}

	inline u16 sync_fetch_and_xor(volatile u16* dest, u16 value)
	{
		return _InterlockedXor16((volatile short*)dest, value);
	}

	inline u32 sync_fetch_and_xor(volatile u32* dest, u32 value)
	{
		return _InterlockedXor((volatile long*)dest, value);
	}

	inline u64 sync_fetch_and_xor(volatile u64* dest, u64 value)
	{
		return _InterlockedXor64((volatile long long*)dest, value);
	}

	inline u128 sync_fetch_and_xor(volatile u128* dest, u128 value)
	{
		while (true)
		{
			u128 old;
			old.lo = dest->lo;
			old.hi = dest->hi;

			if (sync_bool_compare_and_swap(dest, old, old ^ value)) return old;
		}
	}

#endif /* _MSC_VER */

	template<typename T, std::size_t Size = sizeof(T)> struct atomic_storage
	{
		static_assert(!Size, "Invalid atomic type");
	};

	template<typename T> struct atomic_storage<T, 1>
	{
		using type = u8;
	};

	template<typename T> struct atomic_storage<T, 2>
	{
		using type = u16;
	};

	template<typename T> struct atomic_storage<T, 4>
	{
		using type = u32;
	};

	template<typename T> struct atomic_storage<T, 8>
	{
		using type = u64;
	};

	template<typename T> struct atomic_storage<T, 16>
	{
		using type = u128;
	};

	template<typename T> using atomic_storage_t = typename atomic_storage<T>::type;

	// atomic result wrapper; implements special behaviour for void result type
	template<typename T, typename RT, typename VT> struct atomic_op_result
	{
		RT result;

		template<typename... Args> atomic_op_result(T func, VT& var, Args&&... args)
			: result(std::move(func(var, std::forward<Args>(args)...)))
		{
		}

		RT move()
		{
			return std::move(result);
		}
	};

	// void specialization: result is the initial value of the first arg
	template<typename T, typename VT> struct atomic_op_result<T, void, VT>
	{
		VT result;

		template<typename... Args> atomic_op_result(T func, VT& var, Args&&... args)
			: result(var)
		{
			func(var, std::forward<Args>(args)...);
		}

		VT move()
		{
			return std::move(result);
		}
	};

	// member function specialization
	template<typename CT, typename... FArgs, typename RT, typename VT> struct atomic_op_result<RT(CT::*)(FArgs...), RT, VT>
	{
		RT result;

		template<typename... Args> atomic_op_result(RT(CT::*func)(FArgs...), VT& var, Args&&... args)
			: result(std::move((var.*func)(std::forward<Args>(args)...)))
		{
		}

		RT move()
		{
			return std::move(result);
		}
	};

	// member function void specialization
	template<typename CT, typename... FArgs, typename VT> struct atomic_op_result<void(CT::*)(FArgs...), void, VT>
	{
		VT result;

		template<typename... Args> atomic_op_result(void(CT::*func)(FArgs...), VT& var, Args&&... args)
			: result(var)
		{
			(var.*func)(std::forward<Args>(args)...);
		}

		VT move()
		{
			return std::move(result);
		}
	};

	// Atomic type with lock-free and standard layout guarantees (and appropriate limitations)
	template<typename T>
	class atomic
	{
		using type = std::remove_cv_t<T>;
		using stype = atomic_storage_t<type>;
		using storage = atomic_storage<type>;

		static_assert(alignof(type) <= alignof(stype), "atomic<> error: unexpected alignment");

		stype m_data;

		template<typename T2> static inline void write_relaxed(volatile T2& data, const T2& value)
		{
			data = value;
		}

		static inline void write_relaxed(volatile u128& data, const u128& value)
		{
			sync_lock_test_and_set(&data, value);
		}

		template<typename T2> static inline T2 read_relaxed(const volatile T2& data)
		{
			return data;
		}

		static inline u128 read_relaxed(const volatile u128& value)
		{
			return sync_val_compare_and_swap(const_cast<volatile u128*>(&value), u128{ 0 }, u128{ 0 });
		}

	public:
		static inline const stype to_subtype(const type& value)
		{
			return reinterpret_cast<const stype&>(value);
		}

		static inline const type from_subtype(const stype value)
		{
			return reinterpret_cast<const type&>(value);
		}

		atomic() = default;

		atomic(const atomic&) = delete;

		atomic(type value)
			: m_data(to_subtype(value))
		{
		}

		atomic& operator =(const atomic&) = delete;

		atomic& operator =(type value)
		{
			return write_relaxed(m_data, to_subtype(value)), *this;
		}

		operator type() const volatile
		{
			return from_subtype(read_relaxed(m_data));
		}

		// Unsafe direct access
		stype* raw_data()
		{
			return reinterpret_cast<stype*>(&m_data);
		}

		// Unsafe direct access
		type& raw()
		{
			return reinterpret_cast<type&>(m_data);
		}

		// Atomically compare data with cmp, replace with exch if equal, return previous data value anyway
		type compare_and_swap(const type& cmp, const type& exch) volatile
		{
			return from_subtype(sync_val_compare_and_swap(&m_data, to_subtype(cmp), to_subtype(exch)));
		}

		// Atomically compare data with cmp, replace with exch if equal, return true if data was replaced
		bool compare_and_swap_test(const type& cmp, const type& exch) volatile
		{
			return sync_bool_compare_and_swap(&m_data, to_subtype(cmp), to_subtype(exch));
		}

		// Atomically replace data with exch, return previous data value
		type exchange(const type& exch) volatile
		{
			return from_subtype(sync_lock_test_and_set(&m_data, to_subtype(exch)));
		}

		// Atomically read data, possibly without memory barrier (not for 128 bit)
		type load() const volatile
		{
			return from_subtype(read_relaxed(m_data));
		}

		// Atomically write data, possibly without memory barrier (not for 128 bit)
		void store(const type& value) volatile
		{
			write_relaxed(m_data, to_subtype(value));
		}

		// Perform an atomic operation on data (func is either pointer to member function or callable object with a T& first arg);
		// Returns the result of the callable object call or previous (old) value of the atomic variable if the return type is void
		template<typename F, typename... Args, typename RT = std::result_of_t<F(T&, Args...)>>
		auto atomic_op(F func, Args&&... args) volatile -> decltype(atomic_op_result<F, RT, T>::result)
		{
			while (true)
			{
				// Read the old value from memory
				const stype old = read_relaxed(m_data);

				// Copy the old value
				stype _new = old;

				// Call atomic op for the local copy of the old value and save the return value of the function
				atomic_op_result<F, RT, T> result(func, reinterpret_cast<type&>(_new), args...);

				// Atomically compare value with `old`, replace with `_new` and return on success
				if (sync_bool_compare_and_swap(&m_data, old, _new)) return result.move();
			}
		}

		// Atomic bitwise OR, returns previous data
		type _or(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_or(&m_data, to_subtype(right)));
		}

		// Atomic bitwise AND, returns previous data
		type _and(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_and(&m_data, to_subtype(right)));
		}

		// Atomic bitwise AND NOT (inverts right argument), returns previous data
		type _and_not(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_and(&m_data, ~to_subtype(right)));
		}

		// Atomic bitwise XOR, returns previous data
		type _xor(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_xor(&m_data, to_subtype(right)));
		}

		type operator |=(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_or(&m_data, to_subtype(right)) | to_subtype(right));
		}

		type operator &=(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_and(&m_data, to_subtype(right)) & to_subtype(right));
		}

		type operator ^=(const type& right) volatile
		{
			return from_subtype(sync_fetch_and_xor(&m_data, to_subtype(right)) ^ to_subtype(right));
		}
	};

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), T> operator ++(atomic<T>& left)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), 1) + 1);
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), T> operator --(atomic<T>& left)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), 1) - 1);
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), T> operator ++(atomic<T>& left, int)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), 1));
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), T> operator --(atomic<T>& left, int)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), 1));
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, T> operator +=(atomic<T>& left, const T2& right)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), right) + right);
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, T> operator -=(atomic<T>& left, const T2& right)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), right) - right);
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), nse_t<T>> operator ++(atomic<nse_t<T>>& left)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), 1) + 1);
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), nse_t<T>> operator --(atomic<nse_t<T>>& left)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), 1) - 1);
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), nse_t<T>> operator ++(atomic<nse_t<T>>& left, int)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), 1));
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), nse_t<T>> operator --(atomic<nse_t<T>>& left, int)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), 1));
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, nse_t<T>> operator +=(atomic<nse_t<T>>& left, const T2& right)
	{
		return left.from_subtype(sync_fetch_and_add(left.raw_data(), right) + right);
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, nse_t<T>> operator -=(atomic<nse_t<T>>& left, const T2& right)
	{
		return left.from_subtype(sync_fetch_and_sub(left.raw_data(), right) - right);
	}

	template<typename T> inline std::enable_if_t<IS_INTEGRAL(T), se_t<T>> operator ++(atomic<se_t<T>>& left)
	{
		return left.atomic_op([](se_t<T>& value) -> se_t<T>
		{
			return ++value;
		});
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), se_t<T>> operator --(atomic<se_t<T>>& left)
	{
		return left.atomic_op([](se_t<T>& value) -> se_t<T>
		{
			return --value;
		});
	}

	template<typename T> inline std::enable_if_t<IS_INTEGRAL(T), se_t<T>>
		operator ++(atomic<se_t<T>>& left, int)
	{
		return left.atomic_op([](se_t<T>& value) -> se_t<T>
		{
			return value++;
		});
	}

	template<typename T>
	inline std::enable_if_t<IS_INTEGRAL(T), se_t<T>> operator --(atomic<se_t<T>>& left, int)
	{
		return left.atomic_op([](se_t<T>& value) -> se_t<T>
		{
			return value--;
		});
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, se_t<T>> operator +=(atomic<se_t<T>>& left, const T2& right)
	{
		return left.atomic_op([&](se_t<T>& value) -> se_t<T>
		{
			return value += right;
		});
	}

	template<typename T, typename T2>
	inline std::enable_if_t<IS_INTEGRAL(T) && std::is_convertible<T2, T>::value, se_t<T>> operator -=(atomic<se_t<T>>& left, const T2& right)
	{
		return left.atomic_op([&](se_t<T>& value) -> se_t<T>
		{
			return value -= right;
		});
	}

	// Atomic BE Type (for PS3 virtual memory)
	template<typename T> using atomic_be_t = atomic<be_t<T>>;

	// Atomic LE Type (for PSV virtual memory)
	template<typename T> using atomic_le_t = atomic<le_t<T>>;

	// Algorithm for std::atomic; similar to atomic::atomic_op()
	template<typename T, typename F, typename... Args, typename RT = std::result_of_t<F(T&, Args...)>>
	auto atomic_op(std::atomic<T>& var, F func, Args&&... args) -> decltype(atomic_op_result<F, RT, T>::result)
	{
		auto old = var.load();

		while (true)
		{
			auto _new = old;

			atomic_op_result<F, RT, T> result(func, _new, args...);

			if (var.compare_exchange_strong(old, _new)) return result.move();
		}
	}
}