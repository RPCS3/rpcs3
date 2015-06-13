#pragma once

namespace vm
{
	template<typename T, typename AT = u32>
	struct _ref_base
	{
		AT m_addr;

		static_assert(!std::is_pointer<T>::value, "vm::_ref_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ref_base<> error: invalid type (reference)");

		AT addr() const
		{
			return m_addr;
		}

		template<typename AT2 = AT> static _ref_base make(const AT2& addr)
		{
			return{ convert_le_be<AT>(addr) };
		}

		T& get_ref() const
		{
			return vm::get_ref<T>(vm::cast(m_addr));
		}

		T& priv_ref() const
		{
			return vm::priv_ref<T>(vm::cast(m_addr));
		}

		// conversion operator
		//template<typename CT> operator std::enable_if_t<std::is_convertible<T, CT>::value, CT>()
		//{
		//	return get_ref();
		//}

		// temporarily, because SFINAE doesn't work for some reason:

		operator to_ne_t<T>() const
		{
			return get_ref();
		}

		operator T() const
		{
			return get_ref();
		}

		// copy assignment operator
		_ref_base& operator =(const _ref_base& right)
		{
			get_ref() = right.get_ref();
			return *this;
		}

		template<typename CT, typename AT2> std::enable_if_t<std::is_assignable<T&, CT>::value, const _ref_base&> operator =(const _ref_base<CT, AT2>& right) const
		{
			get_ref() = right.get_ref();
			return *this;
		}

		template<typename CT> std::enable_if_t<std::is_assignable<T&, CT>::value, const _ref_base&> operator =(const CT& right) const
		{
			get_ref() = right;
			return *this;
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
		template<typename T, typename AT = u32> using ref = refl<T, AT>;

		// default reference for PSV HLE structures (LE reference to LE data)
		template<typename T, typename AT = u32> using lref = lrefl<T, AT>;
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}

// external specialization for is_be_t<>

template<typename T, typename AT>
struct is_be_t<vm::_ref_base<T, AT>> : public std::integral_constant<bool, is_be_t<AT>::value>
{
};

// external specialization for to_ne_t<>

template<typename T, typename AT>
struct to_ne<vm::_ref_base<T, AT>>
{
	using type = vm::_ref_base<T, to_ne_t<AT>>;
};

// external specialization for to_be_t<>

template<typename T, typename AT>
struct to_be<vm::_ref_base<T, AT>>
{
	using type = vm::_ref_base<T, to_be_t<AT>>;
};

// external specialization for to_le_t<> (not used)

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
