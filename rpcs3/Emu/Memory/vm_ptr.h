#pragma once

#include "vm_ref.h"

class PPUThread;
class ARMv7Thread;

namespace vm
{
	// helper SFINAE type for vm::_ptr_base comparison operators (enables comparison between equal types and between any type and void*)
	template<typename T1, typename T2, typename RT = void> using if_comparable_t = std::enable_if_t<
		std::is_void<T1>::value ||
		std::is_void<T2>::value ||
		std::is_same<std::remove_cv_t<T1>, std::remove_cv_t<T2>>::value,
		RT>;

	template<typename T, typename AT = u32> class _ptr_base
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

		// get vm pointer to a struct member
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ptr_base<MT> ptr(MT T2::*const mptr) const
		{
			return{ VM_CAST(m_addr) + get_offset(mptr), vm::addr };
		}

		// get vm pointer to a struct member with array subscription
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ptr_base<std::remove_extent_t<MT>> ptr(MT T2::*const mptr, u32 index) const
		{
			return{ VM_CAST(m_addr) + get_offset(mptr) + sizeof32(T) * index, vm::addr };
		}

		// get vm reference to a struct member
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ref_base<MT> ref(MT T2::*const mptr) const
		{
			return{ VM_CAST(m_addr) + get_offset(mptr), vm::addr };
		}

		// get vm reference to a struct member with array subscription
		template<typename MT, typename T2, typename = if_comparable_t<T, T2>> _ref_base<std::remove_extent_t<MT>> ref(MT T2::*const mptr, u32 index) const
		{
			return{ VM_CAST(m_addr) + get_offset(mptr) + sizeof32(T) * index, vm::addr };
		}

		// get vm reference
		_ref_base<T, u32> ref() const
		{
			return{ VM_CAST(m_addr), vm::addr };
		}
		
		/*[[deprecated("Use constructor instead")]]*/ void set(addr_type value)
		{
			m_addr = value;
		}

		/*[[deprecated("Use constructor instead")]]*/ static _ptr_base make(addr_type addr)
		{
			return{ addr, vm::addr };
		}

		T* get_ptr() const
		{
			return static_cast<T*>(vm::base(VM_CAST(m_addr)));
		}

		T* get_ptr_priv() const
		{
			return static_cast<T*>(vm::base_priv(VM_CAST(m_addr)));
		}

		T* operator ->() const
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator-> is not available for void pointers");

			return get_ptr();
		}

		std::add_lvalue_reference_t<T> operator [](u32 index) const
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator[] is not available for void pointers");

			return *static_cast<T*>(vm::base(VM_CAST(m_addr) + sizeof32(T) * index));
		}

		// enable only the conversions which are originally possible between pointer types
		template<typename T2, typename AT2, typename = std::enable_if_t<std::is_convertible<T*, T2*>::value>> operator _ptr_base<T2, AT2>() const
		{
			return{ VM_CAST(m_addr), vm::addr };
		}

		//template<typename T2, typename = std::enable_if_t<std::is_convertible<T*, T2*>::value>> explicit operator T2*() const
		//{
		//	return get_ptr();
		//}

		explicit operator bool() const
		{
			return m_addr != 0;
		}

		// Test address for arbitrary alignment: (addr & (align - 1)) != 0
		bool aligned(u32 align) const
		{
			return (m_addr & (align - 1)) == 0;
		}

		// Test address for type's alignment using alignof(T)
		bool aligned() const
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: aligned() is not available for void pointers");

			return aligned(alignof32(T));
		}

		// Call aligned(value)
		explicit_bool_t operator %(u32 align) const
		{
			return aligned(align);
		}

		// pointer increment (postfix)
		_ptr_base operator ++(int)
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator++ is not available for void pointers");

			const addr_type result = m_addr;
			m_addr = VM_CAST(m_addr) + sizeof32(T);
			return{ result, vm::addr };
		}

		// pointer increment (prefix)
		_ptr_base& operator ++()
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator++ is not available for void pointers");

			m_addr = VM_CAST(m_addr) + sizeof32(T);
			return *this;
		}

		// pointer decrement (postfix)
		_ptr_base operator --(int)
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator-- is not available for void pointers");

			const addr_type result = m_addr;
			m_addr = VM_CAST(m_addr) - sizeof32(T);
			return{ result, vm::addr };
		}

		// pointer decrement (prefix)
		_ptr_base& operator --()
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator-- is not available for void pointers");

			m_addr = VM_CAST(m_addr) - sizeof32(T);
			return *this;
		}

		_ptr_base& operator +=(s32 count)
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator+= is not available for void pointers");

			m_addr = VM_CAST(m_addr) + count * sizeof32(T);
			return *this;
		}

		_ptr_base& operator -=(s32 count)
		{
			static_assert(!std::is_void<T>::value, "vm::_ptr_base<> error: operator-= is not available for void pointers");

			m_addr = VM_CAST(m_addr) - count * sizeof32(T);
			return *this;
		}
	};

	template<typename AT, typename RT, typename... T> class _ptr_base<RT(T...), AT>
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

		/*[[deprecated("Use constructor instead")]]*/ void set(addr_type value)
		{
			m_addr = value;
		}

		/*[[deprecated("Use constructor instead")]]*/ static _ptr_base make(addr_type addr)
		{
			return{ addr, vm::addr };
		}

		// defined in CB_FUNC.h, passing context is mandatory
		RT operator()(PPUThread& CPU, T... args) const;

		// defined in ARMv7Callback.h, passing context is mandatory
		RT operator()(ARMv7Thread& context, T... args) const;

		// conversion to another function pointer
		template<typename AT2> operator _ptr_base<RT(T...), AT2>() const
		{
			return{ VM_CAST(m_addr), vm::addr };
		}

		explicit operator bool() const
		{
			return m_addr != 0;
		}
	};

	template<typename AT, typename RT, typename... T> class _ptr_base<RT(*)(T...), AT>
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

		// perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(static_cast<to_be_t<CT>*>(std::declval<T*>()))> inline _ptr_base<to_be_t<CT>> static_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ VM_CAST(other.addr()), vm::addr };
		}

		// perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(const_cast<to_be_t<CT>*>(std::declval<T*>()))> inline _ptr_base<to_be_t<CT>> const_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ VM_CAST(other.addr()), vm::addr };
		}
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

		// perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(static_cast<to_le_t<CT>*>(std::declval<T*>()))> inline _ptr_base<to_le_t<CT>> static_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ VM_CAST(other.addr()), vm::addr };
		}

		// perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
		template<typename CT, typename T, typename AT, typename = decltype(const_cast<to_le_t<CT>*>(std::declval<T*>()))> inline _ptr_base<to_le_t<CT>> const_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return{ VM_CAST(other.addr()), vm::addr };
		}
	}

	struct null_t
	{
		template<typename T, typename AT> operator _ptr_base<T, AT>() const
		{
			return{ 0, vm::addr };
		}
	};

	// vm::null is convertible to any vm::ptr type as null pointer in virtual memory
	static null_t null;

	// Call wait_op() for specified vm pointer
	template<typename T, typename AT, typename F, typename... Args> inline auto wait_op(named_thread_t& thread, const _ptr_base<T, AT>& ptr, F pred, Args&&... args) -> decltype(static_cast<void>(pred(args...)))
	{
		return wait_op(thread, ptr.addr(), sizeof32(T), std::move(pred), std::forward<Args>(args)...);
	}

	// Call notify_at() for specified vm pointer
	template<typename T, typename AT> inline void notify_at(const vm::_ptr_base<T, AT>& ptr)
	{
		return notify_at(ptr.addr(), sizeof32(T));
	}
}

// unary plus operator for vm::_ptr_base (always available)
template<typename T, typename AT> inline vm::_ptr_base<T> operator +(const vm::_ptr_base<T, AT>& ptr)
{
	return ptr;
}

// indirection operator for vm::_ptr_base
template<typename T, typename AT> inline std::enable_if_t<std::is_object<T>::value, T&> operator *(const vm::_ptr_base<T, AT>& ptr)
{
	return *ptr.get_ptr();
}

// addition operator for vm::_ptr_base (pointer + integer)
template<typename T, typename AT> inline std::enable_if_t<std::is_object<T>::value, vm::_ptr_base<T>> operator +(const vm::_ptr_base<T, AT>& ptr, u32 count)
{
	return{ VM_CAST(ptr.addr()) + count * sizeof32(T), vm::addr };
}

// addition operator for vm::_ptr_base (integer + pointer)
template<typename T, typename AT> inline std::enable_if_t<std::is_object<T>::value, vm::_ptr_base<T>> operator +(u32 count, const vm::_ptr_base<T, AT>& ptr)
{
	return{ VM_CAST(ptr.addr()) + count * sizeof32(T), vm::addr };
}

// subtraction operator for vm::_ptr_base (pointer - integer)
template<typename T, typename AT> inline std::enable_if_t<std::is_object<T>::value, vm::_ptr_base<T>> operator -(const vm::_ptr_base<T, AT>& ptr, u32 count)
{
	return{ VM_CAST(ptr.addr()) - count * sizeof32(T), vm::addr };
}

// pointer difference operator for vm::_ptr_base
template<typename T1, typename AT1, typename T2, typename AT2> inline std::enable_if_t<
	std::is_object<T1>::value &&
	std::is_object<T2>::value &&
	std::is_same<std::remove_cv_t<T1>, std::remove_cv_t<T2>>::value,
	s32> operator -(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return static_cast<s32>(VM_CAST(left.addr()) - VM_CAST(right.addr())) / sizeof32(T1);
}

// comparison operator for vm::_ptr_base (pointer1 == pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator ==(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() == right.addr();
}

template<typename T, typename AT> inline bool operator ==(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return !ptr.operator bool();
}

template<typename T, typename AT> inline bool operator ==(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return !ptr.operator bool();
}

// comparison operator for vm::_ptr_base (pointer1 != pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator !=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() != right.addr();
}

template<typename T, typename AT> inline bool operator !=(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.operator bool();
}

template<typename T, typename AT> inline bool operator !=(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.operator bool();
}

// comparison operator for vm::_ptr_base (pointer1 < pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator <(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() < right.addr();
}

template<typename T, typename AT> inline bool operator <(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return ptr.operator bool();
}

template<typename T, typename AT> inline bool operator <(const vm::_ptr_base<T, AT>&, const vm::null_t&)
{
	return false;
}

// comparison operator for vm::_ptr_base (pointer1 <= pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator <=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() <= right.addr();
}

template<typename T, typename AT> inline bool operator <=(const vm::null_t&, const vm::_ptr_base<T, AT>&)
{
	return true;
}

template<typename T, typename AT> inline bool operator <=(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return !ptr.operator bool();
}

// comparison operator for vm::_ptr_base (pointer1 > pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator >(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() > right.addr();
}

template<typename T, typename AT> inline bool operator >(const vm::null_t&, const vm::_ptr_base<T, AT>&)
{
	return false;
}

template<typename T, typename AT> inline bool operator >(const vm::_ptr_base<T, AT>& ptr, const vm::null_t&)
{
	return ptr.operator bool();
}

// comparison operator for vm::_ptr_base (pointer1 >= pointer2)
template<typename T1, typename AT1, typename T2, typename AT2> inline vm::if_comparable_t<T1, T2, bool> operator >=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() >= right.addr();
}

template<typename T, typename AT> inline bool operator >=(const vm::null_t&, const vm::_ptr_base<T, AT>& ptr)
{
	return !ptr.operator bool();
}

template<typename T, typename AT> inline bool operator >=(const vm::_ptr_base<T, AT>&, const vm::null_t&)
{
	return true;
}

// external specialization for to_se<> (change AT endianness to BE/LE)

template<typename T, typename AT, bool Se> struct to_se<vm::_ptr_base<T, AT>, Se>
{
	using type = vm::_ptr_base<T, typename to_se<AT, Se>::type>;
};

// external specialization for to_ne<> (change AT endianness to native)

template<typename T, typename AT> struct to_ne<vm::_ptr_base<T, AT>>
{
	using type = vm::_ptr_base<T, typename to_ne<AT>::type>;
};

namespace fmt
{
	// external specialization for fmt::format function

	template<typename T, typename AT> struct unveil<vm::_ptr_base<T, AT>, false>
	{
		using result_type = typename unveil<AT>::result_type;

		static inline result_type get_value(const vm::_ptr_base<T, AT>& arg)
		{
			return unveil<AT>::get_value(arg.addr());
		}
	};
}

// external specializations for PPU GPR (SC_FUNC.h, CB_FUNC.h)

template<typename T, bool is_enum> struct cast_ppu_gpr;

template<typename T, typename AT> struct cast_ppu_gpr<vm::_ptr_base<T, AT>, false>
{
	static inline u64 to_gpr(const vm::_ptr_base<T, AT>& value)
	{
		return cast_ppu_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	static inline vm::_ptr_base<T, AT> from_gpr(const u64 reg)
	{
		return{ cast_ppu_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg), vm::addr };
	}
};

// external specializations for ARMv7 GPR

template<typename T, bool is_enum> struct cast_armv7_gpr;

template<typename T, typename AT> struct cast_armv7_gpr<vm::_ptr_base<T, AT>, false>
{
	static inline u32 to_gpr(const vm::_ptr_base<T, AT>& value)
	{
		return cast_armv7_gpr<AT, std::is_enum<AT>::value>::to_gpr(value.addr());
	}

	static inline vm::_ptr_base<T, AT> from_gpr(const u32 reg)
	{
		return{ cast_armv7_gpr<AT, std::is_enum<AT>::value>::from_gpr(reg), vm::addr };
	}
};
