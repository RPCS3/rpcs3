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
	};

	//BE reference to LE data
	template<typename T, typename AT = u32> struct brefl : public _ref_base<T, typename to_be_t<AT>::type>
	{
		brefl& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
		const brefl& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
	};

	//BE reference to BE data
	template<typename T, typename AT = u32> struct brefb : public _ref_base<typename to_be_t<T>::type, typename to_be_t<AT>::type>
	{
		brefb& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
		const brefb& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
	};

	//LE reference to BE data
	template<typename T, typename AT = u32> struct lrefb : public _ref_base<typename to_be_t<T>::type, AT>
	{
		lrefb& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
		const lrefb& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
	};

	//LE reference to LE data
	template<typename T, typename AT = u32> struct lrefl : public _ref_base<T, AT>
	{
		lrefl& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
		const lrefl& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
	};

	namespace ps3
	{
		//default reference for HLE functions (LE reference to BE data)
		template<typename T, typename AT = u32> struct ref : public lrefb<T, AT>
		{
			ref& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
			const ref& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
		};

		//default reference for HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> struct bref : public brefb<T, AT>
		{
			bref& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
			const bref& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
		};
	}

	namespace psv
	{
		//default reference for HLE functions & structures (LE reference to LE data)
		template<typename T, typename AT = u32> struct ref : public lrefl<T, AT>
		{
			ref& operator = (T right) { get_ref<T>(m_addr) = right; return *this; }
			const ref& operator = (T right) const { get_ref<T>(m_addr) = right; return *this; }
		};
	}

	//PS3 emulation is main now, so lets it be as default
	using namespace ps3;
}