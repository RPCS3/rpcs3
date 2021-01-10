#pragma once

#include "vm_ptr.h"

#include "util/to_endian.hpp"

namespace vm
{
	template <memory_location_t Location = vm::main>
	struct page_allocator
	{
		static inline vm::addr_t alloc(u32 size, u32 align)
		{
			return vm::cast(vm::alloc(size, Location, std::max<u32>(align, 0x10000)));
		}

		static inline void dealloc(u32 addr, u32 size = 0) noexcept
		{
			ensure(vm::dealloc(addr, Location));
		}
	};

	template <typename T>
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

	// General variable base class
	template <typename T, typename A>
	class _var_base final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

	public:
		// Unmoveable object
		_var_base(const _var_base&) = delete;

		_var_base& operator=(const _var_base&) = delete;

		_var_base()
		    : pointer(A::alloc(sizeof(T), alignof(T)))
		{
		}

		_var_base(const T& right)
		    : _var_base()
		{
			std::memcpy(pointer::get_ptr(), &right, sizeof(T));
		}

		~_var_base()
		{
			if (pointer::addr())
			{
				A::dealloc(pointer::addr(), sizeof(T));
			}
		}
	};

	// Dynamic length array variable specialization
	template <typename T, typename A>
	class _var_base<T[], A> final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

		u32 m_size;

	public:
		_var_base(const _var_base&) = delete;

		_var_base& operator=(const _var_base&) = delete;

		_var_base(u32 count)
		    : pointer(A::alloc(u32{sizeof(T)} * count, alignof(T)))
		    , m_size(u32{sizeof(T)} * count)
		{
		}

		// Initialize via the iterator
		template <typename I>
		_var_base(u32 count, I&& it)
			: _var_base(count)
		{
			std::copy_n(std::forward<I>(it), count, pointer::get_ptr());
		}

		~_var_base()
		{
			if (pointer::addr())
			{
				A::dealloc(pointer::addr(), m_size);
			}
		}

		// Remove operator ->
		T* operator->() const = delete;

		u32 get_count() const
		{
			return m_size / u32{sizeof(T)};
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
	template <typename T, typename A>
	using varl = _var_base<to_le_t<T>, A>;

	// BE variable
	template <typename T, typename A>
	using varb = _var_base<to_be_t<T>, A>;

	inline namespace ps3_
	{
		// BE variable
		template <typename T, typename A = stack_allocator<ppu_thread>>
		using var = varb<T, A>;

		// Make BE variable initialized from value
		template <typename T, typename A = stack_allocator<ppu_thread>>
		[[nodiscard]] auto make_var(const T& value)
		{
			return (varb<T, A>(value));
		}

		// Make char[] variable initialized from std::string
		template <typename A = stack_allocator<ppu_thread>>
		[[nodiscard]] auto make_str(const std::string& str)
		{
			return (_var_base<char[], A>(size32(str) + 1, str.c_str()));
		}

		// Global HLE variable
		template <typename T, uint Count = 1>
		struct gvar final : ptr<T>
		{
			static constexpr u32 alloc_size{sizeof(T) * Count};
			static constexpr u32 alloc_align{alignof(T)};
		};
	} // namespace ps3_
} // namespace vm
