#pragma once

namespace vm
{
	template<typename T, typename AT = u32>
	class _ref_base
	{
	protected:
		AT m_addr;

	public:
		typedef T type;
		static_assert(!std::is_pointer<T>::value, "vm::_ref_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ref_base<> error: invalid type (reference)");
		typedef typename remove_be_t<T>::type le_type;
		typedef typename to_be_t<T>::type be_type;

		operator T&()
		{
			return get_ref<T>(m_addr);
		}

		operator const T&() const
		{
			return get_ref<const T>(m_addr);
		}

		AT addr() const
		{
			return m_addr;
		}

		static _ref_base make(const AT& addr)
		{
			return reinterpret_cast<_ref_base&>(addr);
		}

		_ref_base& operator = (le_type right)
		{
			get_ref<T>(m_addr) = right;
			return *this;
		}

		const _ref_base& operator = (le_type right) const
		{
			get_ref<T>(m_addr) = right;
			return *this;
		}

		_ref_base& operator = (be_type right)
		{
			get_ref<T>(m_addr) = right;
			return *this;
		}

		const _ref_base& operator = (be_type right) const
		{
			get_ref<T>(m_addr) = right;
			return *this;
		}
	};

	//BE reference to LE data
	template<typename T, typename AT = u32> using brefl = _ref_base<T, typename to_be_t<AT>::type>;

	//BE reference to BE data
	template<typename T, typename AT = u32> using brefb = _ref_base<typename to_be_t<T>::type, typename to_be_t<AT>::type>;

	//LE reference to BE data
	template<typename T, typename AT = u32> using lrefb = _ref_base<typename to_be_t<T>::type, AT>;

	//LE reference to LE data
	template<typename T, typename AT = u32> using lrefl = _ref_base<T, AT>;

	namespace ps3
	{
		//default reference for HLE functions (LE reference to BE data)
		template<typename T, typename AT = u32> using ref = lrefb<T, AT>;

		//default reference for HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> using bref = brefb<T, AT>;
	}

	namespace psv
	{
		//default reference for HLE functions & structures (LE reference to LE data)
		template<typename T, typename AT = u32> using ref = lrefl<T, AT>;
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}

namespace fmt
{
	// external specialization for fmt::format function

	template<typename T, typename AT>
	struct unveil<vm::_ref_base<T, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::_ref_base<T, AT>& arg)
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
	__forceinline static u64 to_gpr(const vm::_ref_base<T, AT>& value)
	{
		return value.addr();
	}

	__forceinline static vm::_ref_base<T, AT> from_gpr(const u64 reg)
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
	__forceinline static u32 to_gpr(const vm::_ref_base<T, AT>& value)
	{
		return value.addr();
	}

	__forceinline static vm::_ref_base<T, AT> from_gpr(const u32 reg)
	{
		return vm::_ref_base<T, AT>::make(cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
