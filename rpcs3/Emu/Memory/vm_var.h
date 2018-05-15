#pragma once

#include "vm_ptr.h"

namespace vm
{
	template<memory_location_t Location = vm::main>
	struct page_allocator
	{
		static inline vm::addr_t alloc(u32 size, u32 align)
		{
			return vm::cast(vm::alloc(size, Location, std::max<u32>(align, 0x10000)));
		}

		static inline void dealloc(u32 addr, u32 size = 0) noexcept
		{
			return vm::dealloc_verbose_nothrow(addr, Location);
		}
	};

	template<typename T>
	struct stack_allocator
	{
		static inline vm::addr_t alloc(u32 size, u32 align)
		{
			return vm::cast(T::stack_push(size, align));
		}

		static inline void dealloc(u32 addr, u32 size) noexcept
		{
			T::stack_pop_verbose(addr, size);
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

		auto begin() const
		{
			return *this + 0;
		}

		auto end() const
		{
			return *this + get_count();
		}
	};

	// LE variable
	template<typename T, typename A> using varl = _var_base<to_le_t<T>, A>;

	// BE variable
	template<typename T, typename A> using varb = _var_base<to_be_t<T>, A>;

	inline namespace ps3_
	{
		// BE variable
		template<typename T, typename A = stack_allocator<ppu_thread>> using var = varb<T, A>;

		// Make BE variable initialized from value
		template<typename T, typename A = stack_allocator<ppu_thread>>
		inline auto make_var(const T& value)
		{
			return varb<T, A>(value);
		}

		// Make char[] variable initialized from std::string
		template<typename A = stack_allocator<ppu_thread>>
		static auto make_str(const std::string& str)
		{
			var<char[], A> var_(size32(str) + 1);
			std::memcpy(var_.get_ptr(), str.c_str(), str.size() + 1);
			return var_;
		}

		// Global HLE variable
		template<typename T>
		struct gvar : ptr<T>
		{
		};
	}
}
