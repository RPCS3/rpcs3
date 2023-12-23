#pragma once

#include "util/types.hpp"
#include "util/typeindices.hpp"

#include <utility>
#include <type_traits>

namespace stx
{
	// Simplified typemap with exactly one object of each used type, non-moveable.
	template <typename Tag /*Tag should be unique*/, u32 Size = 0, u32 Align = (Size ? 64 : __STDCPP_DEFAULT_NEW_ALIGNMENT__)>
	class alignas(Align) auto_typemap
	{
		// Save default constructor and destructor
		struct typeinfo
		{
			bool(*create)(uchar* ptr, auto_typemap&) noexcept = nullptr;
			void(*destroy)(void* ptr) noexcept = nullptr;

			template <typename T>
			static bool call_ctor(uchar* ptr, auto_typemap& _this) noexcept
			{
				{
					// Allow passing reference to "this"
					if constexpr (std::is_constructible_v<T, auto_typemap&>)
					{
						new (ptr) T(_this);
						return true;
					}

					// Call default constructor only if available
					if constexpr (std::is_default_constructible_v<T>)
					{
						new (ptr) T();
						return true;
					}
				}

				return false;
			}

			template <typename T>
			static void call_dtor(void* ptr) noexcept
			{
				std::launder(static_cast<T*>(ptr))->~T();
				std::memset(ptr, 0xCC, sizeof(T)); // Set to trap values
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

		// Objects
		union
		{
			uchar* m_list;
			mutable uchar m_data[Size ? Size : 1];
		};

		// Creation order for each object (used to reverse destruction order)
		void** m_order;

		// Helper for destroying in reverse order
		const typeinfo** m_info;

		// Indicates whether object is created at given index
		bool* m_init;

	public:
		auto_typemap() noexcept
			: m_order(new void*[stx::typelist<typeinfo>().count()])
			, m_info(new const typeinfo*[stx::typelist<typeinfo>().count()])
			, m_init(new bool[stx::typelist<typeinfo>().count()]{})
		{
			if constexpr (Size == 0)
			{
				if (stx::typelist<typeinfo>().align() > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
				{
					m_list = static_cast<uchar*>(::operator new(usz{stx::typelist<typeinfo>().size()}, std::align_val_t{stx::typelist<typeinfo>().align()}));
				}
				else
				{
					m_list = new uchar[stx::typelist<typeinfo>().size()];
				}
			}
			else
			{
				ensure(Size >= stx::typelist<typeinfo>().size());
				ensure(Align >= stx::typelist<typeinfo>().align());
			}

			// Set to trap values
			std::memset(Size == 0 ? m_list : m_data, 0xCC, stx::typelist<typeinfo>().size());

			for (const auto& type : stx::typelist<typeinfo>())
			{
				const u32 id = type.index();
				uchar* data = (Size ? +m_data : m_list) + type.pos();

				// Allocate initialization order id
				if (type.create(data, *this))
				{
					*m_order++ = data;
					*m_info++ = &type;
					m_init[id] = true;
				}
			}
		}

		auto_typemap(const auto_typemap&) = delete;

		auto_typemap& operator=(const auto_typemap&) = delete;

		~auto_typemap()
		{
			// Get actual number of created objects
			u32 _max = 0;

			for (const auto& type : stx::typelist<typeinfo>())
			{
				if (m_init[type.index()])
				{
					// Skip object if not created
					_max++;
				}
			}

			// Destroy objects in reverse order
			for (; _max; _max--)
			{
				auto* info = *--m_info;
				const u32 type_index = static_cast<const type_info<typeinfo>*>(info)->index();
				info->destroy(*--m_order);

				// Set init to false. We don't want other fxo to use this fxo in their destructor.
				m_init[type_index] = false;
			}

			// Pointers should be restored to their positions
			delete[] m_init;
			delete[] m_info;
			delete[] m_order;

			if constexpr (Size == 0)
			{
				if (stx::typelist<typeinfo>().align() > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
				{
					::operator delete[](m_list, std::align_val_t{stx::typelist<typeinfo>().align()});
				}
				else
				{
					delete[] m_list;
				}
			}
		}

		// Check if object is not initialized but shall be initialized first (to use in initializing other objects)
		template <typename T>
		void need() noexcept
		{
			if (!m_init[stx::typeindex<typeinfo, std::decay_t<T>>()])
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
			if (std::exchange(m_init[stx::typeindex<typeinfo, std::decay_t<T>, std::decay_t<As>>()], true))
			{
				// Already exists, recreation is not supported (may be added later)
				return nullptr;
			}

			As* obj = nullptr;

			if constexpr (Size != 0)
			{
				obj = new (m_data + stx::typeoffset<typeinfo, std::decay_t<T>>()) std::decay_t<As>(std::forward<Args>(args)...);
			}
			else
			{
				obj = new (m_list + stx::typeoffset<typeinfo, std::decay_t<T>>()) std::decay_t<As>(std::forward<Args>(args)...);
			}

			*m_order++ = obj;
			*m_info++ = &stx::typedata<typeinfo, std::decay_t<T>, std::decay_t<As>>();
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

		template <typename T>
		bool is_init() const noexcept
		{
			return m_init[stx::typeindex<typeinfo, std::decay_t<T>>()];
		}

		// Obtain object reference
		template <typename T>
		T& get() const noexcept
		{
			if constexpr (Size != 0)
			{
				return *std::launder(reinterpret_cast<T*>(m_data + stx::typeoffset<typeinfo, std::decay_t<T>>()));
			}
			else
			{
				return *std::launder(reinterpret_cast<T*>(m_list + stx::typeoffset<typeinfo, std::decay_t<T>>()));
			}
		}

		// Obtain object pointer if initialized
		template <typename T>
		T* try_get() const noexcept
		{
			if (is_init<T>())
			{
				[[likely]] return &get<T>();
			}

			[[unlikely]] return nullptr;
		}
	};
}

using stx::auto_typemap;
