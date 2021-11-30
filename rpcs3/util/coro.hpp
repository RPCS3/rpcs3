#pragma once

#if __has_include(<coroutine>)
#if defined(__clang__) && !defined(__cpp_impl_coroutine)
#define __cpp_impl_coroutine 123
#endif

#include <coroutine>

#if defined(__clang__) && (__cpp_impl_coroutine == 123)
namespace std::experimental
{
	using ::std::coroutine_traits;
	using ::std::coroutine_handle;
}
#endif
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace std
{
	using experimental::coroutine_traits;
	using experimental::coroutine_handle;
	using experimental::noop_coroutine_handle;
	using experimental::suspend_always;
	using experimental::suspend_never;
}
#endif

#include <iterator>

namespace stx
{
	template <typename T>
	struct lazy;

	template <typename T>
	struct generator;

	namespace detail
	{
		struct lazy_promise_base
		{
			struct final_suspend_t
			{
				constexpr bool await_ready() const noexcept { return false; }

				template <typename P>
				std::coroutine_handle<> await_suspend(std::coroutine_handle<P> h) noexcept
				{
					return h.promise().m_handle;
				}

				void await_resume() noexcept {}
			};

			constexpr lazy_promise_base() noexcept = default;
			lazy_promise_base(const lazy_promise_base&) = delete;
			lazy_promise_base& operator=(const lazy_promise_base&) = delete;
			std::suspend_always initial_suspend() { return {}; }
			final_suspend_t final_suspend() noexcept { return {}; }
			void unhandled_exception() {}

		protected:
			std::coroutine_handle<> m_handle{};
		};

		template <typename T>
		struct lazy_promise final : lazy_promise_base
		{
			constexpr lazy_promise() noexcept = default;

			lazy<T> get_return_object() noexcept;

			template <typename U>
			void return_value(U&& value)
			{
				m_value = std::forward<U>(value);
			}

			T& result() &
			{
				return m_value;
			}

			T&& result() &&
			{
				return m_value;
			}

		private:
			T m_value{};
		};

		template <typename T>
		struct lazy_promise<T&> final : lazy_promise_base
		{
			constexpr lazy_promise() noexcept = default;

			lazy<T&> get_return_object() noexcept;

			void return_value(T& value) noexcept
			{
				m_value = std::addressof(value);
			}

			T& result()
			{
				return *m_value;
			}

		private:
			T* m_value{};
		};

		template <>
		struct lazy_promise<void> final : lazy_promise_base
		{
			constexpr lazy_promise() noexcept = default;
			lazy<void> get_return_object() noexcept;
			void return_void() noexcept {}
			void result() {}
		};
	}

	template <typename T = void>
	struct [[nodiscard]] lazy
	{
		using promise_type = detail::lazy_promise<T>;
		using value_type = T;

		struct awaitable_base
		{
			std::coroutine_handle<promise_type> coro;

			bool await_ready() const noexcept
			{
				return !coro || coro.done();
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
			{
				coro.promise().m_handle = h;
				return coro;
			}
		};

		lazy(const lazy&) = delete;

		lazy(lazy&& rhs) noexcept
			: m_handle(rhs.m_handle)
		{
			rhs.m_handle = nullptr;
		}

		lazy& operator=(const lazy&) = delete;

		~lazy()
		{
			if (m_handle)
			{
				m_handle.destroy();
			}
		}

		bool is_ready() const noexcept
		{
			return !m_handle || m_handle.done();
		}

		auto operator co_await() const & noexcept
		{
			struct awaitable : awaitable_base
			{
				using awaitable_base::awaitable_base;

				decltype(auto) await_resume()
				{
					return this->m_handle.promise().result();
				}
			};

			return awaitable{m_handle};
		}

		auto operator co_await() const && noexcept
		{
			struct awaitable : awaitable_base
			{
				using awaitable_base::awaitable_base;

				decltype(auto) await_resume()
				{
					return std::move(this->m_handle.promise()).result();
				}
			};

			return awaitable{m_handle};
		}

	private:
		friend promise_type;

		explicit constexpr lazy(std::coroutine_handle<promise_type> h) noexcept
			: m_handle(h)
		{
		}

		std::coroutine_handle<promise_type> m_handle;
	};

	template <typename T>
	inline lazy<T> detail::lazy_promise<T>::get_return_object() noexcept
	{
		return lazy<T>{std::coroutine_handle<lazy_promise>::from_promise(*this)};
	}

	template <typename T>
	inline lazy<T&> detail::lazy_promise<T&>::get_return_object() noexcept
	{
		return lazy<T&>{std::coroutine_handle<lazy_promise>::from_promise(*this)};
	}

	inline lazy<void> detail::lazy_promise<void>::get_return_object() noexcept
	{
		return lazy<void>{std::coroutine_handle<lazy_promise>::from_promise(*this)};
	}

	namespace detail
	{
		template <typename T>
		struct generator_promise
		{
			using value_type = std::remove_reference_t<T>;
			using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;

			generator_promise() = default;

			generator<T> get_return_object() noexcept;

			constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
			constexpr std::suspend_always final_suspend() const noexcept { return {}; }
			void unhandled_exception() {}

			std::suspend_always yield_value(std::remove_reference_t<T>& value) noexcept
			{
				m_ptr = std::addressof(value);
				return {};
			}

			std::suspend_always yield_value(std::remove_reference_t<T>&& value) noexcept
			{
				m_ptr = std::addressof(value);
				return {};
			}

			void return_void() {}

			reference_type value() const noexcept
			{
				return static_cast<reference_type>(*m_ptr);
			}

			template <typename U>
			std::suspend_never await_transform(U&& value) = delete;

		private:
			value_type* m_ptr;
		};

		struct generator_end
		{
		};

		template<typename T>
		struct generator_iterator
		{
			using iterator_category = std::input_iterator_tag;
			using value_type = typename generator_promise<T>::value_type;
			using reference = typename generator_promise<T>::reference_type;

			constexpr generator_iterator() noexcept = default;

			explicit constexpr generator_iterator(std::coroutine_handle<generator_promise<T>> coro) noexcept
				: m_coro(coro)
			{
			}

			bool operator==(generator_end) const noexcept
			{
				return !m_coro || m_coro.done();
			}

			generator_iterator& operator++()
			{
				m_coro.resume();
				return *this;
			}

			void operator++(int)
			{
				m_coro.resume();
			}

			reference operator*() const noexcept
			{
				return m_coro.promise().value();
			}

			value_type* operator->() const noexcept
			{
				return std::addressof(operator*());
			}

		private:
			std::coroutine_handle<generator_promise<T>> m_coro;
		};
	}

	template<typename T>
	struct [[nodiscard]] generator
	{
		using promise_type = detail::generator_promise<T>;
		using iterator = detail::generator_iterator<T>;

		generator(const generator&) = delete;

		generator(generator&& rhs) noexcept
			: m_handle(rhs.m_handle)
		{
			rhs.m_handle = nullptr;
		}

		generator& operator=(const generator&) = delete;

		~generator()
		{
			if (m_handle)
			{
				m_handle.destroy();
			}
		}

		iterator begin()
		{
			if (m_handle)
			{
				m_handle.resume();
			}

			return iterator{m_handle};
		}

		detail::generator_end end() noexcept
		{
			return detail::generator_end{};
		}

	private:
		friend promise_type;

		explicit constexpr generator(std::coroutine_handle<promise_type> h) noexcept
			: m_handle(h)
		{
		}

		std::coroutine_handle<promise_type> m_handle;
	};

	template <typename T>
	generator<T> detail::generator_promise<T>::get_return_object() noexcept
	{
		return generator<T>{std::coroutine_handle<generator_promise<T>>::from_promise(*this)};
	}
}
