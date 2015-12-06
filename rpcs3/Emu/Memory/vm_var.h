#pragma once

namespace vm
{
	template<memory_location_t Location = vm::main> class page_alloc_t
	{
		u32 m_addr;

	public:
		static inline u32 alloc(u32 size, u32 align)
		{
			return vm::alloc(size, Location, std::max<u32>(align, 4096));
		}

		static inline void dealloc(u32 addr, u32 size) noexcept
		{
			return vm::dealloc_verbose_nothrow(addr, Location);
		}

		page_alloc_t()
			: m_addr(0)
		{
		}

		page_alloc_t(u32 size, u32 align)
			: m_addr(alloc(size, align))
		{
		}

		page_alloc_t(page_alloc_t&& other)
			: m_addr(other.m_addr)
		{
			other.m_addr = 0;
		}

		~page_alloc_t()
		{
			if (m_addr)
			{
				dealloc(m_addr, 0);
			}
		}

		page_alloc_t& operator =(page_alloc_t&& other)
		{
			std::swap(m_addr, other.m_addr);

			return *this;
		}

		u32 get_addr() const
		{
			return m_addr;
		}
	};

	class stack_alloc_t
	{
		u32 m_addr;
		u32 m_size;

	public:
		static inline u32 alloc(u32 size, u32 align)
		{
			return vm::stack_push(size, align);
		}

		static inline void dealloc(u32 addr, u32 size)
		{
			if (!std::uncaught_exception()) // Don't call during stack unwinding
			{
				vm::stack_pop(addr, size);
			}
		}

		stack_alloc_t(u32 size, u32 align)
			: m_addr(alloc(size, align))
			, m_size(size)
		{
		}
	
		~stack_alloc_t() noexcept(false) // Allow exceptions
		{
			dealloc(m_addr, m_size);
		}

		stack_alloc_t(const stack_alloc_t&) = delete; // Delete copy/move constructors and copy/move operators

		u32 get_addr() const
		{
			return m_addr;
		}
	};

	// _var_base prototype (T - data type, A - allocation traits)
	template<typename T, typename A> class _var_base;

	// _var_base general specialization (single object of type T)
	template<typename T, typename A> class _var_base final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

	public:
		// Call the constructor with specified arguments
		template<typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>> _var_base(Args&&... args)
			: pointer(A::alloc(sizeof32(T), alignof32(T)), vm::addr)
		{
#include "restore_new.h"
			new(pointer::get_ptr()) T(std::forward<Args>(args)...);
#include "define_new_memleakdetect.h"
		}

		_var_base(const _var_base&) = delete; // Delete copy/move constructors and copy/move operators

		~_var_base() noexcept(noexcept(std::declval<T&>().~T()) && noexcept(A::dealloc(0, 0)))
		{
			// Call the destructor
			pointer::get_ptr()->~T();

			// Deallocate memory
			A::dealloc(pointer::addr(), sizeof32(T));
		}

		// Remove operator []
		std::add_lvalue_reference_t<T> operator [](u32 index) const = delete;
	};

	// _var_base unknown length array specialization
	template<typename T, typename A> class _var_base<T[], A> final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

		u32 m_count;

	public:
		// Call the default constructor for each element
		_var_base(u32 count)
			: pointer(A::alloc(sizeof32(T) * count, alignof32(T)), vm::addr)
			, m_count(count)
		{
#include "restore_new.h"
			new(pointer::get_ptr()) T[count]();
#include "define_new_memleakdetect.h"
		}

		// Call the constructor for each element using [it, it + count)
		template<typename T2> _var_base(u32 count, T2 it)
			: pointer(A::alloc(sizeof32(T) * count, alignof32(T)), vm::addr)
			, m_count(count)
		{
#include "restore_new.h"
			for (u32 i = 0; i < m_count; i++, it++) new(pointer::get_ptr() + i) T(*it);
#include "define_new_memleakdetect.h"
		}

		_var_base(const _var_base&) = delete; // Delete copy/move constructors and copy/move operators

		~_var_base() noexcept(noexcept(std::declval<T&>().~T()) && noexcept(A::dealloc(0, 0)))
		{
			// Call the destructor for each element
			for (u32 i = m_count - 1; ~i; i--) pointer::operator [](i).~T();

			// Deallocate memory
			A::dealloc(pointer::addr(), sizeof32(T) * m_count);
		}

		u32 get_count() const
		{
			return m_count;
		}

		std::add_lvalue_reference_t<T> at(u32 index) const
		{
			if (index >= m_count) throw EXCEPTION("Out of range (0x%x >= 0x%x)", index, m_count);

			return pointer::operator [](index);
		}

		// Remove operator ->
		T* operator ->() const = delete;
	};

	// _var_base fixed length array specialization
	template<typename T, typename A, u32 N> class _var_base<T[N], A> final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

	public:
		// Call the default constructor for each element
		_var_base()
			: pointer(A::alloc(sizeof32(T) * N, alignof32(T)), vm::addr)
		{
#include "restore_new.h"
			new(pointer::get_ptr()) T[N]();
#include "define_new_memleakdetect.h"
		}

		// Call the constructor for each element using array
		template<typename T2> _var_base(const T2(&array)[N])
			: pointer(A::alloc(sizeof32(T) * N, alignof32(T)), vm::addr)
		{
#include "restore_new.h"
			for (u32 i = 0; i < N; i++) new(pointer::get_ptr() + i) T(array[i]);
#include "define_new_memleakdetect.h"
		}

		_var_base(const _var_base&) = delete; // Delete copy/move constructors and copy/move operators

		~_var_base() noexcept(noexcept(std::declval<T&>().~T()) && noexcept(A::dealloc(0, 0)))
		{
			// Call the destructor for each element
			for (u32 i = N - 1; ~i; i--) pointer::operator [](i).~T();

			// Deallocate memory
			A::dealloc(pointer::addr(), sizeof32(T) * N);
		}

		constexpr u32 get_count() const
		{
			return N;
		}

		std::add_lvalue_reference_t<T> at(u32 index) const
		{
			if (index >= N) throw EXCEPTION("Out of range (0x%x > 0x%x)", index, N);

			return pointer::operator [](index);
		}

		// Remove operator ->
		T* operator ->() const = delete;
	};

	// LE variable
	template<typename T, typename A = vm::stack_alloc_t> using varl = _var_base<to_le_t<T>, A>;

	// BE variable
	template<typename T, typename A = vm::stack_alloc_t> using varb = _var_base<to_be_t<T>, A>;

	namespace ps3
	{
		// BE variable
		template<typename T, typename A = vm::stack_alloc_t> using var = varb<T, A>;

		// BE variable initialized from value
		template<typename T> inline varb<T, vm::stack_alloc_t> make_var(const T& value)
		{
			return{ value };
		}
	}

	namespace psv
	{
		// LE variable
		template<typename T, typename A = vm::stack_alloc_t> using var = varl<T, A>;

		// LE variable initialized from value
		template<typename T> inline varl<T, vm::stack_alloc_t> make_var(const T& value)
		{
			return{ value };
		}
	}

	static _var_base<char[], vm::stack_alloc_t> make_str(const std::string& str)
	{
		return{ size32(str) + 1, str.data() };
	}
}
