#pragma once
#include <util/types.hpp>
#include <util/bless.hpp>
#include <span>
#include <functional>

namespace rsx
{
	template <typename T>
	concept SpanLike = requires(T t)
	{
		{ t.data() } -> std::convertible_to<void*>;
		{ t.size_bytes() } -> std::convertible_to<usz>;
	};

	template <typename T>
	concept Integral = std::is_integral_v<T> || std::is_same_v<T, u128>;

	class io_buffer
	{
		mutable void* m_ptr = nullptr;
		mutable usz m_size = 0;

		std::function<std::tuple<void*, usz>(usz)> m_allocator{};
		mutable usz m_allocation_size = 0u;

	public:
		io_buffer() = default;

		template <SpanLike T>
		io_buffer(const T& container)
		{
			m_ptr = const_cast<void*>(reinterpret_cast<const void*>(container.data()));
			m_size = container.size_bytes();
		}

		io_buffer(std::function<std::tuple<void*, usz>(usz)> allocator)
		{
			ensure(allocator);
			m_allocator = allocator;
		}

		template <Integral T>
		io_buffer(void* ptr, T size)
			: m_ptr(ptr), m_size(size)
		{}

		template <Integral T>
		io_buffer(const void* ptr, T size)
			: m_ptr(const_cast<void*>(ptr)), m_size(size)
		{}

		void reserve(usz size) const
		{
			m_allocation_size = size;
		}

		std::pair<void*, usz> raw() const
		{
			return { m_ptr, m_size };
		}

		template <Integral T = u8>
		T* data() const
		{
			if (!m_ptr && m_allocator)
			{
				std::tie(m_ptr, m_size) = m_allocator(m_allocation_size);
			}

			return static_cast<T*>(m_ptr);
		}

		usz size() const
		{
			return m_size;
		}

		template<typename T>
		std::span<T> as_span() const
		{
			auto bytes = data();
			return { utils::bless<T>(bytes), m_size / sizeof(T) };
		}

		bool empty() const
		{
			return m_size == 0;
		}
	};
}
