#pragma once

class PPUThread;
struct ARMv7Context;

namespace vm
{
	// helper SFINAE type for vm::_ptr_base comparison operators (enables comparison between equal types and between any type and void*)
	template<typename T1, typename T2, typename RT = void> using if_comparable_t = std::enable_if_t<
		std::is_void<T1>::value ||
		std::is_void<T2>::value ||
		std::is_same<std::remove_cv_t<T1>, std::remove_cv_t<T2>>::value,
		RT>;

	// helper SFINAE type for vm::_ptr_base pointer arithmetic operators and indirection (disabled for void and function pointers)
	template<typename T, typename RT = void> using if_arithmetical_ptr_t = std::enable_if_t<
		!std::is_void<T>::value &&
		!std::is_function<T>::value,
		RT>;

	template<typename T, typename AT = u32>
	struct _ptr_base
	{
		AT m_addr; // don't access directly

		using type = T;

		static_assert(!std::is_pointer<T>::value, "vm::_ptr_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ptr_base<> error: invalid type (reference)");

		AT addr() const
		{
			return m_addr;
		}

		// get vm pointer to member
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ptr_base<MT> of(MT T2::*const member) const
		{
			const u32 offset = static_cast<u32>(reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T*>(0ull)->*member)));
			return{ vm::cast(m_addr + offset) };
		}

		// get vm pointer to array member with array subscribtion
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ptr_base<std::remove_extent_t<MT>> of(MT T2::*const member, u32 index) const
		{
			const u32 offset = static_cast<u32>(reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<T*>(0ull)->*member)));
			return{ vm::cast(m_addr + offset + sizeof32(T) * index) };
		}
		
		template<typename CT> std::enable_if_t<std::is_assignable<AT&, CT>::value> set(const CT& value)
		{
			m_addr = value;
		}

		template<typename AT2 = AT> static std::enable_if_t<std::is_constructible<AT, AT2>::value, _ptr_base> make(const AT2& addr)
		{
			return{ addr };
		}

		T* get_ptr() const
		{
			return vm::get_ptr<T>(vm::cast(m_addr));
		}

		T* priv_ptr() const
		{
			return vm::priv_ptr<T>(vm::cast(m_addr));
		}

		T* operator ->() const
		{
			return get_ptr();
		}

		std::add_lvalue_reference_t<T> operator [](u32 index) const
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator[] is not available for void pointers");

			return vm::get_ref<T>(vm::cast(m_addr + sizeof32(T) * index));
		}

		// enable only the conversions which are originally possible between pointer types
		template<typename T2, typename AT2, typename = std::enable_if_t<std::is_convertible<T*, T2*>::value>> operator _ptr_base<T2, AT2>() const
		{
			return{ vm::cast(m_addr) };
		}

		template<typename T2, typename = std::enable_if_t<std::is_convertible<T*, T2*>::value>> explicit operator T2*() const
		{
			return get_ptr();
		}

		explicit operator bool() const
		{
			return m_addr != 0;
		}

		// test address alignment using alignof(T)
		bool aligned() const
		{
			return m_addr % alignof32(T) == 0;
		}

		// test address for arbitrary alignment or something
		force_inline explicit_bool_t operator %(to_ne_t<AT> right) const
		{
			return m_addr % right;
		}

		_ptr_base& operator =(const _ptr_base&) = default;
	};

	template<typename AT, typename RT, typename... T>
	struct _ptr_base<RT(T...), AT>
	{
		AT m_addr;

		using type = func_def<RT(T...)>;

		AT addr() const
		{
			return m_addr;
		}

		template<typename CT> std::enable_if_t<std::is_assignable<AT&, CT>::value> set(const CT& value)
		{
			m_addr = value;
		}

		template<typename AT2 = AT> static std::enable_if_t<std::is_constructible<AT, AT2>::value, _ptr_base> make(const AT2& addr)
		{
			return{ addr };
		}

		// defined in CB_FUNC.h, passing context is mandatory
		RT operator()(PPUThread& CPU, T... args) const;

		// defined in ARMv7Callback.h, passing context is mandatory
		RT operator()(ARMv7Context& context, T... args) const;

		// conversion to another function pointer
		template<typename AT2> operator _ptr_base<type, AT2>() const
		{
			return{ vm::cast(m_addr) };
		}

		explicit operator bool() const
		{
			return m_addr != 0;
		}

		_ptr_base& operator =(const _ptr_base&) = default;
	};

	template<typename AT, typename RT, typename... T>
	struct _ptr_base<RT(*)(T...), AT>
	{
		AT m_addr;

		static_assert(!sizeof(AT), "vm::_ptr_base<> error: use RT(T...) format for functions instead of RT(*)(T...)");
	};

	// Native endianness pointer to LE data
	template<typename T, typename AT = u32> using ptrl = _ptr_base<to_le_t<T>, AT>;

	// Native endianness pointer to BE data
	template<typename T, typename AT = u32> using ptrb = _ptr_base<to_be_t<T>, AT>;

	// BE pointer to LE data
	template<typename T, typename AT = u32> using bptrl = _ptr_base<to_le_t<T>, to_be_t<AT>>;

	// BE pointer to BE data
	template<typename T, typename AT = u32> using bptrb = _ptr_base<to_be_t<T>, to_be_t<AT>>;

	// LE pointer to LE data
	template<typename T, typename AT = u32> using lptrl = _ptr_base<to_le_t<T>, to_le_t<AT>>;

	// LE pointer to BE data
	template<typename T, typename AT = u32> using lptrb = _ptr_base<to_be_t<T>, to_le_t<AT>>;

	namespace ps3
	{
		// default pointer for PS3 HLE functions (Native endianness pointer to BE data)
		template<typename T, typename AT = u32> using ptr = ptrb<T, AT>;

		// default pointer to pointer for PS3 HLE functions (Native endianness pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using pptr = ptr<ptr<T, AT2>, AT>;

		// default pointer for PS3 HLE structures (BE pointer to BE data)
		template<typename T, typename AT = u32> using bptr = bptrb<T, AT>;

		// default pointer to pointer for PS3 HLE structures (BE pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using bpptr = bptr<ptr<T, AT2>, AT>;

		// native endianness pointer to const BE data
		template<typename T, typename AT = u32> using cptr = ptr<const T, AT>;

		// BE pointer to const BE data
		template<typename T, typename AT = u32> using bcptr = bptr<const T, AT>;

		template<typename T, typename AT = u32> using cpptr = pptr<const T, AT>;
		template<typename T, typename AT = u32> using bcpptr = bpptr<const T, AT>;
	}

	namespace psv
	{
		// default pointer for PSV HLE functions (Native endianness pointer to LE data)
		template<typename T> using ptr = ptrl<T>;

		// default pointer to pointer for PSV HLE functions (Native endianness pointer to LE pointer to LE data)
		template<typename T> using pptr = ptr<ptr<T>>;

		// default pointer for PSV HLE structures (LE pointer to LE data)
		template<typename T> using lptr = lptrl<T>;

		// default pointer to pointer for PSV HLE structures (LE pointer to LE pointer to LE data)
		template<typename T> using lpptr = lptr<ptr<T>>;

		// native endianness pointer to const LE data
		template<typename T> using cptr = ptr<const T>;

		// LE pointer to const LE data
		template<typename T> using lcptr = lptr<const T>;

		template<typename T> using cpptr = pptr<const T>;
		template<typename T> using lcpptr = lpptr<const T>;
	}

	struct null_t
	{
		template<typename T, typename AT> operator _ptr_base<T, AT>() const
		{
			return{};
		}
	};

	// vm::null is convertible to any vm::ptr type as null pointer in virtual memory
	static null_t null;

	// perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
	template<typename CT, typename T, typename AT, typename = decltype(static_cast<CT*>(std::declval<T*>()))> inline _ptr_base<CT, AT> static_ptr_cast(const _ptr_base<T, AT>& other)
	{
		return{ other.m_addr };
	}

	// perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
	template<typename CT, typename T, typename AT, typename = decltype(const_cast<CT*>(std::declval<T*>()))> inline _ptr_base<CT, AT> const_ptr_cast(const _ptr_base<T, AT>& other)
	{
		return{ other.m_addr };
	}

	// perform reinterpret_cast (for example, vm::ptr<char> to vm::ptr<u32>)
	template<typename CT, typename T, typename AT, typename = decltype(reinterpret_cast<CT*>(std::declval<T*>()))> inline _ptr_base<CT, AT> reinterpret_ptr_cast(const _ptr_base<T, AT>& other)
	{
		return{ other.m_addr };
	}
}

// unary plus operator for vm::_ptr_base (always available)
template<typename T, typename AT> inline vm::_ptr_base<T, AT> operator +(const vm::_ptr_base<T, AT>& ptr)
{
	return ptr;
}

// indirection operator for vm::_ptr_base
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, T&> operator *(const vm::_ptr_base<T, AT>& ptr)
{
	return vm::get_ref<T>(vm::cast(ptr.m_addr));
}

// postfix increment operator for vm::_ptr_base
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>> operator ++(vm::_ptr_base<T, AT>& ptr, int)
{
	const AT result = ptr.m_addr;
	ptr.m_addr += sizeof32(T);
	return{ result };
}

// prefix increment operator for vm::_ptr_base
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>&> operator ++(vm::_ptr_base<T, AT>& ptr)
{
	ptr.m_addr += sizeof32(T);
	return ptr;
}

// postfix decrement operator for vm::_ptr_base
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>> operator --(vm::_ptr_base<T, AT>& ptr, int)
{
	const AT result = ptr.m_addr;
	ptr.m_addr -= sizeof32(T);
	return{ result };
}

// prefix decrement operator for vm::_ptr_base
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>&> operator --(vm::_ptr_base<T, AT>& ptr)
{
	ptr.m_addr -= sizeof32(T);
	return ptr;
}

// addition assignment operator for vm::_ptr_base (pointer += integer)
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>&> operator +=(vm::_ptr_base<T, AT>& ptr, to_ne_t<AT> count)
{
	ptr.m_addr += count * sizeof32(T);
	return ptr;
}

// subtraction assignment operator for vm::_ptr_base (pointer -= integer)
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>&> operator -=(vm::_ptr_base<T, AT>& ptr, to_ne_t<AT> count)
{
	ptr.m_addr -= count * sizeof32(T);
	return ptr;
}

// addition operator for vm::_ptr_base (pointer + integer)
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>> operator +(const vm::_ptr_base<T, AT>& ptr, to_ne_t<AT> count)
{
	return{ ptr.m_addr + count * sizeof32(T) };
}

// addition operator for vm::_ptr_base (integer + pointer)
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>> operator +(to_ne_t<AT> count, const vm::_ptr_base<T, AT>& ptr)
{
	return{ ptr.m_addr + count * sizeof32(T) };
}

// subtraction operator for vm::_ptr_base (pointer - integer)
template<typename T, typename AT> inline vm::if_arithmetical_ptr_t<T, vm::_ptr_base<T, AT>> operator -(const vm::_ptr_base<T, AT>& ptr, to_ne_t<AT> count)
{
	return{ ptr.m_addr - count * sizeof32(T) };
}

// pointer difference operator for vm::_ptr_base
template<typename T1, typename AT1, typename T2, typename AT2> inline std::enable_if_t<
	!std::is_void<T1>::value &&
	!std::is_void<T2>::value &&
	!std::is_function<T1>::value &&
	!std::is_function<T2>::value &&
	std::is_same<std::remove_cv_t<T1>, std::remove_cv_t<T2>>::value,
	s32> operator -(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return static_cast<s32>(left.m_addr - right.m_addr) / sizeof32(T1);
}

// comparison operator for vm::_ptr_base (pointer1 == pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator ==(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr == right.m_addr;
}

template<typename T, typename AT> bool operator ==(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.m_addr == 0;
}

template<typename T, typename AT> bool operator ==(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.m_addr == 0;
}

// comparison operator for vm::_ptr_base (pointer1 != pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator !=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr != right.m_addr;
}

template<typename T, typename AT> bool operator !=(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.m_addr != 0;
}

template<typename T, typename AT> bool operator !=(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.m_addr != 0;
}

// comparison operator for vm::_ptr_base (pointer1 < pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator <(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr < right.m_addr;
}

template<typename T, typename AT> bool operator <(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.m_addr != 0;
}

template<typename T, typename AT> bool operator <(const vm::_ptr_base<T, AT>&, const vm::null_t&)
{
	return false;
}

// comparison operator for vm::_ptr_base (pointer1 <= pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator <=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr <= right.m_addr;
}

template<typename T, typename AT> bool operator <=(const vm::null_t&, const vm::_ptr_base<T, AT>&)
{
	return true;
}

template<typename T, typename AT> bool operator <=(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.m_addr == 0;
}

// comparison operator for vm::_ptr_base (pointer1 > pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator >(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr > right.m_addr;
}

template<typename T, typename AT> bool operator >(const vm::null_t&, const vm::_ptr_base<T, AT>&)
{
	return false;
}

template<typename T, typename AT> bool operator >(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.m_addr != 0;
}

// comparison operator for vm::_ptr_base (pointer1 >= pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> vm::if_comparable_t<T1, T2, bool> operator >=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.m_addr >= right.m_addr;
}

template<typename T, typename AT> bool operator >=(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.m_addr == 0;
}

template<typename T, typename AT> bool operator >=(const vm::_ptr_base<T, AT>&, const vm::null_t&)
{
	return true;
}

// external specialization for is_be_t<> (true if AT is be_t<>)

template<typename T, typename AT>
struct is_be_t<vm::_ptr_base<T, AT>> : public std::integral_constant<bool, is_be_t<AT>::value>
{
};

// external specialization for is_le_t<> (true if AT is le_t<>)

template<typename T, typename AT>
struct is_le_t<vm::_ptr_base<T, AT>> : public std::integral_constant<bool, is_le_t<AT>::value>
{
};

// external specialization for to_ne_t<> (change AT endianness to native)

template<typename T, typename AT>
struct to_ne<vm::_ptr_base<T, AT>>
{
	using type = vm::_ptr_base<T, to_ne_t<AT>>;
};

// external specialization for to_be_t<> (change AT endianness to BE)

template<typename T, typename AT>
struct to_be<vm::_ptr_base<T, AT>>
{
	using type = vm::_ptr_base<T, to_be_t<AT>>;
};

// external specialization for to_le_t<> (change AT endianness to LE)

template<typename T, typename AT>
struct to_le<vm::_ptr_base<T, AT>>
{
	using type = vm::_ptr_base<T, to_le_t<AT>>;
};

namespace fmt
{
	// external specialization for fmt::format function

	template<typename T, typename AT>
	struct unveil<vm::_ptr_base<T, AT>, false>
	{
		using result_type = typename unveil<AT>::result_type;

		force_inline static result_type get_value(const vm::_ptr_base<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum>
struct cast_ppu_gpr;

template<typename T, typename AT>
struct cast_ppu_gpr<vm::_ptr_base<T, AT>, false>
{
	force_inline static u64 to_gpr(const vm::_ptr_base<T, AT>& value)
	{
		return cast_ppu_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ptr_base<T, AT> from_gpr(const u64 reg)
	{
		return vm::_ptr_base<T, AT>::make(cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};

// external specializations for ARMv7 GPR

template<typename T, bool is_enum>
struct cast_armv7_gpr;

template<typename T, typename AT>
struct cast_armv7_gpr<vm::_ptr_base<T, AT>, false>
{
	force_inline static u32 to_gpr(const vm::_ptr_base<T, AT>& value)
	{
		return cast_armv7_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ptr_base<T, AT> from_gpr(const u32 reg)
	{
		return vm::_ptr_base<T, AT>::make(cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
