#pragma once

#include "vm_ref.h"

class PPUThread;
class ARMv7Thread;

namespace vm
{
	// SFINAE helper type for vm::_ptr_base comparison operators (enables comparison between equal types and between any type and void*)
	template<typename T1, typename T2, typename RT = void>
	using if_comparable_t = std::enable_if_t<std::is_void<T1>::value || std::is_void<T2>::value || std::is_same<std::remove_cv_t<T1>, std::remove_cv_t<T2>>::value, RT>;

	template<typename T, typename AT = u32>
	class _ptr_base
	{
		AT m_addr;

		static_assert(!std::is_pointer<T>::value, "vm::_ptr_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ptr_base<> error: invalid type (reference)");

	public:
		using type = T;
		using addr_type = std::remove_cv_t<AT>;

		_ptr_base() = default;

		constexpr _ptr_base(addr_type addr, const addr_tag_t&)
			: m_addr(addr)
		{
		}

		constexpr addr_type addr() const
		{
			return m_addr;
		}

		void set(addr_type addr)
		{
			this->m_addr = addr;
		}

		static constexpr _ptr_base make(addr_type addr)
		{
			return{ addr, vm::addr };
		}

		// Enable only the conversions which are originally possible between pointer types
		template<typename T2, typename AT2, typename = std::enable_if_t<std::is_convertible<T*, T2*>::value>>
		operator _ptr_base<T2, AT2>() const
		{
			return{ vm::cast(m_addr, HERE), vm::addr };
		}

		explicit constexpr operator bool() const
		{
			return m_addr != 0;
		}

		// Get vm pointer to a struct member
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>>
		_ptr_base<MT> ptr(MT T2::*const mptr) const
		{
			return{ vm::cast(m_addr, HERE) + get_offset(mptr), vm::addr };
		}

		// Get vm pointer to a struct member with array subscription
		template<typename MT, typename T2, typename ET = std::remove_extent_t<MT>, typename = if_comparable_t<T, T2>>
		_ptr_base<ET> ptr(MT T2::*const mptr, u32 index) const
		{
			return{ vm::cast(m_addr, HERE) + get_offset(mptr) + SIZE_32(ET) * index, vm::addr };
		}

		// Get vm reference to a struct member
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>>
		_ref_base<MT> ref(MT T2::*const mptr) const
		{
			return{ vm::cast(m_addr, HERE) + get_offset(mptr), vm::addr };
		}

		// Get vm reference to a struct member with array subscription
		template<typename MT, typename T2, typename ET = std::remove_extent_t<MT>, typename = if_comparable_t<T, T2>>
		_ref_base<ET> ref(MT T2::*const mptr, u32 index) const
		{
			return{ vm::cast(m_addr, HERE) + get_offset(mptr) + SIZE_32(ET) * index, vm::addr };
		}

		// Get vm reference
		_ref_base<T, u32> ref() const
		{
			return{ vm::cast(m_addr, HERE), vm::addr };
		}

		T* get_ptr() const
		{
			return static_cast<T*>(vm::base(vm::cast(m_addr, HERE)));
		}

		T* get_ptr_priv() const
		{
			return static_cast<T*>(vm::base_priv(vm::cast(m_addr, HERE)));
		}

		T* operator ->() const
		{
			return get_ptr();
		}

		std::add_lvalue_reference_t<T> operator *() const
		{
			return *static_cast<T*>(vm::base(vm::cast(m_addr, HERE)));
		}

		std::add_lvalue_reference_t<T> operator [](u32 index) const
		{
			return *static_cast<T*>(vm::base(vm::cast(m_addr, HERE) + SIZE_32(T) * index));
		}

		// Test address for arbitrary alignment: (addr & (align - 1)) == 0
		bool aligned(u32 align) const
		{
			return (m_addr & (align - 1)) == 0;
		}

		// Test address alignment using alignof(T)
		bool aligned() const
		{
			return aligned(ALIGN_32(T));
		}

		// Test address for arbitrary alignment: (addr & (align - 1)) != 0
		explicit_bool_t operator %(u32 align) const
		{
			return !aligned(align);
		}

		_ptr_base<T, u32> operator +() const
		{
			return{ vm::cast(m_addr, HERE), vm::addr };
		}

		_ptr_base<T, u32> operator +(u32 count) const
		{
			return{ vm::cast(m_addr, HERE) + count * SIZE_32(T), vm::addr };
		}

		_ptr_base<T, u32> operator -(u32 count) const
		{
			return{ vm::cast(m_addr, HERE) - count * SIZE_32(T), vm::addr };
		}

		friend _ptr_base<T, u32> operator +(u32 count, const _ptr_base& ptr)
		{
			return{ vm::cast(ptr.m_addr, HERE) + count * SIZE_32(T), vm::addr };
		}

		// Pointer difference operator
		template<typename T2, typename AT2>
		std::enable_if_t<std::is_object<T2>::value && std::is_same<CV T, CV T2>::value, s32> operator -(const _ptr_base<T2, AT2>& right) const
		{
			return static_cast<s32>(vm::cast(m_addr, HERE) - vm::cast(right.m_addr, HERE)) / SIZE_32(T);
		}

		_ptr_base operator ++(int)
		{
			const addr_type result = m_addr;
			m_addr = vm::cast(m_addr, HERE) + SIZE_32(T);
			return{ result, vm::addr };
		}

		_ptr_base& operator ++()
		{
			m_addr = vm::cast(m_addr, HERE) + SIZE_32(T);
			return *this;
		}

		_ptr_base operator --(int)
		{
			const addr_type result = m_addr;
			m_addr = vm::cast(m_addr, HERE) - SIZE_32(T);
			return{ result, vm::addr };
		}

		_ptr_base& operator --()
		{
			m_addr = vm::cast(m_addr, HERE) - SIZE_32(T);
			return *this;
		}

		_ptr_base& operator +=(s32 count)
		{
			m_addr = vm::cast(m_addr, HERE) + count * SIZE_32(T);
			return *this;
		}

		_ptr_base& operator -=(s32 count)
		{
			m_addr = vm::cast(m_addr, HERE) - count * SIZE_32(T);
			return *this;
		}
	};

	template<typename AT, typename RT, typename... T>
	class _ptr_base<RT(T...), AT>
	{
		AT m_addr;

	public:
		using addr_type = std::remove_cv_t<AT>;

		_ptr_base() = default;

		constexpr _ptr_base(addr_type addr, const addr_tag_t&)
			: m_addr(addr)
		{
		}

		constexpr addr_type addr() const
		{
			return m_addr;
		}

		void set(addr_type addr)
		{
			m_addr = addr;
		}

		static constexpr _ptr_base make(addr_type addr)
		{
			return{ addr, vm::addr };
		}

		// Conversion to another function pointer
		template<typename AT2>
		operator _ptr_base<RT(T...), AT2>() const
		{
			return{ vm::cast(m_addr, HERE), vm::addr };
		}

		explicit constexpr operator bool() const
		{
			return m_addr != 0;
		}

		_ptr_base<RT(T...), u32> operator +() const
		{
			return{ vm::cast(m_addr, HERE), vm::addr };
		}

		// Callback; defined in PPUCallback.h, passing context is mandatory
		RT operator()(PPUThread& ppu, T... args) const;

		// Callback; defined in ARMv7Callback.h, passing context is mandatory
		RT operator()(ARMv7Thread& cpu, T... args) const;
	};

	template<typename AT, typename RT, typename... T>
	class _ptr_base<RT(*)(T...), AT>
	{
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
		// Default pointer type for PS3 HLE functions (Native endianness pointer to BE data)
		template<typename T, typename AT = u32> using ptr = ptrb<T, AT>;

		// Default pointer to pointer type for PS3 HLE functions (Native endianness pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using pptr = ptr<ptr<T, AT2>, AT>;

		// Default pointer type for PS3 HLE structures (BE pointer to BE data)
		template<typename T, typename AT = u32> using bptr = bptrb<T, AT>;

		// Default pointer to pointer type for PS3 HLE structures (BE pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using bpptr = bptr<ptr<T, AT2>, AT>;

		// Native endianness pointer to const BE data
		template<typename T, typename AT = u32> using cptr = ptr<const T, AT>;

		// BE pointer to const BE data
		template<typename T, typename AT = u32> using bcptr = bptr<const T, AT>;

		template<typename T, typename AT = u32> using cpptr = pptr<const T, AT>;
		template<typename T, typename AT = u32> using bcpptr = bpptr<const T, AT>;

		// Perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(static_cast<to_be_t<CT>*>(std::declval<T*>()))>
		inline _ptr_base<to_be_t<CT>> static_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ vm::cast(other.addr(), HERE), vm::addr };
		}

		// Perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(const_cast<to_be_t<CT>*>(std::declval<T*>()))>
		inline _ptr_base<to_be_t<CT>> const_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ vm::cast(other.addr(), HERE), vm::addr };
		}
	}

	namespace psv
	{
		// Default pointer type for PSV HLE functions (Native endianness pointer to LE data)
		template<typename T> using ptr = ptrl<T>;

		// Default pointer to pointer type for PSV HLE functions (Native endianness pointer to LE pointer to LE data)
		template<typename T> using pptr = ptr<ptr<T>>;

		// Default pointer type for PSV HLE structures (LE pointer to LE data)
		template<typename T> using lptr = lptrl<T>;

		// Default pointer to pointer type for PSV HLE structures (LE pointer to LE pointer to LE data)
		template<typename T> using lpptr = lptr<ptr<T>>;

		// Native endianness pointer to const LE data
		template<typename T> using cptr = ptr<const T>;

		// LE pointer to const LE data
		template<typename T> using lcptr = lptr<const T>;

		template<typename T> using cpptr = pptr<const T>;
		template<typename T> using lcpptr = lpptr<const T>;

		// Perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(static_cast<to_le_t<CT>*>(std::declval<T*>()))>
		inline _ptr_base<to_le_t<CT>> static_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ vm::cast(other.addr(), HERE), vm::addr };
		}

		// Perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(const_cast<to_le_t<CT>*>(std::declval<T*>()))>
		inline _ptr_base<to_le_t<CT>> const_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ vm::cast(other.addr(), HERE), vm::addr };
		}
	}

	struct null_t
	{
		template<typename T, typename AT>
		constexpr operator _ptr_base<T, AT>() const
		{
			return _ptr_base<T, AT>{ 0, vm::addr };
		}

		template<typename T, typename AT>
		friend constexpr bool operator ==(const null_t&, const _ptr_base<T, AT>& ptr)
		{
			return !ptr;
		}

		template<typename T, typename AT>
		friend constexpr bool operator ==(const _ptr_base<T, AT>& ptr, const null_t&)
		{
			return !ptr;
		}

		template<typename T, typename AT>
		friend constexpr bool operator !=(const null_t&, const _ptr_base<T, AT>& ptr)
		{
			return ptr.operator bool();
		}

		template<typename T, typename AT>
		friend constexpr bool operator !=(const _ptr_base<T, AT>& ptr, const null_t&)
		{
			return ptr.operator bool();
		}

		template<typename T, typename AT>
		friend constexpr bool operator <(const null_t&, const _ptr_base<T, AT>& ptr)
		{
			return ptr.operator bool();
		}

		template<typename T, typename AT>
		friend constexpr bool operator <(const _ptr_base<T, AT>&, const null_t&)
		{
			return false;
		}
		
		template<typename T, typename AT>
		friend constexpr bool operator <=(const null_t&, const _ptr_base<T, AT>&)
		{
			return true;
		}

		template<typename T, typename AT>
		friend constexpr bool operator <=(const _ptr_base<T, AT>& ptr, const null_t&)
		{
			return !ptr.operator bool();
		}

		template<typename T, typename AT>
		friend constexpr bool operator >(const null_t&, const _ptr_base<T, AT>&)
		{
			return false;
		}

		template<typename T, typename AT>
		friend constexpr bool operator >(const _ptr_base<T, AT>& ptr, const null_t&)
		{
			return ptr.operator bool();
		}

		template<typename T, typename AT>
		friend constexpr bool operator >=(const null_t&, const _ptr_base<T, AT>& ptr)
		{
			return !ptr;
		}

		template<typename T, typename AT>
		friend constexpr bool operator >=(const _ptr_base<T, AT>&, const null_t&)
		{
			return true;
		}
	};

	// Null pointer convertible to any vm::ptr* type
	static null_t null;
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator ==(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() == right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator !=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() != right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator <(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() < right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator <=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() <= right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator >(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() > right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2>
inline vm::if_comparable_t<T1, T2, bool> operator >=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() >= right.addr();
}

// Change AT endianness to BE/LE
template<typename T, typename AT, bool Se>
struct to_se<vm::_ptr_base<T, AT>, Se>
{
	using type = vm::_ptr_base<T, typename to_se<AT, Se>::type>;
};

// Format pointer
template<typename T, typename AT>
struct unveil<vm::_ptr_base<T, AT>, void>
{
	static inline auto get(const vm::_ptr_base<T, AT>& arg)
	{
		return unveil<AT>::get(arg.addr());
	}
};
