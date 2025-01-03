#pragma once

#include <type_traits>
#include "vm.h"

#include "util/to_endian.hpp"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

namespace vm
{
	template <typename T, typename AT>
	class _ptr_base;

	template <typename T, typename AT>
	class _ref_base
	{
		AT m_addr;

		static_assert(!std::is_pointer_v<T>, "vm::_ref_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference_v<T>, "vm::_ref_base<> error: invalid type (reference)");
		static_assert(!std::is_function_v<T>, "vm::_ref_base<> error: invalid type (function)");
		static_assert(!std::is_void_v<T>, "vm::_ref_base<> error: invalid type (void)");

	public:
		using type = T;
		using addr_type = std::remove_cv_t<AT>;

		_ref_base(const _ref_base&) = default;

		_ref_base(vm::addr_t addr)
			: m_addr(addr)
		{
		}

		addr_type addr() const
		{
			return m_addr;
		}

		T& get_ref() const
		{
			return *static_cast<T*>(vm::base(vm::cast(m_addr)));
		}

		// convert to vm pointer
		vm::_ptr_base<T, u32> ptr() const
		{
			return vm::cast(m_addr);
		}

		operator std::common_type_t<T>() const
		{
			return get_ref();
		}

		operator T&() const
		{
			return get_ref();
		}

		T& operator =(const _ref_base& right)
		{
			return get_ref() = right.get_ref();
		}

		T& operator =(const std::common_type_t<T>& right) const
		{
			return get_ref() = right;
		}

		decltype(auto) operator ++(int) const
		{
			return get_ref()++;
		}

		decltype(auto) operator ++() const
		{
			return ++get_ref();
		}

		decltype(auto) operator --(int) const
		{
			return get_ref()--;
		}

		decltype(auto) operator --() const
		{
			return --get_ref();
		}

		template<typename T2>
		decltype(auto) operator +=(const T2& right)
		{
			return get_ref() += right;
		}

		template<typename T2>
		decltype(auto) operator -=(const T2& right)
		{
			return get_ref() -= right;
		}

		template<typename T2>
		decltype(auto) operator *=(const T2& right)
		{
			return get_ref() *= right;
		}

		template<typename T2>
		decltype(auto) operator /=(const T2& right)
		{
			return get_ref() /= right;
		}

		template<typename T2>
		decltype(auto) operator %=(const T2& right)
		{
			return get_ref() %= right;
		}

		template<typename T2>
		decltype(auto) operator &=(const T2& right)
		{
			return get_ref() &= right;
		}

		template<typename T2>
		decltype(auto) operator |=(const T2& right)
		{
			return get_ref() |= right;
		}

		template<typename T2>
		decltype(auto) operator ^=(const T2& right)
		{
			return get_ref() ^= right;
		}

		template<typename T2>
		decltype(auto) operator <<=(const T2& right)
		{
			return get_ref() <<= right;
		}

		template<typename T2>
		decltype(auto) operator >>=(const T2& right)
		{
			return get_ref() >>= right;
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

	inline namespace ps3_
	{
		// default reference for PS3 HLE functions (Native endianness reference to BE data)
		template<typename T, typename AT = u32> using ref = refb<T, AT>;

		// default reference for PS3 HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> using bref = brefb<T, AT>;
	}
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

// Change AT endianness to BE/LE
template<typename T, typename AT, bool Se>
struct to_se<vm::_ref_base<T, AT>, Se>
{
	using type = vm::_ref_base<T, typename to_se<AT, Se>::type>;
};

// Forbid formatting
template <typename T, typename AT>
struct fmt_unveil<vm::_ref_base<T, AT>>
{
	static_assert(!sizeof(T), "vm::_ref_base<>: ambiguous format argument");
};
