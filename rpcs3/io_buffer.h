#pragma once
#include <util/types.hpp>
#include <span>
#include <vector>
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
		void* m_ptr = nullptr;
		usz m_size = 0;

		std::function<std::tuple<void*, usz> ()> m_allocator = nullptr;

	public:
		io_buffer() = default;

		template <SpanLike T>
		io_buffer(T& container)
		{
			m_ptr = reinterpret_cast<void*>(container.data());
			m_size = container.size_bytes();
		}

		io_buffer(std::function<std::tuple<void*, usz> ()> allocator)
		{
			ensure(allocator);
			m_allocator = allocator;
		}

		template<Integral T>
		T* data()
		{
			if (!m_ptr && m_allocator)
			{
				std::tie(m_ptr, m_size) = m_allocator();
			}

			return static_cast<T*>(m_ptr);
		}

		usz size() const
		{
			return m_size;
		}

		template<Integral T>
		std::span<T> as_span()
		{
			const auto bytes = data<T>();
			return { bytes, m_size / sizeof(T) };
		}
	};
}
