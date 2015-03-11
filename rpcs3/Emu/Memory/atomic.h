#pragma once

#undef InterlockedExchange
#undef InterlockedCompareExchange
#undef InterlockedOr
#undef InterlockedAnd
#undef InterlockedXor

template<typename T, size_t size = sizeof(T)>
struct _to_atomic
{
	static_assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 16, "Invalid atomic type");

	typedef T type;
};

template<typename T>
struct _to_atomic<T, 1>
{
	typedef uint8_t type;
};

template<typename T>
struct _to_atomic<T, 2>
{
	typedef uint16_t type;
};

template<typename T>
struct _to_atomic<T, 4>
{
	typedef uint32_t type;
};

template<typename T>
struct _to_atomic<T, 8>
{
	typedef uint64_t type;
};

template<typename T>
struct _to_atomic<T, 16>
{
	typedef u128 type;
};

template<typename T>
class _atomic_base
{
	typedef typename _to_atomic<T, sizeof(T)>::type atomic_type;
	atomic_type data;

public:
	// atomically compare data with cmp, replace with exch if equal, return previous data value anyway
	__forceinline const T compare_and_swap(const T& cmp, const T& exch) volatile
	{
		const atomic_type res = InterlockedCompareExchange(&data, (atomic_type&)(exch), (atomic_type&)(cmp));
		return (T&)res;
	}

	// atomically compare data with cmp, replace with exch if equal, return true if data was replaced
	__forceinline bool compare_and_swap_test(const T& cmp, const T& exch) volatile
	{
		return InterlockedCompareExchangeTest(&data, (atomic_type&)(exch), (atomic_type&)(cmp));
	}

	// read data with memory barrier
	__forceinline const T read_sync() const volatile 
	{
		const atomic_type res = InterlockedCompareExchange(const_cast<volatile atomic_type*>(&data), 0, 0);
		return (T&)res;
	}

	// atomically replace data with exch, return previous data value
	__forceinline const T exchange(const T& exch) volatile
	{
		const atomic_type res = InterlockedExchange(&data, (atomic_type&)(exch));
		return (T&)res;
	}

	// read data without memory barrier
	__forceinline const T read_relaxed() const volatile
	{
		return (T&)data;
	}

	// write data without memory barrier
	__forceinline void write_relaxed(const T& value)
	{
		data = (atomic_type&)(value);
	}

	// perform atomic operation on data
	template<typename FT> __forceinline void atomic_op(const FT atomic_proc) volatile
	{
		while (true)
		{
			const T old = read_relaxed();
			T _new = old;
			atomic_proc(_new); // function should accept reference to T type
			if (compare_and_swap_test(old, _new)) return;
		}
	}

	// perform atomic operation on data with special exit condition (if intermediate result != proceed_value)
	template<typename RT, typename FT> __forceinline RT atomic_op(const RT proceed_value, const FT atomic_proc) volatile
	{
		while (true)
		{
			const T old = read_relaxed();
			T _new = old;
			RT res = (RT)atomic_proc(_new); // function should accept reference to T type and return some value
			if (res != proceed_value) return res;
			if (compare_and_swap_test(old, _new)) return proceed_value;
		}
	}

	// perform atomic operation on data with additional memory barrier
	template<typename FT> __forceinline void atomic_op_sync(const FT atomic_proc) volatile
	{
		T old = read_sync();
		while (true)
		{
			T _new = old;
			atomic_proc(_new); // function should accept reference to T type
			const T val = compare_and_swap(old, _new);
			if ((atomic_type&)val == (atomic_type&)old) return;
			old = val;
		}
	}

	// perform atomic operation on data with additional memory barrier and special exit condition (if intermediate result != proceed_value)
	template<typename RT, typename FT> __forceinline RT atomic_op_sync(const RT proceed_value, const FT atomic_proc) volatile
	{
		T old = read_sync();
		while (true)
		{
			T _new = old;
			RT res = (RT)atomic_proc(_new); // function should accept reference to T type and return some value
			if (res != proceed_value) return res;
			const T val = compare_and_swap(old, _new);
			if ((atomic_type&)val == (atomic_type&)old) return proceed_value;
			old = val;
		}
	}

	// perform non-atomic operation on data directly without memory barriers
	template<typename FT> __forceinline void direct_op(const FT direct_proc) volatile
	{
		direct_proc((T&)data);
	}

	// atomic bitwise OR, returns previous data
	__forceinline const T _or(const T& right) volatile
	{
		const atomic_type res = InterlockedOr(&data, (atomic_type&)(right));
		return (T&)res;
	}

	// atomic bitwise AND, returns previous data
	__forceinline const T _and(const T& right) volatile
	{
		const atomic_type res = InterlockedAnd(&data, (atomic_type&)(right));
		return (T&)res;
	}

	// atomic bitwise AND NOT (inverts right argument), returns previous data
	__forceinline const T _and_not(const T& right) volatile
	{
		const atomic_type res = InterlockedAnd(&data, ~(atomic_type&)(right));
		return (T&)res;
	}

	// atomic bitwise XOR, returns previous data
	__forceinline const T _xor(const T& right) volatile
	{
		const atomic_type res = InterlockedXor(&data, (atomic_type&)(right));
		return (T&)res;
	}

	__forceinline const T operator |= (const T& right) volatile
	{
		const atomic_type res = InterlockedOr(&data, (atomic_type&)(right)) | (atomic_type&)(right);
		return (T&)res;
	}

	__forceinline const T operator &= (const T& right) volatile
	{
		const atomic_type res = InterlockedAnd(&data, (atomic_type&)(right)) & (atomic_type&)(right);
		return (T&)res;
	}

	__forceinline const T operator ^= (const T& right) volatile
	{
		const atomic_type res = InterlockedXor(&data, (atomic_type&)(right)) ^ (atomic_type&)(right);
		return (T&)res;
	}
};

// Helper definitions
template<typename T, typename T2 = T> using if_arithmetic_t = typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, T>::type;
template<typename T, typename T2 = T> using if_arithmetic_be_t = typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, be_t<T>>::type;
template<typename T, typename T2 = T> using if_arithmetic_atomic_t = typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, _atomic_base<T>>::type;
template<typename T, typename T2 = T> using if_arithmetic_atomic_be_t = typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, _atomic_base<be_t<T>>>::type;

template<typename T> inline static if_arithmetic_t<T> operator ++(_atomic_base<T>& left)
{
	T result;

	left.atomic_op([&result](T& value)
	{
		result = ++value;
	});

	return result;
}

template<typename T> inline static if_arithmetic_t<T> operator --(_atomic_base<T>& left)
{
	T result;

	left.atomic_op([&result](T& value)
	{
		result = --value;
	});

	return result;
}

template<typename T> inline static if_arithmetic_t<T> operator ++(_atomic_base<T>& left, int)
{
	T result;

	left.atomic_op([&result](T& value)
	{
		result = value++;
	});

	return result;
}

template<typename T> inline static if_arithmetic_t<T> operator --(_atomic_base<T>& left, int)
{
	T result;

	left.atomic_op([&result](T& value)
	{
		result = value--;
	});

	return result;
}

template<typename T, typename T2> inline static if_arithmetic_t<T, T2> operator +=(_atomic_base<T>& left, T2 right)
{
	T result;

	left.atomic_op([&result, right](T& value)
	{
		result = (value += right);
	});

	return result;
}

template<typename T, typename T2> inline static if_arithmetic_t<T, T2> operator -=(_atomic_base<T>& left, T2 right)
{
	T result;

	left.atomic_op([&result, right](T& value)
	{
		result = (value -= right);
	});

	return result;
}

template<typename T> inline static if_arithmetic_be_t<T> operator ++(_atomic_base<be_t<T>>& left)
{
	be_t<T> result;

	left.atomic_op([&result](be_t<T>& value)
	{
		result = ++value;
	});

	return result;
}

template<typename T> inline static if_arithmetic_be_t<T> operator --(_atomic_base<be_t<T>>& left)
{
	be_t<T> result;

	left.atomic_op([&result](be_t<T>& value)
	{
		result = --value;
	});

	return result;
}

template<typename T> inline static if_arithmetic_be_t<T> operator ++(_atomic_base<be_t<T>>& left, int)
{
	be_t<T> result;

	left.atomic_op([&result](be_t<T>& value)
	{
		result = value++;
	});

	return result;
}

template<typename T> inline static if_arithmetic_be_t<T> operator --(_atomic_base<be_t<T>>& left, int)
{
	be_t<T> result;

	left.atomic_op([&result](be_t<T>& value)
	{
		result = value--;
	});

	return result;
}

template<typename T, typename T2> inline static if_arithmetic_be_t<T, T2> operator +=(_atomic_base<be_t<T>>& left, T2 right)
{
	be_t<T> result;

	left.atomic_op([&result, right](be_t<T>& value)
	{
		result = (value += right);
	});

	return result;
}

template<typename T, typename T2> inline static if_arithmetic_be_t<T, T2> operator -=(_atomic_base<be_t<T>>& left, T2 right)
{
	be_t<T> result;

	left.atomic_op([&result, right](be_t<T>& value)
	{
		result = (value -= right);
	});

	return result;
}

template<typename T> using atomic_le_t = _atomic_base<T>;

template<typename T> using atomic_be_t = _atomic_base<typename to_be_t<T>::type>;

namespace ps3
{
	template<typename T> using atomic_t = atomic_be_t<T>;
}

namespace psv
{
	template<typename T> using atomic_t = atomic_le_t<T>;
}

using namespace ps3;
