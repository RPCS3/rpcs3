#pragma once

template<typename T, size_t size = sizeof(T)>
struct _to_atomic_subtype
{
	static_assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 16, "Invalid atomic type");
};

template<typename T>
struct _to_atomic_subtype<T, 1>
{
	using type = uint8_t;
};

template<typename T>
struct _to_atomic_subtype<T, 2>
{
	using type = uint16_t;
};

template<typename T>
struct _to_atomic_subtype<T, 4>
{
	using type = uint32_t;
};

template<typename T>
struct _to_atomic_subtype<T, 8>
{
	using type = uint64_t;
};

template<typename T>
struct _to_atomic_subtype<T, 16>
{
	using type = u128;
};

template<typename T>
union _atomic_base
{
	using type = typename std::remove_cv<T>::type;
	using subtype = typename _to_atomic_subtype<type, sizeof(type)>::type;

	type data; // unsafe direct access
	subtype sub_data; // unsafe direct access to substitute type

	force_inline static const subtype to_subtype(const type& value)
	{
		return reinterpret_cast<const subtype&>(value);
	}

	force_inline static const type from_subtype(const subtype value)
	{
		return reinterpret_cast<const type&>(value);
	}

	force_inline static type& to_type(subtype& value)
	{
		return reinterpret_cast<type&>(value);
	}

public:
	// atomically compare data with cmp, replace with exch if equal, return previous data value anyway
	force_inline const type compare_and_swap(const type& cmp, const type& exch) volatile
	{
		return from_subtype(sync_val_compare_and_swap(&sub_data, to_subtype(cmp), to_subtype(exch)));
	}

	// atomically compare data with cmp, replace with exch if equal, return true if data was replaced
	force_inline bool compare_and_swap_test(const type& cmp, const type& exch) volatile
	{
		return sync_bool_compare_and_swap(&sub_data, to_subtype(cmp), to_subtype(exch));
	}

	// read data with memory barrier
	force_inline const type read_sync() const volatile
	{
		const subtype zero = {};
		return from_subtype(sync_val_compare_and_swap(const_cast<subtype*>(&sub_data), zero, zero));
	}

	// atomically replace data with exch, return previous data value
	force_inline const type exchange(const type& exch) volatile
	{
		return from_subtype(sync_lock_test_and_set(&sub_data, to_subtype(exch)));
	}

	// read data without memory barrier
	force_inline const type read_relaxed() const volatile
	{
		const subtype value = const_cast<const subtype&>(sub_data);
		return from_subtype(value);
	}

	// write data without memory barrier
	force_inline void write_relaxed(const type& value) volatile
	{
		const_cast<subtype&>(sub_data) = to_subtype(value);
	}

	// perform atomic operation on data
	template<typename FT> force_inline void atomic_op(const FT atomic_proc) volatile
	{
		while (true)
		{
			const subtype old = const_cast<const subtype&>(sub_data);
			subtype _new = old;
			atomic_proc(to_type(_new)); // function should accept reference to T type
			if (sync_bool_compare_and_swap(&sub_data, old, _new)) return;
		}
	}

	// perform atomic operation on data with special exit condition (if intermediate result != proceed_value)
	template<typename RT, typename FT> force_inline RT atomic_op(const RT proceed_value, const FT atomic_proc) volatile
	{
		while (true)
		{
			const subtype old = const_cast<const subtype&>(sub_data);
			subtype _new = old;
			auto res = static_cast<RT>(atomic_proc(to_type(_new))); // function should accept reference to T type and return some value
			if (res != proceed_value) return res;
			if (sync_bool_compare_and_swap(&sub_data, old, _new)) return proceed_value;
		}
	}

	// perform atomic operation on data with additional memory barrier
	template<typename FT> force_inline void atomic_op_sync(const FT atomic_proc) volatile
	{
		const subtype zero = {};
		subtype old = sync_val_compare_and_swap(&sub_data, zero, zero);
		while (true)
		{
			subtype _new = old;
			atomic_proc(to_type(_new)); // function should accept reference to T type
			const subtype val = sync_val_compare_and_swap(&sub_data, old, _new);
			if (val == old) return;
			old = val;
		}
	}

	// perform atomic operation on data with additional memory barrier and special exit condition (if intermediate result != proceed_value)
	template<typename RT, typename FT> force_inline RT atomic_op_sync(const RT proceed_value, const FT atomic_proc) volatile
	{
		const subtype zero = {};
		subtype old = sync_val_compare_and_swap(&sub_data, zero, zero);
		while (true)
		{
			subtype _new = old;
			auto res = static_cast<RT>(atomic_proc(to_type(_new))); // function should accept reference to T type and return some value
			if (res != proceed_value) return res;
			const subtype val = sync_val_compare_and_swap(&sub_data, old, _new);
			if (val == old) return proceed_value;
			old = val;
		}
	}

	// atomic bitwise OR, returns previous data
	force_inline const type _or(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_or(&sub_data, to_subtype(right)));
	}

	// atomic bitwise AND, returns previous data
	force_inline const type _and(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_and(&sub_data, to_subtype(right)));
	}

	// atomic bitwise AND NOT (inverts right argument), returns previous data
	force_inline const type _and_not(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_and(&sub_data, ~to_subtype(right)));
	}

	// atomic bitwise XOR, returns previous data
	force_inline const type _xor(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_xor(&sub_data, to_subtype(right)));
	}

	force_inline const type operator |= (const type& right) volatile
	{
		return from_subtype(sync_fetch_and_or(&sub_data, to_subtype(right)) | to_subtype(right));
	}

	force_inline const type operator &= (const type& right) volatile
	{
		return from_subtype(sync_fetch_and_and(&sub_data, to_subtype(right)) & to_subtype(right));
	}

	force_inline const type operator ^= (const type& right) volatile
	{
		return from_subtype(sync_fetch_and_xor(&sub_data, to_subtype(right)) ^ to_subtype(right));
	}
};

// Helper definitions

template<typename T, typename T2 = T> using if_arithmetic_le_t = const typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, le_t<T>>::type;
template<typename T, typename T2 = T> using if_arithmetic_be_t = const typename std::enable_if<std::is_arithmetic<T>::value && std::is_arithmetic<T2>::value, be_t<T>>::type;

template<typename T> inline static if_arithmetic_le_t<T> operator ++(_atomic_base<le_t<T>>& left)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, 1) + 1);
}

template<typename T> inline static if_arithmetic_le_t<T> operator --(_atomic_base<le_t<T>>& left)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, 1) - 1);
}

template<typename T> inline static if_arithmetic_le_t<T> operator ++(_atomic_base<le_t<T>>& left, int)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, 1));
}

template<typename T> inline static if_arithmetic_le_t<T> operator --(_atomic_base<le_t<T>>& left, int)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, 1));
}

template<typename T, typename T2> inline static if_arithmetic_le_t<T, T2> operator +=(_atomic_base<le_t<T>>& left, T2 right)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, right) + right);
}

template<typename T, typename T2> inline static if_arithmetic_le_t<T, T2> operator -=(_atomic_base<le_t<T>>& left, T2 right)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, right) - right);
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

template<typename T> using atomic = _atomic_base<T>; // Atomic Type with native endianness (for emulator memory)

template<typename T> using atomic_be_t = _atomic_base<typename to_be_t<T>::type>; // Atomic BE Type (for PS3 virtual memory)

template<typename T> using atomic_le_t = _atomic_base<typename to_le_t<T>::type>; // Atomic LE Type (for PSV virtual memory)
