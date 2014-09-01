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
		using _ref_base::operator=;
	};

	//BE reference to BE data
	template<typename T, typename AT = u32> struct brefb : public _ref_base<typename to_be_t<T>::type, typename to_be_t<AT>::type>
	{
		using _ref_base::operator=;
	};

	//LE reference to BE data
	template<typename T, typename AT = u32> struct lrefb : public _ref_base<typename to_be_t<T>::type, AT>
	{
		using _ref_base::operator=;
	};

	//LE reference to LE data
	template<typename T, typename AT = u32> struct lrefl : public _ref_base<T, AT>
	{
		using _ref_base::operator=;
	};

	namespace ps3
	{
		//default reference for HLE functions (LE reference to BE data)
		template<typename T, typename AT = u32> struct ref : public lrefb<T, AT>
		{
			using _ref_base::operator=;
		};

		//default reference for HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> struct bref : public brefb<T, AT>
		{
			using _ref_base::operator=;
		};
	}

	namespace psv
	{
		//default reference for HLE functions & structures (LE reference to LE data)
		template<typename T, typename AT = u32> struct ref : public lrefl<T, AT>
		{
			using _ref_base::operator=;
		};
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}