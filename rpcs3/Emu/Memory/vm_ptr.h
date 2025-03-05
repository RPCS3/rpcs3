#pragma once

#include "util/types.hpp"
#include "util/to_endian.hpp"
#include "Utilities/StrFmt.h"
#include "vm.h"

class ppu_thread;
struct ppu_func_opd_t;

namespace vm
{
	template <typename T, typename AT>
	class _ref_base;

	// Enables comparison between comparable types of pointers
	template<typename T1, typename T2>
	concept PtrComparable = requires (T1* t1, T2* t2) { t1 == t2; };

	template <typename T, typename AT>
	class _ptr_base
	{
		AT m_addr;

		static_assert(!std::is_pointer_v<T>, "vm::_ptr_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference_v<T>, "vm::_ptr_base<> error: invalid type (reference)");

	public:
		using type = T;
		using addr_type = std::remove_cv_t<AT>;

		ENABLE_BITWISE_SERIALIZATION;

		_ptr_base() = default;

		_ptr_base(vm::addr_t addr) noexcept
			: m_addr(addr)
		{
		}

		addr_type addr() const
		{
			return m_addr;
		}

		void set(addr_type addr)
		{
			this->m_addr = addr;
		}

		static _ptr_base make(addr_type addr)
		{
			_ptr_base result;
			result.m_addr = addr;
			return result;
		}

		// Enable only the conversions which are originally possible between pointer types
		template <typename T2, typename AT2> requires (std::is_convertible_v<T*, T2*>)
		operator _ptr_base<T2, AT2>() const noexcept
		{
			return vm::cast(m_addr);
		}

		explicit operator bool() const noexcept
		{
			return m_addr != 0u;
		}

		// Get vm pointer to a struct member
		template <typename MT, typename T2> requires PtrComparable<T, T2>
		_ptr_base<MT, u32> ptr(MT T2::*const mptr) const
		{
			return vm::cast(vm::cast(m_addr) + offset32(mptr));
		}

		// Get vm pointer to a struct member with array subscription
		template <typename MT, typename T2, typename ET = std::remove_extent_t<MT>> requires PtrComparable<T, T2>
		_ptr_base<ET, u32> ptr(MT T2::*const mptr, u32 index) const
		{
			return vm::cast(vm::cast(m_addr) + offset32(mptr) + u32{sizeof(ET)} * index);
		}

		// Get vm reference to a struct member
		template <typename MT, typename T2> requires PtrComparable<T, T2> && (!std::is_void_v<T>)
		_ref_base<MT, u32> ref(MT T2::*const mptr) const
		{
			return vm::cast(vm::cast(m_addr) + offset32(mptr));
		}

		// Get vm reference to a struct member with array subscription
		template <typename MT, typename T2, typename ET = std::remove_extent_t<MT>> requires PtrComparable<T, T2> && (!std::is_void_v<T>)
		_ref_base<ET, u32> ref(MT T2::*const mptr, u32 index) const
		{
			return vm::cast(vm::cast(m_addr) + offset32(mptr) + u32{sizeof(ET)} * index);
		}

		// Get vm reference
		_ref_base<T, u32> ref() const requires (!std::is_void_v<T>)
		{
			return vm::cast(m_addr);
		}

		template <bool Strict = false>
		T* get_ptr() const
		{
			if constexpr (Strict)
			{
				AUDIT(m_addr);
			}

			return static_cast<T*>(vm::base(vm::cast(m_addr)));
		}

		T* operator ->() const requires (!std::is_void_v<T>)
		{
			return get_ptr<true>();
		}

		std::add_lvalue_reference_t<T> operator *() const requires (!std::is_void_v<T>)
		{
			return *get_ptr<true>();
		}

		std::add_lvalue_reference_t<T> operator [](u32 index) const requires (!std::is_void_v<T>)
		{
			AUDIT(m_addr);

			return *static_cast<T*>(vm::base(vm::cast(m_addr) + u32{sizeof(T)} * index));
		}

		// Test address for arbitrary alignment: (addr & (align - 1)) == 0
		bool aligned(u32 align = alignof(T)) const
		{
			return (m_addr & (align - 1)) == 0u;
		}

		// Get type size
		static constexpr u32 size() noexcept requires (!std::is_void_v<T>)
		{
			return sizeof(T);
		}

		// Get type alignment
		static constexpr u32 align() noexcept requires (!std::is_void_v<T>)
		{
			return alignof(T);
		}

		_ptr_base<T, u32> operator +() const
		{
			return vm::cast(m_addr);
		}

		_ptr_base<T, u32> operator +(u32 count) const requires (!std::is_void_v<T>)
		{
			return vm::cast(vm::cast(m_addr) + count * size());
		}

		_ptr_base<T, u32> operator -(u32 count) const requires (!std::is_void_v<T>)
		{
			return vm::cast(vm::cast(m_addr) - count * size());
		}

		friend _ptr_base<T, u32> operator +(u32 count, const _ptr_base& ptr) requires (!std::is_void_v<T>)
		{
			return vm::cast(vm::cast(ptr.m_addr) + count * size());
		}

		// Pointer difference operator
		template <typename T2, typename AT2> requires (std::is_object_v<T2> && std::is_same_v<std::decay_t<T>, std::decay_t<T2>>)
		s32 operator -(const _ptr_base<T2, AT2>& right) const
		{
			return static_cast<s32>(vm::cast(m_addr) - vm::cast(right.m_addr)) / size();
		}

		_ptr_base operator ++(int) requires (!std::is_void_v<T>)
		{
			_ptr_base result = *this;
			m_addr = vm::cast(m_addr) + size();
			return result;
		}

		_ptr_base& operator ++() requires (!std::is_void_v<T>)
		{
			m_addr = vm::cast(m_addr) + size();
			return *this;
		}

		_ptr_base operator --(int) requires (!std::is_void_v<T>)
		{
			_ptr_base result = *this;
			m_addr = vm::cast(m_addr) - size();
			return result;
		}

		_ptr_base& operator --() requires (!std::is_void_v<T>)
		{
			m_addr = vm::cast(m_addr) - size();
			return *this;
		}

		_ptr_base& operator +=(s32 count) requires (!std::is_void_v<T>)
		{
			m_addr = vm::cast(m_addr) + count * size();
			return *this;
		}

		_ptr_base& operator -=(s32 count) requires (!std::is_void_v<T>)
		{
			m_addr = vm::cast(m_addr) - count * size();
			return *this;
		}

		std::pair<bool, std::conditional_t<std::is_void_v<T>, char, std::remove_const_t<T>>> try_read() const requires (std::is_copy_constructible_v<T>)
		{
			alignas(sizeof(T) >= 16 ? 16 : 8) char buf[sizeof(T)]{};
			const bool ok = vm::try_access(vm::cast(m_addr), buf, sizeof(T), false);
			return { ok, std::bit_cast<decltype(try_read().second)>(buf) };
		}

		bool try_read(std::conditional_t<std::is_void_v<T>, char, std::remove_const_t<T>>& out) const requires (!std::is_void_v<T>)
		{
			return vm::try_access(vm::cast(m_addr), std::addressof(out), sizeof(T), false);
		}

		bool try_write(const std::conditional_t<std::is_void_v<T>, char, T>& _in) const requires (!std::is_void_v<T>)
		{
			return vm::try_access(vm::cast(m_addr), const_cast<T*>(std::addressof(_in)), sizeof(T), true);
		}

		// Don't use
		auto& raw()
		{
			return m_addr;
		}
	};

	template<typename AT, typename RT, typename... T>
	class _ptr_base<RT(T...), AT>
	{
		AT m_addr;

	public:
		using addr_type = std::remove_cv_t<AT>;
		ENABLE_BITWISE_SERIALIZATION;

		_ptr_base() = default;

		_ptr_base(vm::addr_t addr)
			: m_addr(addr)
		{
		}

		addr_type addr() const
		{
			return m_addr;
		}

		void set(addr_type addr)
		{
			m_addr = addr;
		}

		static _ptr_base make(addr_type addr)
		{
			_ptr_base result;
			result.m_addr = addr;
			return result;
		}

		// Conversion to another function pointer
		template<typename AT2>
		operator _ptr_base<RT(T...), AT2>() const
		{
			return vm::cast(m_addr);
		}

		explicit operator bool() const
		{
			return m_addr != 0u;
		}

		_ptr_base<RT(T...), u32> operator +() const
		{
			return vm::cast(m_addr);
		}

		// Don't use
		auto& raw()
		{
			return m_addr;
		}

		// Callback; defined in PPUCallback.h, passing context is mandatory
		RT operator()(ppu_thread& ppu, T... args) const;
		const ppu_func_opd_t& opd() const;
	};

	template<typename AT, typename RT, typename... T>
	class _ptr_base<RT(*)(T...), AT>
	{
		static_assert(!sizeof(AT), "vm::_ptr_base<> error: use RT(T...) format for functions instead of RT(*)(T...)");
	};

	// Native endianness pointer to LE data
	template<typename T, typename AT = u32> using ptrl = _ptr_base<to_le_t<T>, AT>;
	template<typename T, typename AT = u32> using cptrl = ptrl<const T, AT>;

	// Native endianness pointer to BE data
	template<typename T, typename AT = u32> using ptrb = _ptr_base<to_be_t<T>, AT>;
	template<typename T, typename AT = u32> using cptrb = ptrb<const T, AT>;

	// BE pointer to LE data
	template<typename T, typename AT = u32> using bptrl = _ptr_base<to_le_t<T>, to_be_t<AT>>;

	// BE pointer to BE data
	template<typename T, typename AT = u32> using bptrb = _ptr_base<to_be_t<T>, to_be_t<AT>>;

	// LE pointer to LE data
	template<typename T, typename AT = u32> using lptrl = _ptr_base<to_le_t<T>, to_le_t<AT>>;

	// LE pointer to BE data
	template<typename T, typename AT = u32> using lptrb = _ptr_base<to_be_t<T>, to_le_t<AT>>;

	inline namespace ps3_
	{
		// Default pointer type for PS3 HLE functions (Native endianness pointer to BE data)
		template<typename T, typename AT = u32> using ptr = ptrb<T, AT>;
		template<typename T, typename AT = u32> using cptr = ptr<const T, AT>;

		// Default pointer to pointer type for PS3 HLE functions (Native endianness pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using pptr = ptr<ptr<T, AT2>, AT>;
		template<typename T, typename AT = u32, typename AT2 = u32> using cpptr = pptr<const T, AT, AT2>;

		// Default pointer type for PS3 HLE structures (BE pointer to BE data)
		template<typename T, typename AT = u32> using bptr = bptrb<T, AT>;
		template<typename T, typename AT = u32> using bcptr = bptr<const T, AT>;

		// Default pointer to pointer type for PS3 HLE structures (BE pointer to BE pointer to BE data)
		template<typename T, typename AT = u32, typename AT2 = u32> using bpptr = bptr<ptr<T, AT2>, AT>;
		template<typename T, typename AT = u32, typename AT2 = u32> using bcpptr = bpptr<const T, AT, AT2>;

		// Perform static_cast (for example, vm::ptr<void> to vm::ptr<char>)
		template <typename CT, typename T, typename AT>
			requires requires(T* t) { static_cast<to_be_t<CT>*>(t); }
		inline _ptr_base<to_be_t<CT>, u32> static_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return vm::cast(other.addr());
		}

		// Perform const_cast (for example, vm::cptr<char> to vm::ptr<char>)
		template <typename CT, typename T, typename AT>
			requires requires(T* t) { const_cast<to_be_t<CT>*>(t); }
		inline _ptr_base<to_be_t<CT>, u32> const_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return vm::cast(other.addr());
		}

		// Perform reinterpret cast
		template <typename CT, typename T, typename AT>
			requires requires(T* t) { reinterpret_cast<to_be_t<CT>*>(t); }
		inline _ptr_base<to_be_t<CT>, u32> unsafe_ptr_cast(const _ptr_base<T, AT>& other)
		{
			return vm::cast(other.addr());
		}
	}

	struct null_t
	{
		template<typename T, typename AT>
		operator _ptr_base<T, AT>() const
		{
			return _ptr_base<T, AT>{};
		}

		template<typename T, typename AT>
		constexpr bool operator ==(const _ptr_base<T, AT>& ptr) const
		{
			return !ptr;
		}

		template<typename T, typename AT>
		constexpr bool operator <(const _ptr_base<T, AT>& ptr) const
		{
			return 0 < ptr.addr();
		}
	};

	// Null pointer convertible to any vm::ptr* type
	constexpr null_t null{};
}

template<typename T1, typename AT1, typename T2, typename AT2> requires vm::PtrComparable<T1, T2>
inline bool operator ==(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() == right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2> requires vm::PtrComparable<T1, T2>
inline bool operator <(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() < right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2> requires vm::PtrComparable<T1, T2>
inline bool operator <=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() <= right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2> requires vm::PtrComparable<T1, T2>
inline bool operator >(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
{
	return left.addr() > right.addr();
}

template<typename T1, typename AT1, typename T2, typename AT2> requires vm::PtrComparable<T1, T2>
inline bool operator >=(const vm::_ptr_base<T1, AT1>& left, const vm::_ptr_base<T2, AT2>& right)
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
template <typename T, typename AT>
struct fmt_unveil<vm::_ptr_base<T, AT>>
{
	using type = vm::_ptr_base<T, u32>; // Use only T, ignoring AT

	static inline auto get(const vm::_ptr_base<T, AT>& arg)
	{
		return fmt_unveil<AT>::get(arg.addr());
	}
};

template <>
struct fmt_class_string<vm::_ptr_base<const void, u32>>
{
	static void format(std::string& out, u64 arg);
};

template <typename T>
struct fmt_class_string<vm::_ptr_base<T, u32>> : fmt_class_string<vm::_ptr_base<const void, u32>>
{
	// Classify all pointers as const void*
};

template <>
struct fmt_class_string<vm::_ptr_base<const char, u32>>
{
	static void format(std::string& out, u64 arg);
};

template <>
struct fmt_class_string<vm::_ptr_base<char, u32>> : fmt_class_string<vm::_ptr_base<const char, u32>>
{
	// Classify char* as const char*
};

template <usz Size>
struct fmt_class_string<vm::_ptr_base<const char[Size], u32>> : fmt_class_string<vm::_ptr_base<const char, u32>>
{
	// Classify const char[] as const char*
};

template <usz Size>
struct fmt_class_string<vm::_ptr_base<char[Size], u32>> : fmt_class_string<vm::_ptr_base<const char, u32>>
{
	// Classify char[] as const char*
};
