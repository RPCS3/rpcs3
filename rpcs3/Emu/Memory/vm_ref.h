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

		static _ref_base make(AT addr)
		{
			return (_ref_base&)addr;
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
	template<typename T, typename AT = u32> struct brefl : public _ref_base<T, typename to_be_t<AT>::type>
	{
		using _ref_base<T, typename to_be_t<AT>::type>::operator=;
	};

	//BE reference to BE data
	template<typename T, typename AT = u32> struct brefb : public _ref_base<typename to_be_t<T>::type, typename to_be_t<AT>::type>
	{
		using _ref_base<typename to_be_t<T>::type, typename to_be_t<AT>::type>::operator=;
	};

	//LE reference to BE data
	template<typename T, typename AT = u32> struct lrefb : public _ref_base<typename to_be_t<T>::type, AT>
	{
		using _ref_base<typename to_be_t<T>::type, AT>::operator=;
	};

	//LE reference to LE data
	template<typename T, typename AT = u32> struct lrefl : public _ref_base<T, AT>
	{
		using _ref_base<T, AT>::operator=;
	};

	namespace ps3
	{
		//default reference for HLE functions (LE reference to BE data)
		template<typename T, typename AT = u32> struct ref : public lrefb<T, AT>
		{
			using lrefb<T, AT>::operator=;
		};

		//default reference for HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> struct bref : public brefb<T, AT>
		{
			using brefb<T, AT>::operator=;
		};
	}

	namespace psv
	{
		//default reference for HLE functions & structures (LE reference to LE data)
		template<typename T, typename AT = u32> struct ref : public lrefl<T, AT>
		{
			using lrefl<T, AT>::operator=;
		};
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}

namespace fmt
{
	// external specializations for fmt::format function

	template<typename T, typename AT>
	struct unveil<vm::ps3::ref<T, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::ps3::ref<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};

	template<typename T, typename AT>
	struct unveil<vm::ps3::bref<T, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::ps3::bref<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};

	template<typename T, typename AT>
	struct unveil<vm::psv::ref<T, AT>, false>
	{
		typedef typename unveil<AT>::result_type result_type;

		__forceinline static result_type get_value(const vm::psv::ref<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum>
struct cast_ppu_gpr;

template<typename T, typename AT>
struct cast_ppu_gpr<vm::ps3::ref<T, AT>, false>
{
	__forceinline static u64 to_gpr(const vm::ps3::ref<T, AT>& value)
	{
		return value.addr();
	}

	__forceinline static vm::ps3::ref<T, AT> from_gpr(const u64 reg)
	{
		return vm::ps3::ref<T, AT>::make(cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg));
	}
};
