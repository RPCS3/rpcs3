#pragma once

namespace vm
{
	template<typename T, typename AT = u32>
	class _ref_base
	{
		AT m_addr;

	public:
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

		bool check() const
		{
			return Memory.IsGoodAddr(m_addr, sizeof(T));
		}

		static ref make(u32 addr)
		{
			return (ref&)addr;
		}
	};

	//BE reference to LE data
	template<typename T, typename AT = u32> class brefl : public _ref_base<T, be_t<AT>> {};

	//BE reference to BE data
	template<typename T, typename AT = u32> class brefb : public _ref_base<be_t<T>, be_t<AT>> {};

	//LE reference to BE data
	template<typename T, typename AT = u32> class lrefb : public _ref_base<be_t<T>, AT> {};

	//LE reference to LE data
	template<typename T, typename AT = u32> class lrefl : public _ref_base<T, AT> {};

	namespace ps3
	{
		//default reference for HLE functions (LE reference to BE data)
		template<typename T, typename AT = u32> class ref : public lrefb {};

		//default reference for HLE structures (BE reference to BE data)
		template<typename T, typename AT = u32> class bref : public brefb {};
	}

	namespace psv
	{
		//default reference for HLE functions & structures (LE reference to LE data)
		template<typename T, typename AT = u32> class ref : public lrefl {};
	}
}