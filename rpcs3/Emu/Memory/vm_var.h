#pragma once

#include "vm_ptr.h"

namespace vm
{
	template<memory_location_t Location = vm::main>
	struct page_allocator
	{
		static inline vm::addr_t alloc(u32 size, u32 align)
		{
			return vm::cast(vm::alloc(size, Location, std::max<u32>(align, 4096)));
		}

		static inline void dealloc(u32 addr, u32 size = 0) noexcept
		{
			return vm::dealloc_verbose_nothrow(addr, Location);
		}
	};

	struct stack_allocator
	{
		static inline vm::addr_t alloc(u32 size, u32 align)
		{
			return vm::cast(vm::stack_push(size, align));
		}

		static inline void dealloc(u32 addr, u32 size) noexcept
		{
			vm::stack_pop_verbose(addr, size);
		}
	};

	// Variable general specialization
	template<typename T, typename A>
	class _var_base final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

	public:
		_var_base()
			: pointer(A::alloc(SIZE_32(T), ALIGN_32(T)))
		{
		}

		_var_base(const T& right)
			: _var_base()
		{
			std::memcpy(pointer::get_ptr(), &right, sizeof(T));
		}

		_var_base(_var_base&& right)
			: pointer(right)
		{
			reinterpret_cast<u32&>(static_cast<pointer&>(right)) = 0;
		}

		~_var_base()
		{
			if (pointer::addr()) A::dealloc(pointer::addr(), SIZE_32(T));
		}
	};

	// Dynamic length array variable
	template<typename T, typename A>
	class _var_base<T[], A> final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

		u32 m_size;

	public:
		_var_base(u32 count)
			: pointer(A::alloc(SIZE_32(T) * count, ALIGN_32(T)))
			, m_size(SIZE_32(T) * count)
		{
		}

		_var_base(_var_base&& right)
			: pointer(right)
			, m_size(right.m_size)
		{
			reinterpret_cast<u32&>(static_cast<pointer&>(right)) = 0;
		}

		~_var_base()
		{
			if (pointer::addr()) A::dealloc(pointer::addr(), m_size);
		}

		// Remove operator ->
		T* operator ->() const = delete;

		u32 get_count() const
		{
			return m_size / SIZE_32(T);
		}
	};

	// LE variable
	template<typename T, typename A = stack_allocator> using varl = _var_base<to_le_t<T>, A>;

	// BE variable
	template<typename T, typename A = stack_allocator> using varb = _var_base<to_be_t<T>, A>;

	namespace ps3
	{
		// BE variable
		template<typename T, typename A = stack_allocator> using var = varb<T, A>;

		// Make BE variable initialized from value
		template<typename T> inline auto make_var(const T& value)
		{
			varb<T, stack_allocator> var(value);
			return var;
		}

		// Global HLE variable
		template<typename T>
		struct gvar : ptr<T>
		{
		};
	}

	namespace psv
	{
		// LE variable
		template<typename T, typename A = stack_allocator> using var = varl<T, A>;

		// Make LE variable initialized from value
		template<typename T> inline auto make_var(const T& value)
		{
			varl<T, stack_allocator> var(value);
			return var;
		}

		// Global HLE variable
		template<typename T>
		struct gvar : ptr<T>
		{
		};
	}

	// Make char[] variable initialized from std::string
	static auto make_str(const std::string& str)
	{
		_var_base<char[], stack_allocator> var(size32(str) + 1);
		std::memcpy(var.get_ptr(), str.c_str(), str.size() + 1);
		return var;
	}
}
