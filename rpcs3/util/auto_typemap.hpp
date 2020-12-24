#pragma once

#include "util/typeindices.hpp"

#include <utility>
#include <type_traits>

namespace stx
{
	// Simplified typemap with exactly one object of each used type, non-moveable.
	template <typename Tag /*Tag should be unique*/>
	class auto_typemap
	{
		// Save default constructor and destructor
		struct typeinfo
		{
			void(*create)(void*& ptr, auto_typemap&) noexcept;
			void(*destroy)(void* ptr) noexcept;

			template <typename T>
			static void call_ctor(void*& ptr, auto_typemap& _this) noexcept
			{
				// Don't overwrite if already exists
				if (!ptr)
				{
					// Allow passing reference to "this"
					if constexpr (std::is_constructible_v<T, auto_typemap&>)
					{
						ptr = new T(_this);
						return;
					}

					// Call default constructor only if available
					if constexpr (std::is_default_constructible_v<T>)
					{
						ptr = new T();
						return;
					}
				}
			}

			template <typename T>
			static void call_dtor(void* ptr) noexcept
			{
				delete static_cast<T*>(ptr);
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

		// Raw pointers to existing objects (may be nullptr)
		void** m_list;

		// Creation order for each object (used to reverse destruction order)
		void** m_order;

		// Helper for destroying in reverse order
		const typeinfo** m_info;

	public:
		auto_typemap() noexcept
			: m_list(new void*[stx::typelist<typeinfo>().count()]{})
			, m_order(new void*[stx::typelist<typeinfo>().count()])
			, m_info(new const typeinfo*[stx::typelist<typeinfo>().count()])
		{
			for (const auto& type : stx::typelist<typeinfo>())
			{
				const unsigned id = type.index();

				type.create(m_list[id], *this);

				// Allocate initialization order id
				if (m_list[id])
				{
					*m_order++ = m_list[id];
					*m_info++ = &type;
				}
			}
		}

		auto_typemap(const auto_typemap&) = delete;

		auto_typemap& operator=(const auto_typemap&) = delete;

		~auto_typemap()
		{
			// Get actual number of created objects
			unsigned _max = 0;

			for (const auto& type : stx::typelist<typeinfo>())
			{
				if (m_list[type.index()])
				{
					// Skip object if not created
					_max++;
				}
			}

			// Destroy objects in reverse order
			for (; _max; _max--)
			{
				(*--m_info)->destroy(*--m_order);
			}

			// Pointers should be restored to their positions
			delete[] m_info;
			delete[] m_order;
			delete[] m_list;
		}

		// Check if object is not initialized but shall be initialized first (to use in initializing other objects)
		template <typename T>
		void need() noexcept
		{
			if (!m_list[stx::typeindex<typeinfo, std::decay_t<T>>()])
			{
				if constexpr (std::is_constructible_v<T, auto_typemap&>)
				{
					init<T>(*this);
					return;
				}

				if constexpr (std::is_default_constructible_v<T>)
				{
					init<T>();
					return;
				}
			}
		}

		// Explicitly initialize object of type T possibly with dynamic type As and arguments
		template <typename T, typename As = T, typename... Args>
		As* init(Args&&... args) noexcept
		{
			auto& ptr = m_list[stx::typeindex<typeinfo, std::decay_t<T>>()];

			if (ptr)
			{
				// Already exists, recreation is not supported (may be added later)
				return nullptr;
			}

			As* obj = new std::decay_t<As>(std::forward<Args>(args)...);
			*m_order++ = obj;
			*m_info++ = &stx::typedata<typeinfo, std::decay_t<T>>();
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

		// Obtain object reference
		template <typename T>
		T& get() const noexcept
		{
			return *static_cast<T*>(m_list[stx::typeindex<typeinfo, std::decay_t<T>>()]);
		}
	};
}

using stx::auto_typemap;
