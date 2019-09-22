#pragma once

#include <memory>
#include <utility>
#include <type_traits>
#include <util/typeindices.hpp>

namespace stx
{
	// Typemap with exactly one object of each used type, created on init() and destroyed on clear()
	template <typename /*Tag*/>
	class manual_fixed_typemap
	{
		// Save default constructor and destructor
		struct typeinfo
		{
			void(*create)(void*& ptr) noexcept;
			void(*destroy)(void*& ptr) noexcept;

			template <typename T>
			static void call_ctor(void*& ptr) noexcept
			{
				// Call default constructor only if available
				if constexpr (std::is_default_constructible_v<T>)
				{
					// Don't overwrite if already exists
					if (!ptr)
					{
						ptr = new T();
					}
				}
			}

			template <typename T>
			static void call_dtor(void*& ptr) noexcept
			{
				delete static_cast<T*>(ptr);
				ptr = nullptr;
			}

			template <typename T>
			static typeinfo make_typeinfo()
			{
				typeinfo r;
				r.create = &call_ctor<T>;
				r.destroy = &call_dtor<T>;
				return r;
			}
		};

		std::unique_ptr<void*[]> m_list;

	public:
		constexpr manual_fixed_typemap() noexcept = default;

		manual_fixed_typemap(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap(manual_fixed_typemap&& r) noexcept
		{
			std::swap(m_list, r.m_list);
		}

		manual_fixed_typemap& operator=(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap& operator=(manual_fixed_typemap&& r) noexcept
		{
			manual_fixed_typemap x(std::move(*this));
			std::swap(m_list, x.m_list);
		}

		// Destroy all objects and keep them in uninitialized state, must be called first
		void reset() noexcept
		{
			if (!m_list)
			{
				m_list = std::make_unique<void*[]>(stx::typelist_v<typeinfo>.count());
				return;
			}

			for (auto& type : stx::typelist_v<typeinfo>)
			{
				type.destroy(m_list[type.index()]);
			}
		}

		// Default initialize all objects if possible and not already initialized
		void init() noexcept
		{
			for (auto& type : stx::typelist_v<typeinfo>)
			{
				type.create(m_list[type.index()]);
			}
		}

		// Explicitly (re)initialize object of type T possibly with dynamic type As and arguments
		template <typename T, typename As = T, typename... Args>
		As* init(Args&&... args) noexcept
		{
			auto& ptr = m_list[stx::typeindex<typeinfo, std::decay_t<T>>()];

			if (ptr)
			{
				delete static_cast<T*>(ptr);
			}

			As* obj = new std::decay_t<As>(std::forward<Args>(args)...);
			ptr = static_cast<T*>(obj);
			return obj;
		}

		// Obtain object pointer (the only thread safe function)
		template <typename T>
		T* get() const noexcept
		{
			return static_cast<T*>(m_list[stx::typeindex<typeinfo, std::decay_t<T>>()]);
		}
	};
}
