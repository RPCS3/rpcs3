#pragma once

#include <memory>
#include <typeinfo>
#include <utility>
#include <type_traits>
#include <util/typeindices.hpp>

namespace stx
{
	namespace detail
	{
		// Destroy list element
		struct destroy_info
		{
			void** object_pointer;
			unsigned long long created;
			void(*destroy)(void*& ptr) noexcept;
			const char* name;

			static void sort_by_reverse_creation_order(destroy_info* begin, destroy_info* end);
		};
	}

	// Typemap with exactly one object of each used type, created on init() and destroyed on clear()
	template <typename /*Tag*/, bool Report = true>
	class manual_fixed_typemap
	{
		// Save default constructor and destructor
		struct typeinfo
		{
			void(*create)(void*& ptr) noexcept;
			void(*destroy)(void*& ptr) noexcept;
			const char* type_name = "__";

			template <typename T>
			static void call_ctor(void*& ptr) noexcept
			{
				// Don't overwrite if already exists
				if (!ptr)
				{
					// Call default constructor only if available
					if constexpr (std::is_default_constructible_v<T>)
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
				r.type_name = typeid(T).name();
				return r;
			}
		};

		// Raw pointers to existing objects (may be nullptr)
		std::unique_ptr<void*[]> m_list;

		// Creation order for each object (used to reverse destruction order)
		std::unique_ptr<unsigned long long[]> m_order;

		// Used to generate creation order (increased on every construction)
		unsigned long long m_init_count = 0;

		// Body is somewhere else if enabled
		void init_reporter(const char* name, unsigned long long created) const noexcept;

		// Body is somewhere else if enabled
		void destroy_reporter(const char* name, unsigned long long created) const noexcept;

	public:
		constexpr manual_fixed_typemap() noexcept = default;

		manual_fixed_typemap(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap(manual_fixed_typemap&& r) noexcept
			: m_list(std::move(r.m_list))
			, m_order(std::move(r.m_order))
			, m_init_count(r.m_init_count)
		{
			r.m_init_count = 0;
		}

		manual_fixed_typemap& operator=(const manual_fixed_typemap&) = delete;

		manual_fixed_typemap& operator=(manual_fixed_typemap&& r) noexcept
		{
			manual_fixed_typemap x(std::move(*this));
			std::swap(m_list, x.m_list);
			std::swap(m_order, x.m_order);
			std::swap(m_init_count, x.m_init_count);
		}

		~manual_fixed_typemap()
		{
			if (!m_list && !m_order)
			{
				return;
			}

			reset();
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

			using detail::destroy_info;

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
				all_data[_max].name = type.type_name;

				// Clear creation order
				m_order[type.index()] = 0;
				_max++;
			}

			// Sort destroy list according to absolute creation order
			destroy_info::sort_by_reverse_creation_order(all_data.get(), all_data.get() + _max);

			// Destroy objects in correct order
			for (unsigned i = 0; i < _max; i++)
			{
				if constexpr (Report)
					destroy_reporter(all_data[i].name, all_data[i].created);
				all_data[i].destroy(*all_data[i].object_pointer);
			}

			// Reset creation order since it now may be printed
			m_init_count = 0;
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
					if constexpr (Report)
						init_reporter(type.type_name, m_init_count);
				}
			}
		}

		// Check if object is not initialized but shall be initialized first (to use in initializing other objects)
		template <typename T>
		void need() noexcept
		{
			if (!get<T>())
			{
				init<T>();
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

		// CTAD adaptor for init (see init description), accepts template not type
		template <template <class...> typename Template, typename... Args>
		auto init(Args&&... args) noexcept
		{
			// Deduce the type from given template and its arguments
			using T = decltype(Template(std::forward<Args>(args)...));
			return init<T>(std::forward<Args>(args)...);
		}

		// Obtain object pointer (thread safe just against other get calls)
		template <typename T>
		T* get() const noexcept
		{
			return static_cast<T*>(m_list[stx::typeindex<typeinfo, std::decay_t<T>>()]);
		}
	};
}
