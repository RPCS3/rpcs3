#pragma once

#include <type_traits>
#include "vm.h"
#include "vm_ptr.h"

#include "util/to_endian.hpp"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

namespace vm
{
	template <typename T>
	concept Ref = requires (T& ref)
	{
		ref.addr();
		ref.aligned();
		ref.rebind(vm::addr_t{});
	};

	template <typename T, typename AT>
	class _ref_base_base
	{
	protected:
		AT m_addr;

		static_assert(!std::is_pointer<T>::value, "vm::_ref_base<> error: invalid type (pointer)");
		static_assert(!std::is_reference<T>::value, "vm::_ref_base<> error: invalid type (reference)");
		static_assert(!std::is_function<T>::value, "vm::_ref_base<> error: invalid type (function)");
		static_assert(!std::is_void<T>::value, "vm::_ref_base<> error: invalid type (void)");

	public:
		using type = T;
		using addr_type = std::remove_cv_t<AT>;

		_ref_base_base(const _ref_base_base&) = default;

		_ref_base_base(vm::addr_t addr)
			: m_addr(addr)
		{
		}

		template <typename T2, typename AT2> requires (std::is_constructible_v<const volatile T*, const volatile T2*>)
		_ref_base_base(const _ref_base_base<T2, AT2>& other)
			: m_addr(vm::cast(other))
		{
		}

		_ref_base_base& operator =(const _ref_base_base&) = delete;

		template <typename T2, typename AT2>
		_ref_base_base& operator =(const _ref_base_base<T2, AT2>&) = delete; // TODO: verify

		addr_type addr() const
		{
			return m_addr;
		}

		// Test address for arbitrary alignment: (addr & (align - 1)) == 0
		bool aligned(u32 align = alignof(T)) const
		{
			return (m_addr & (align - 1)) == 0u;
		}

		// Get type size
		static constexpr u32 size()
		{
			return sizeof(T);
		}

		// Get type alignment
		static constexpr u32 align()
		{
			return alignof(T);
		}

		// Overwrite reference
		void rebind(vm::addr_t addr)
		{
			m_addr = addr;
		}

		template <typename T2, typename AT2> requires (std::is_assignable_v<const volatile T*, const volatile T2*>)
		void rebind(const _ref_base_base<T2, AT2>& rhs)
		{
			m_addr = vm::cast(rhs.addr());
		}
	};

	template <typename T, typename AT>
	class _ref_base : public _ref_base_base<T, AT>
	{
	public:
		using _ref_base_base<T, AT>::_ref_base_base;

		_ref_base& operator =(const _ref_base&) = delete;

		std::common_type_t<T> operator =(const std::common_type_t<T>& right) const requires (!std::is_const_v<T>)
		{
			write_to_ptr<T>(vm::g_base_addr, vm::cast(this->m_addr), right);
			return right;
		}

		void assign(const std::common_type_t<T>& value) const requires (!std::is_const_v<T>)
		{
			write_to_ptr<T>(vm::g_base_addr, vm::cast(this->m_addr), value);
		}

		operator std::common_type_t<T>() const
		{
			return read_from_ptr<T>(vm::g_base_addr, vm::cast(this->m_addr));
		}

		std::common_type_t<T> value() const
		{
			return read_from_ptr<T>(vm::g_base_addr, vm::cast(this->m_addr));
		}

		SAFE_BUFFERS(std::pair<bool, std::decay_t<T>>) try_read() const
		{
			alignas(T) std::byte buf[sizeof(T)]{};
			const bool ok = vm::try_access(vm::cast(this->m_addr), buf, sizeof(T), false);
			return {ok, std::bit_cast<std::decay_t>(buf)};
		}

		bool try_read(std::decay_t<T>& out) const
		{
			return vm::try_access(vm::cast(this->m_addr), std::addressof(out), sizeof(T), false);
		}

		bool try_write(const std::common_type_t<T>& _in) const requires (!std::is_const_v<T>)
		{
			T value = _in;
			return vm::try_access(vm::cast(this->m_addr), std::addressof(value), sizeof(T), true);
		}

		bool try_write(std::decay_t<T>& value) const requires (!std::is_const_v<T>)
		{
			return vm::try_access(vm::cast(this->m_addr), std::addressof(value), sizeof(T), true);
		}

		// Get vm reference to a struct member
		template <typename MT, typename T2> requires PtrComparable<T, T2>
		_ref_base<stx::copy_cv_t<T, MT>, u32> ref(MT T2::*const mptr) const
		{
			return vm::cast(vm::cast(this->m_addr) + offset32(mptr));
		}

		// Get vm reference to a struct member with array subscription
		template <typename MT, typename T2, typename ET = std::remove_extent_t<MT>> requires PtrComparable<T, T2>
		_ref_base<stx::copy_cv_t<T, ET>, u32> ref(MT T2::*const mptr, u32 index) const
		{
			return vm::cast(vm::cast(this->m_addr) + offset32(mptr) + u32{sizeof(ET)} * index);
		}

		auto operator ++(int) const requires (!std::is_const_v<T>)
		{
			auto x = this->value();
			auto r = x++;
			*this = x;
			return r;
		}

		auto operator ++() const requires (!std::is_const_v<T>)
		{
			auto x = ++this->value();
			*this = x;
			return x;
		}

		auto operator --(int) const requires (!std::is_const_v<T>)
		{
			auto x = this->value();
			auto r = x--;
			*this = x;
			return r;
		}

		auto operator --() const requires (!std::is_const_v<T>)
		{
			auto x = --this->value();
			*this = x;
			return x;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator +=(const T2& right) const
		{
			auto x = this->value();
			auto r = x += right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator -=(const T2& right) const
		{
			auto x = this->value();
			auto r = x -= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator *=(const T2& right) const
		{
			auto x = this->value();
			auto r = x *= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator /=(const T2& right) const
		{
			auto x = this->value();
			auto r = x /= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator %=(const T2& right) const
		{
			auto x = this->value();
			auto r = x %= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator &=(const T2& right) const
		{
			auto x = this->value();
			auto r = x &= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator |=(const T2& right) const
		{
			auto x = this->value();
			auto r = x |= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator ^=(const T2& right) const
		{
			auto x = this->value();
			auto r = x ^= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator <<=(const T2& right) const
		{
			auto x = this->value();
			auto r = x <<= right;
			*this = x;
			return r;
		}

		template <typename T2> requires (!std::is_const_v<T>)
		auto operator >>=(const T2& right) const
		{
			auto x = this->value();
			auto r = x >>= right;
			*this = x;
			return r;
		}
	};

	template <typename T, typename AT>
	class _ref_base<T[], AT> : public _ref_base_base<T[], AT>
	{
	public:
		using _ref_base_base<T, AT>::_ref_base_base;

		_ref_base& operator =(const _ref_base&) = delete;

		_ref_base<T, AT> operator [](u32 index) const
		{
			return _ref_base<T, AT>(vm::cast(this->m_addr + sizeof(T) * index));
		}
	};

	template <typename T, typename AT, std::size_t N>
	class _ref_base<T[N], AT> : public _ref_base_base<T[N], AT>
	{
	public:
		using _ref_base_base<T, AT>::_ref_base_base;

		_ref_base& operator =(const _ref_base&) = delete;

		_ref_base<T, AT> operator [](u32 index) const
		{
			return _ref_base<T, AT>(vm::cast(this->m_addr + sizeof(T) * index));
		}
	};

	template <typename T, typename AT, std::size_t Align>
	class _ref_base<atomic_t<T, Align>, AT> : public _ref_base_base<atomic_t<T, Align>, AT>
	{
		// See atomic.hpp atomic_t
		using type = typename std::remove_cv<T>::type;

		type& raw() const
		{
			return reinterpret_cast<type&>(g_base_addr[vm::cast(this->m_addr)]);
		}

		// Disable overaligned atomics
		static_assert(Align == sizeof(T));

	public:
		using _ref_base_base<atomic_t<T, Align>, AT>::_ref_base_base;

		_ref_base& operator =(const _ref_base&) = delete;

		// See atomic.hpp atomic_t
		type compare_and_swap(const type& cmp, const type& exch) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			type old = cmp;
			atomic_storage<type>::compare_exchange(raw(), old, exch);
			return old;
		}

		// See atomic.hpp atomic_t
		bool compare_and_swap_test(const type& cmp, const type& exch) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			type old = cmp;
			return atomic_storage<type>::compare_exchange(raw(), old, exch);
		}

		// See atomic.hpp atomic_t
		bool compare_exchange(type& cmp_and_old, const type& exch) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			return atomic_storage<type>::compare_exchange(raw(), cmp_and_old, exch);
		}

		// See atomic.hpp atomic_t
		template <typename F, typename RT = std::invoke_result_t<F, T&>> requires (!std::is_const_v<T>)
		std::conditional_t<std::is_void_v<RT>, type, std::pair<type, RT>> fetch_op(F func) const
		{
			ensure(this->aligned());
			type _new, old = atomic_storage<type>::load(raw());

			while (true)
			{
				_new = old;

				if constexpr (std::is_void_v<RT>)
				{
					std::invoke(func, _new);

					if (atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
					{
						return old;
					}
				}
				else
				{
					RT ret = std::invoke(func, _new);

					if (!ret || atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
					{
						return {old, std::move(ret)};
					}
				}
			}
		}

		// See atomic.hpp atomic_t
		template <typename F, typename RT = std::invoke_result_t<F, T&>> requires (!std::is_const_v<T>)
		RT atomic_op(F func) const
		{
			ensure(this->aligned());
			type _new, old = atomic_storage<type>::load(raw());

			while (true)
			{
				_new = old;

				if constexpr (std::is_void_v<RT>)
				{
					std::invoke(func, _new);

					if (atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
					{
						return;
					}
				}
				else
				{
					RT result = std::invoke(func, _new);

					if (atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
					{
						return result;
					}
				}
			}
		}

		// See atomic.hpp atomic_t
		type load() const
		{
			ensure(this->aligned());
			return atomic_storage<type>::load(raw());
		}

		// See atomic.hpp atomic_t
		operator std::common_type_t<T>() const
		{
			return load();
		}

		// See atomic.hpp atomic_t
		type observe() const
		{
			ensure(this->aligned());
			return atomic_storage<type>::observe(raw());
		}

		// See atomic.hpp atomic_t
		void store(const type& rhs) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			atomic_storage<type>::store(raw(), rhs);
		}

		// See atomic.hpp atomic_t
		type operator =(const type& rhs) const requires (!std::is_const_v<T>)
		{
			store(rhs);
			return rhs;
		}

		// See atomic.hpp atomic_t
		void release(const type& rhs) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			atomic_storage<type>::release(raw(), rhs);
		}

		// See atomic.hpp atomic_t
		type exchange(const type& rhs) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			return atomic_storage<type>::exchange(raw(), rhs);
		}

		// See atomic.hpp atomic_t
		auto fetch_add(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return fetch_op([&](T& v)
			{
				v += rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto add_fetch(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				v += rhs;
				return v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator +=(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				return v += rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto fetch_sub(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return fetch_op([&](T& v)
			{
				v -= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto sub_fetch(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				v -= rhs;
				return v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator -=(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				return v -= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto fetch_and(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return fetch_op([&](T& v)
			{
				v &= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto and_fetch(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				v &= rhs;
				return v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator &=(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				return v &= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto fetch_or(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return fetch_op([&](T& v)
			{
				v |= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto or_fetch(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				v |= rhs;
				return v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator |=(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				return v |= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto fetch_xor(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return fetch_op([&](T& v)
			{
				v ^= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto xor_fetch(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				v ^= rhs;
				return v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator ^=(const type& rhs) const requires (!std::is_const_v<T>)
		{
			return atomic_op([&](T& v)
			{
				return v ^= rhs;
			});
		}

		// See atomic.hpp atomic_t
		auto operator ++() const requires (!std::is_const_v<T>)
		{
			return atomic_op([](T& v)
			{
				return ++v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator --() const requires (!std::is_const_v<T>)
		{
			return atomic_op([](T& v)
			{
				return --v;
			});
		}

		// See atomic.hpp atomic_t
		auto operator ++(int) const requires (!std::is_const_v<T>)
		{
			return atomic_op([](T& v)
			{
				return v++;
			});
		}

		// See atomic.hpp atomic_t
		auto operator --(int) const requires (!std::is_const_v<T>)
		{
			return atomic_op([](T& v)
			{
				return v--;
			});
		}

		// See atomic.hpp atomic_t
		bool try_dec(std::common_type_t<T> greater_than) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			type _new, old = atomic_storage<type>::load(raw());

			while (true)
			{
				_new = old;

				if (!(_new > greater_than))
				{
					return false;
				}

				_new -= 1;

				if (atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
				{
					return true;
				}
			}
		}

		// See atomic.hpp atomic_t
		bool try_inc(std::common_type_t<T> less_than) const requires (!std::is_const_v<T>)
		{
			ensure(this->aligned());
			type _new, old = atomic_storage<type>::load(raw());

			while (true)
			{
				_new = old;

				if (!(_new < less_than))
				{
					return false;
				}

				_new += 1;

				if (atomic_storage<type>::compare_exchange(raw(), old, _new)) [[likely]]
				{
					return true;
				}
			}
		}

		// See atomic.hpp atomic_t
		bool bit_test_set(uint bit) const requires (!std::is_const_v<T>)
		{
			return atomic_op([](type& v)
			{
				const auto old = v;
				const auto bit = type(1) << (sizeof(T) * 8 - 1);
				v |= bit;
				return !!(old & bit);
			});
		}

		// See atomic.hpp atomic_t
		bool bit_test_reset(uint bit) const requires (!std::is_const_v<T>)
		{
			return atomic_op([](type& v)
			{
				const auto old = v;
				const auto bit = type(1) << (sizeof(T) * 8 - 1);
				v &= ~bit;
				return !!(old & bit);
			});
		}

		// See atomic.hpp atomic_t
		bool bit_test_invert(uint bit) const requires (!std::is_const_v<T>)
		{
			return atomic_op([](type& v)
			{
				const auto old = v;
				const auto bit = type(1) << (sizeof(T) * 8 - 1);
				v ^= bit;
				return !!(old & bit);
			});
		}
	};

	// Native endianness reference to LE data
	template <typename T, typename AT = u32>
	using refl = _ref_base<to_le_t<T>, AT>;

	// Native endianness reference to BE data
	template <typename T, typename AT = u32>
	using refb = _ref_base<to_be_t<T>, AT>;

	// BE reference to LE data
	template <typename T, typename AT = u32>
	using brefl = _ref_base<to_le_t<T>, to_be_t<AT>>;

	// BE reference to BE data
	template <typename T, typename AT = u32>
	using brefb = _ref_base<to_be_t<T>, to_be_t<AT>>;

	// LE reference to LE data
	template <typename T, typename AT = u32>
	using lrefl = _ref_base<to_le_t<T>, to_le_t<AT>>;

	// LE reference to BE data
	template <typename T, typename AT = u32>
	using lrefb = _ref_base<to_be_t<T>, to_le_t<AT>>;

	inline namespace ps3_
	{
		// Default reference for PS3 HLE functions (Native endianness reference to BE data)
		template <typename T, typename AT = u32>
		using ref = refb<T, AT>;

		// Default reference for PS3 HLE structures (BE reference to BE data)
		template <typename T, typename AT = u32>
		using bref = brefb<T, AT>;

		// Atomic specialization (native endianness reference to atomic BE data)
		template <typename T, typename AT = u32>
		using atomic_ref = _ref_base<atomic_t<to_be_t<T>>, AT>;

		// Atomic specialization for structures (BE reference to atomic BE data)
		template <typename T, typename AT = u32>
		using atomic_bref = _ref_base<atomic_t<to_be_t<T>>, to_be_t<AT>>;
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

// Format address
template<typename T, typename AT>
struct fmt_unveil<vm::_ref_base<T, AT>, void>
{
	using type = vm::_ref_base<T, u32>; // Use only T, ignoring AT

	static inline auto get(const vm::_ref_base<T, AT>& arg)
	{
		return fmt_unveil<AT>::get(arg.addr());
	}
};

template <typename T>
struct fmt_class_string<vm::_ref_base<T, u32>, void> : fmt_class_string<vm::_ptr_base<const void, u32>, void>
{
	// TODO
};

template <>
struct fmt_class_string<vm::_ref_base<char, u32>, void> : fmt_class_string<vm::_ptr_base<const char, u32>, void>
{
};

template <>
struct fmt_class_string<vm::_ref_base<const char, u32>, void> : fmt_class_string<vm::_ptr_base<const char, u32>, void>
{
};