#pragma once

#include <memory>
#include <utility>
#include <type_traits>
#include <algorithm>
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

		std::unique_ptr<unsigned long long[]> m_order;
		unsigned long long m_init_count = 0;

	public:
		constexpr manual_fixed_typemap() noexcept = default;

		manual_fixed_typemap(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap(manual_fixed_typemap&& r) noexcept
		{
			std::swap(m_list, r.m_list);
			std::swap(m_order, r.m_order);
			std::swap(m_init_count, r.m_init_count);
		}

		manual_fixed_typemap& operator=(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap& operator=(manual_fixed_typemap&& r) noexcept
		{
			manual_fixed_typemap x(std::move(*this));
			std::swap(m_list, x.m_list);
			std::swap(m_order, x.m_order);
			std::swap(m_init_count, x.m_init_count);
		}

		// Destroy all objects and keep them in uninitialized state, must be called first
		void reset() noexcept
		{
			const auto total_count = stx::typelist<typeinfo>().count();

			if (!m_list)
			{
				m_list = std::make_unique<void*[]>(total_count);
				m_order = std::make_unique<unsigned long long[]>(total_count);
				return;
			}

			// Destroy list element
			struct destroy_info
			{
				void** object_pointer;
				unsigned long long created;
				void(*destroy)(void*& ptr) noexcept;
			};

			auto all_data = std::make_unique<destroy_info[]>(stx::typelist<typeinfo>().count());

			// Actual number of created objects
			unsigned _max = 0;

			// Create destroy list
			for (auto& type : stx::typelist<typeinfo>())
			{
				if (m_order[type.index()] == 0)
				{
					// Skip object if not created
					continue;
				}

				all_data[_max].object_pointer = &m_list[type.index()];
				all_data[_max].created = m_order[type.index()];
				all_data[_max].destroy = type.destroy;

				// Clear creation order
				m_order[type.index()] = 0;
				_max++;
			}

			// Sort destroy list according to absolute creation order
			std::sort(all_data.get(), all_data.get() + _max, [](const destroy_info& a, const destroy_info& b)
			{
				// Destroy order is the inverse of creation order
				return a.created > b.created;
			});

			// Destroy objects in correct order
			for (unsigned i = 0; i < _max; i++)
			{
				all_data[i].destroy(*all_data[i].object_pointer);
			}
		}

		// Default initialize all objects if possible and not already initialized
		void init() noexcept
		{
			for (auto& type : stx::typelist<typeinfo>())
			{
				type.create(m_list[type.index()]);

				// Allocate initialization order id
				if (m_list[type.index()])
				{
					m_order[type.index()] = ++m_init_count;
				}
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
			m_order[stx::typeindex<typeinfo, std::decay_t<T>>()] = ++m_init_count;
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
