#pragma once

#include "vm_ptr.h"

#include "util/to_endian.hpp"

namespace vm
{
	template <memory_location_t Location = vm::main>
	struct page_allocator
	{
		static inline std::pair<vm::addr_t, u32> alloc(u32 size, u32 align)
		{
			return {vm::cast(vm::alloc(size, Location, std::max<u32>(align, 0x10000))), size};
		}

		static inline void dealloc(u32 addr, u32 size) noexcept
		{
			ensure(vm::dealloc(addr, Location) >= size);
		}
	};

	template <typename T>
	struct stack_allocator
	{
		static inline std::pair<vm::addr_t, u32> alloc(u32 size, u32 align)
		{
			return T::stack_push(size, align);
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

		const u32 m_mem_size;

		_var_base(std::pair<vm::addr_t, u32> alloc_info)
			: pointer(alloc_info.first)
			, m_mem_size(alloc_info.second)
		{
		}

	public:
		// Unmoveable object
		_var_base(const _var_base&) = delete;

		_var_base& operator=(const _var_base&) = delete;

		using enable_bitcopy = std::false_type; // Disable bitcopy inheritence

		_var_base()
		    : _var_base(A::alloc(sizeof(T), alignof(T)))
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
				A::dealloc(pointer::addr(), m_mem_size);
			}
		}
	};

	// Dynamic length array variable specialization
	template <typename T, typename A>
	class _var_base<T[], A> final : public _ptr_base<T, const u32>
	{
		using pointer = _ptr_base<T, const u32>;

		const u32 m_mem_size;
		const u32 m_size;

		_var_base(u32 count, std::pair<vm::addr_t, u32> alloc_info)
			: pointer(alloc_info.first)
			, m_mem_size(alloc_info.second)
		    , m_size(u32{sizeof(T)} * count)
		{
		}

	public:
		_var_base(const _var_base&) = delete;

		_var_base& operator=(const _var_base&) = delete;

		using enable_bitcopy = std::false_type; // Disable bitcopy inheritence

		_var_base(u32 count)
		    : _var_base(count, A::alloc(u32{sizeof(T)} * count, alignof(T)))
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
				A::dealloc(pointer::addr(), m_mem_size);
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
			static constexpr u32 alloc_align{std::max<u32>(alignof(T), 16)};
		};
	} // namespace ps3_
} // namespace vm
