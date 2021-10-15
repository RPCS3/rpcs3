#pragma once

// Backported from auto_typemap.hpp as a more simple alternative

#include "util/types.hpp"
#include "util/typeindices.hpp"

#include <utility>
#include <type_traits>

enum class thread_state : u32;

namespace stx
{
	// Simplified typemap with exactly one object of each used type, non-moveable. Initialized on init(). Destroyed on clear().
	template <typename Tag /*Tag should be unique*/, u32 Size = 0, u32 Align = (Size ? 64 : __STDCPP_DEFAULT_NEW_ALIGNMENT__)>
	class alignas(Align) manual_typemap
	{
		static constexpr std::string_view parse_type(std::string_view pretty_name)
		{
#ifdef _MSC_VER
			const auto pos = pretty_name.find("::typeinfo::make_typeinfo<");
			const auto end = pretty_name.rfind(">(void)");
			return pretty_name.substr(pos + 26, end - pos - 26);
#else
			const auto pos = pretty_name.find("T = ");

			if (pos + 1)
			{
				pretty_name.remove_prefix(pos + 4);

				auto end = pretty_name.find("; Tag =");

				if (end + 1)
				{
					return pretty_name.substr(0, end);
				}

				end = pretty_name.find(", Tag =");

				if (end + 1)
				{
					return pretty_name.substr(0, end);
				}

				if (!pretty_name.empty() && pretty_name.back() == ']')
				{
					pretty_name.remove_suffix(1);
				}

				return pretty_name;
			}

			return {};
#endif
		}

		// Save default constructor and destructor and optional joining operation
		struct typeinfo
		{
			bool(*create)(uchar* ptr, manual_typemap&) noexcept = nullptr;
			void(*stop)(void* ptr, thread_state) noexcept = nullptr;
			void(*destroy)(void* ptr) noexcept = nullptr;
			std::string_view name{};

			template <typename T>
			static bool call_ctor(uchar* ptr, manual_typemap& _this) noexcept
			{
				// Allow passing reference to "this"
				if constexpr (std::is_constructible_v<T, manual_typemap&>)
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

				return false;
			}

			template <typename T>
			static void call_dtor(void* ptr) noexcept
			{
				std::launder(static_cast<T*>(ptr))->~T();
			}

			template <typename T>
			static void call_stop(void* ptr, thread_state state) noexcept
			{
				// Abort and/or join (expected thread_state::aborting or thread_state::finished)
				*std::launder(static_cast<T*>(ptr)) = state;
			}

			template <typename T>
			static typeinfo make_typeinfo()
			{
				static_assert(!std::is_copy_assignable_v<T> && !std::is_copy_constructible_v<T>, "Please make sure the object cannot be accidentally copied.");

				typeinfo r;
				r.create = &call_ctor<T>;
				r.destroy = &call_dtor<T>;

				if constexpr (std::is_assignable_v<T&, thread_state>)
				{
					r.stop = &call_stop<T>;
				}

#ifdef _MSC_VER
				constexpr std::string_view name = parse_type(__FUNCSIG__);
#else
				constexpr std::string_view name = parse_type(__PRETTY_FUNCTION__);
#endif
				r.name = name;
				return r;
			}
		};

		// Objects
		union
		{
			uchar* m_list = nullptr;
			mutable uchar m_data[Size ? Size : 1];
		};

		// Creation order for each object (used to reverse destruction order)
		void** m_order = nullptr;

		// Helper for destroying in reverse order
		const typeinfo** m_info = nullptr;

		// Indicates whether object is created at given index
		bool* m_init = nullptr;

	public:
		manual_typemap() noexcept = default;

		manual_typemap(const manual_typemap&) = delete;

		manual_typemap& operator=(const manual_typemap&) = delete;

		~manual_typemap()
		{
			ensure(!m_init);
		}

		void reset()
		{
			if (m_init)
			{
				clear();
			}

			m_order = new void*[stx::typelist<typeinfo>().count() + 1];
			m_info = new const typeinfo*[stx::typelist<typeinfo>().count() + 1];
			m_init = new bool[stx::typelist<typeinfo>().count()]{};

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
				m_data[0] = 0;
			}

			*m_order++ = nullptr;
			*m_info++ = nullptr;
		}

		void init(bool reset = true)
		{
			if (reset)
			{
				this->reset();
			}

			for (const auto& type : stx::typelist<typeinfo>())
			{
				const u32 id = type.index();
				uchar* data = (Size ? +m_data : m_list) + type.pos();

				// Allocate initialization order id
				if (m_init[id])
				{
					continue;
				}

				if (type.create(data, *this))
				{
					*m_order++ = data;
					*m_info++ = &type;
					m_init[id] = true;
				}
			}
		}

		void clear()
		{
			if (!m_init)
			{
				return;
			}

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
				(*--m_info)->destroy(*--m_order);
			}

			// Pointers should be restored to their positions
			m_info--;
			m_order--;
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

			m_init = nullptr;
			m_info = nullptr;
			m_order = nullptr;

			if constexpr (Size == 0)
			{
				m_list = nullptr;
			}
		}

		// Check if object is not initialized but shall be initialized first (to use in initializing other objects)
		template <typename T>
		void need() noexcept
		{
			if (!m_init[stx::typeindex<typeinfo, std::decay_t<T>>()])
			{
				if constexpr (std::is_constructible_v<T, manual_typemap&>)
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

		// Obtain object pointer (may be uninitialized memory)
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

		class iterator
		{
			const typeinfo** m_info;
			void** m_ptr;

		public:
			iterator(const typeinfo** _info, void** _ptr)
				: m_info(_info)
				, m_ptr(_ptr)
			{
			}

			std::pair<const typeinfo&, void*> operator*() const
			{
				return {*m_info[-1], m_ptr[-1]};
			}

			iterator& operator++()
			{
				m_info--;
				m_ptr--;

				if (!m_info[-1])
				{
					m_info = nullptr;
					m_ptr = nullptr;
				}

				return *this;
			}

			bool operator!=(const iterator& rhs) const
			{
				return m_info != rhs.m_info || m_ptr != rhs.m_ptr;
			}
		};

		iterator begin() noexcept
		{
			return iterator{m_info, m_order};
		}

		iterator end() noexcept
		{
			return iterator{nullptr, nullptr};
		}
	};
}
