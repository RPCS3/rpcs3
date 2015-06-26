#pragma once

template<typename T, size_t size = sizeof(T)> struct _to_atomic_subtype
{
	static_assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 16, "Invalid atomic type");
};

template<typename T> struct _to_atomic_subtype<T, 1>
{
	using type = u8;
};

template<typename T> struct _to_atomic_subtype<T, 2>
{
	using type = u16;
};

template<typename T> struct _to_atomic_subtype<T, 4>
{
	using type = u32;
};

template<typename T> struct _to_atomic_subtype<T, 8>
{
	using type = u64;
};

template<typename T> struct _to_atomic_subtype<T, 16>
{
	using type = u128;
};

template<typename T> using atomic_subtype_t = typename _to_atomic_subtype<T>::type;

// result wrapper to deal with void result type
template<typename RT> struct atomic_op_result_t
{
	RT result;

	template<typename T, typename... Args> inline atomic_op_result_t(T func, Args&&... args)
		: result(std::move(func(std::forward<Args>(args)...)))
	{
	}

	inline RT move()
	{
		return std::move(result);
	}
};

// void specialization
template<> struct atomic_op_result_t<void>
{
	template<typename T, typename... Args> inline atomic_op_result_t(T func, Args&&... args)
	{
		func(std::forward<Args>(args)...);
	}

	inline void move()
	{
	}
};

template<typename T> union _atomic_base
{
	using type = std::remove_cv_t<T>;
	using subtype = atomic_subtype_t<type>;

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

private:
	template<typename T2> force_inline static void write_relaxed(volatile T2& data, const T2& value)
	{
		data = value;
	}

	force_inline static void write_relaxed(volatile u128& data, const u128& value)
	{
		sync_lock_test_and_set(&data, value);
	}

	template<typename T2> force_inline static T2 read_relaxed(const volatile T2& data)
	{
		return data;
	}

	force_inline static u128 read_relaxed(const volatile u128& value)
	{
		return sync_val_compare_and_swap(const_cast<volatile u128*>(&value), {}, {});
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
	force_inline const type load_sync() const volatile
	{
		const subtype zero = {};
		return from_subtype(sync_val_compare_and_swap(const_cast<subtype*>(&sub_data), zero, zero));
	}

	// atomically replace data with exch, return previous data value
	force_inline const type exchange(const type& exch) volatile
	{
		return from_subtype(sync_lock_test_and_set(&sub_data, to_subtype(exch)));
	}

	// read data without memory barrier (works as load_sync() for 128 bit)
	force_inline const type load() const volatile
	{
		return from_subtype(read_relaxed(sub_data));
	}

	// write data without memory barrier (works as exchange() for 128 bit, discarding result)
	force_inline void store(const type& value) volatile
	{
		write_relaxed(sub_data, to_subtype(value));
	}

	// perform an atomic operation on data (callable object version, first arg is a reference to atomic type)
	template<typename F, typename... Args> auto atomic_op(F func, Args&&... args) volatile -> decltype(func(std::declval<T&>(), args...))
	{
		while (true)
		{
			// read the old value from memory
			const subtype old = read_relaxed(sub_data);

			// copy the old value
			subtype _new = old;

			// call atomic op for the local copy of the old value and save the return value of the function
			atomic_op_result_t<std::result_of_t<F(T&, Args...)>> result(func, to_type(_new), args...);

			// atomically compare value with `old`, replace with `_new` and return on success
			if (sync_bool_compare_and_swap(&sub_data, old, _new)) return result.move();
		}
	}

	// perform an atomic operation on data (member function version)
	template<typename RT, typename... FArgs, typename CT, typename... Args, typename = std::enable_if_t<std::is_same<T, CT>::value>> auto atomic_op(RT(CT::* func)(FArgs...), Args&&... args) volatile -> decltype((std::declval<T&>().*func)(args...))
	{
		return atomic_op(std::mem_fn(func), std::forward<Args>(args)...);
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

	force_inline const type operator |=(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_or(&sub_data, to_subtype(right)) | to_subtype(right));
	}

	force_inline const type operator &=(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_and(&sub_data, to_subtype(right)) & to_subtype(right));
	}

	force_inline const type operator ^=(const type& right) volatile
	{
		return from_subtype(sync_fetch_and_xor(&sub_data, to_subtype(right)) ^ to_subtype(right));
	}
};

template<typename T, typename T2 = T> using if_integral_le_t = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value, le_t<T>>;
template<typename T, typename T2 = T> using if_integral_be_t = std::enable_if_t<std::is_integral<T>::value && std::is_integral<T2>::value, be_t<T>>;

template<typename T> inline if_integral_le_t<T> operator ++(_atomic_base<le_t<T>>& left)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, 1) + 1);
}

template<typename T> inline if_integral_le_t<T> operator --(_atomic_base<le_t<T>>& left)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, 1) - 1);
}

template<typename T> inline if_integral_le_t<T> operator ++(_atomic_base<le_t<T>>& left, int)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, 1));
}

template<typename T> inline if_integral_le_t<T> operator --(_atomic_base<le_t<T>>& left, int)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, 1));
}

template<typename T, typename T2> inline if_integral_le_t<T, T2> operator +=(_atomic_base<le_t<T>>& left, T2 right)
{
	return left.from_subtype(sync_fetch_and_add(&left.sub_data, right) + right);
}

template<typename T, typename T2> inline if_integral_le_t<T, T2> operator -=(_atomic_base<le_t<T>>& left, T2 right)
{
	return left.from_subtype(sync_fetch_and_sub(&left.sub_data, right) - right);
}

template<typename T> inline if_integral_be_t<T> operator ++(_atomic_base<be_t<T>>& left)
{
	return left.atomic_op([](be_t<T>& value) -> be_t<T>
	{
		return ++value;
	});
}

template<typename T> inline if_integral_be_t<T> operator --(_atomic_base<be_t<T>>& left)
{
	return left.atomic_op([](be_t<T>& value) -> be_t<T>
	{
		return --value;
	});
}

template<typename T> inline if_integral_be_t<T> operator ++(_atomic_base<be_t<T>>& left, int)
{
	return left.atomic_op([](be_t<T>& value) -> be_t<T>
	{
		return value++;
	});
}

template<typename T> inline if_integral_be_t<T> operator --(_atomic_base<be_t<T>>& left, int)
{
	return left.atomic_op([](be_t<T>& value) -> be_t<T>
	{
		return value--;
	});
}

template<typename T, typename T2> inline if_integral_be_t<T, T2> operator +=(_atomic_base<be_t<T>>& left, T2 right)
{
	return left.atomic_op([right](be_t<T>& value) -> be_t<T>
	{
		return value += right;
	});
}

template<typename T, typename T2> inline if_integral_be_t<T, T2> operator -=(_atomic_base<be_t<T>>& left, T2 right)
{
	return left.atomic_op([right](be_t<T>& value) -> be_t<T>
	{
		return value -= right;
	});
}

template<typename T> using atomic = _atomic_base<T>; // Atomic Type with native endianness (for emulator memory)

template<typename T> using atomic_be_t = _atomic_base<to_be_t<T>>; // Atomic BE Type (for PS3 virtual memory)

template<typename T> using atomic_le_t = _atomic_base<to_le_t<T>>; // Atomic LE Type (for PSV virtual memory)
