#pragma once

namespace vm
{
	template<typename T, typename AT = u32>
	struct _ref_base
	{
		AT m_addr;

		static_assert(!std::is_pointer<T>::value, "vm::_ref_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ref_base<> error: invalid type (reference)");
		static_assert(!std::is_function<T>::value, "vm::_ref_base<> error: invalid type (function)");
		static_assert(!std::is_void<T>::value, "vm::_ref_base<> error: invalid type (void)");

		AT addr() const
		{
			return m_addr;
		}

		template<typename AT2 = AT> static std::enable_if_t<std::is_constructible<AT, AT2>::value, _ref_base> make(const AT2& addr)
		{
			return{ addr };
		}

		T& get_ref() const
		{
			return vm::get_ref<T>(vm::cast(m_addr));
		}

		T& priv_ref() const
		{
			return vm::priv_ref<T>(vm::cast(m_addr));
		}

		// TODO: conversion operator (seems hard to define it correctly)
		//template<typename CT, typename dummy = std::enable_if_t<std::is_convertible<T, CT>::value || std::is_convertible<to_ne_t<T>, CT>::value>> operator CT() const
		//{
		//	return get_ref();
		//}

		operator to_ne_t<T>() const
		{
			return get_ref();
		}

		explicit operator T&() const
		{
			return get_ref();
		}

		// copy assignment operator:
		// returns T& by default, this may be wrong if called assignment operator has different return type
		T& operator =(const _ref_base& right)
		{
			static_assert(!std::is_const<T>::value, "vm::_ref_base<> error: operator= is not available for const reference");

			return get_ref() = right.get_ref();
		}

		template<typename CT, typename AT2> auto operator =(const _ref_base<CT, AT2>& right) const -> decltype(std::declval<T&>() = std::declval<CT>())
		{
			return get_ref() = right.get_ref();
		}

		template<typename CT> auto operator =(const CT& right) const -> decltype(std::declval<T&>() = std::declval<CT>())
		{
			return get_ref() = right;
		}
	};

	// Native endianness reference to LE data
	template<typename T, typename AT = u32> using refl = _ref_base<to_le_t<T>, AT>;

	// Native endianness reference to BE data
	template<typename T, typename AT = u32> using refb = _ref_base<to_be_t<T>, AT>;

	// BE reference to LE data
	template<typename T, typename AT = u32> using brefl = _ref_base<to_le_t<T>, to_be_t<AT>>;

	// BE reference to BE data
	template<typename T, typename AT = u32> using brefb = _ref_base<to_be_t<T>, to_be_t<AT>>;

	// LE reference to LE data
	template<typename T, typename AT = u32> using lrefl = _ref_base<to_le_t<T>, to_le_t<AT>>;

	// LE reference to BE data
	template<typename T, typename AT = u32> using lrefb = _ref_base<to_be_t<T>, to_le_t<AT>>;

	namespace ps3
	{
		// default reference for PS3 HLE functions (Native endianness reference to BE data)
		template<typename T, typename AT = u32> using ref = refb<T, AT>;

		// default reference for PS3 HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> using bref = brefb<T, AT>;
	}

	namespace psv
	{
		// default reference for PSV HLE functions (Native endianness reference to LE data)
		template<typename T> using ref = refl<T>;

		// default reference for PSV HLE structures (LE reference to LE data)
		template<typename T> using lref = lrefl<T>;
	}
}

// postfix increment operator for vm::_ref_base
template<typename T, typename AT> inline auto operator ++(const vm::_ref_base<T, AT>& ref, int) -> decltype(std::declval<T&>()++)
{
	return ref.get_ref()++;
}

// prefix increment operator for vm::_ref_base
template<typename T, typename AT> inline auto operator ++(const vm::_ref_base<T, AT>& ref) -> decltype(++std::declval<T&>())
{
	return ++ref.get_ref();
}

// postfix decrement operator for vm::_ref_base
template<typename T, typename AT> inline auto operator --(const vm::_ref_base<T, AT>& ref, int) -> decltype(std::declval<T&>()--)
{
	return ref.get_ref()--;
}

// prefix decrement operator for vm::_ref_base
template<typename T, typename AT> inline auto operator --(const vm::_ref_base<T, AT>& ref) -> decltype(--std::declval<T&>())
{
	return --ref.get_ref();
}

// addition assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator +=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() += std::declval<T2>())
{
	return ref.get_ref() += right;
}

// subtraction assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator -=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() -= std::declval<T2>())
{
	return ref.get_ref() -= right;
}

// multiplication assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator *=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() *= std::declval<T2>())
{
	return ref.get_ref() *= right;
}

// division assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator /=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() /= std::declval<T2>())
{
	return ref.get_ref() /= right;
}

// modulo assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator %=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() %= std::declval<T2>())
{
	return ref.get_ref() %= right;
}

// bitwise AND assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator &=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() &= std::declval<T2>())
{
	return ref.get_ref() &= right;
}

// bitwise OR assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator |=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() |= std::declval<T2>())
{
	return ref.get_ref() |= right;
}

// bitwise XOR assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator ^=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() ^= std::declval<T2>())
{
	return ref.get_ref() ^= right;
}

// bitwise left shift assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator <<=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() <<= std::declval<T2>())
{
	return ref.get_ref() <<= right;
}

// bitwise right shift assignment operator for vm::_ref_base
template<typename T, typename AT, typename T2> inline auto operator >>=(const vm::_ref_base<T, AT>& ref, const T2& right) -> decltype(std::declval<T&>() >>= std::declval<T2>())
{
	return ref.get_ref() >>= right;
}

// external specialization for is_be_t<> (true if AT's endianness is BE)

template<typename T, typename AT>
struct is_be_t<vm::_ref_base<T, AT>> : public std::integral_constant<bool, is_be_t<AT>::value>
{
};

// external specialization for is_le_t<> (true if AT's endianness is LE)

template<typename T, typename AT>
struct is_le_t<vm::_ref_base<T, AT>> : public std::integral_constant<bool, is_le_t<AT>::value>
{
};

// external specialization for to_ne_t<> (change AT's endianness to native)

template<typename T, typename AT>
struct to_ne<vm::_ref_base<T, AT>>
{
	using type = vm::_ref_base<T, to_ne_t<AT>>;
};

// external specialization for to_be_t<> (change AT's endianness to BE)

template<typename T, typename AT>
struct to_be<vm::_ref_base<T, AT>>
{
	using type = vm::_ref_base<T, to_be_t<AT>>;
};

// external specialization for to_le_t<> (change AT's endianness to LE)

template<typename T, typename AT>
struct to_le<vm::_ref_base<T, AT>>
{
	using type = vm::_ref_base<T, to_le_t<AT>>;
};

namespace fmt
{
	// external specialization for fmt::format function

	template<typename T, typename AT>
	struct unveil<vm::_ref_base<T, AT>, false>
	{
		using result_type = typename unveil<AT>::result_type;

		force_inline static result_type get_value(const vm::_ref_base<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum>
struct cast_ppu_gpr;

template<typename T, typename AT>
struct cast_ppu_gpr<vm::_ref_base<T, AT>, false>
{
	force_inline static u64 to_gpr(const vm::_ref_base<T, AT>& value)
	{
		return cast_ppu_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ref_base<T, AT> from_gpr(const u64 reg)
	{
		return vm::_ref_base<T, AT>::make(cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};

// external specializations for ARMv7 GPR

template<typename T, bool is_enum>
struct cast_armv7_gpr;

template<typename T, typename AT>
struct cast_armv7_gpr<vm::_ref_base<T, AT>, false>
{
	force_inline static u32 to_gpr(const vm::_ref_base<T, AT>& value)
	{
		return cast_armv7_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	force_inline static vm::_ref_base<T, AT> from_gpr(const u32 reg)
	{
		return vm::_ref_base<T, AT>::make(cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
