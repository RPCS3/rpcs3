#pragma once

// Backported from auto_typemap.hpp as a more simple alternative

#include "util/types.hpp"
#include "util/typeindices.hpp"

#include <utility>
#include <type_traits>
#include <algorithm>

enum class thread_state : u32;

extern thread_local std::string_view g_tls_serialize_name;

namespace utils
{
	struct serial;
}

namespace stx
{
	struct launch_retainer{};

	extern u16 serial_breathe_and_tag(utils::serial& ar, std::string_view name, bool tag_bit);

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
			bool(*create)(uchar* ptr, manual_typemap&, utils::serial*, std::string_view) noexcept = nullptr;
			void(*thread_op)(void* ptr, thread_state) noexcept = nullptr;
			void(*save)(void* ptr, utils::serial&) noexcept = nullptr;
			bool(*saveable)(bool) noexcept = nullptr;
			void(*destroy)(void* ptr) noexcept = nullptr;
			bool is_trivial_and_nonsavable = false;
			std::string_view name;

			template <typename T>
			static bool call_ctor(uchar* ptr, manual_typemap& _this, utils::serial* ar, std::string_view name) noexcept
			{
				if (ar)
				{
					if constexpr (std::is_constructible_v<T, exact_t<manual_typemap&>, exact_t<utils::serial&>>)
					{
						g_tls_serialize_name = name;
						new (ptr) T(exact_t<manual_typemap&>(_this), exact_t<utils::serial&>(*ar));
						return true;
					}

					if constexpr (std::is_constructible_v<T, exact_t<const launch_retainer&>, exact_t<utils::serial&>>)
					{
						g_tls_serialize_name = name;
						new (ptr) T(exact_t<const launch_retainer&>(launch_retainer{}), exact_t<utils::serial&>(*ar));
						return true;
					}

					if constexpr (std::is_constructible_v<T, exact_t<utils::serial&>>)
					{
						g_tls_serialize_name = name;
						new (ptr) T(exact_t<utils::serial&>(*ar));
						return true;
					}
				}

				// Allow passing reference to "this"
				if constexpr (std::is_constructible_v<T, exact_t<manual_typemap&>>)
				{
					new (ptr) T(exact_t<manual_typemap&>(_this));
					return true;
				}

				if constexpr (std::is_constructible_v<T, exact_t<const launch_retainer&>>)
				{
					new (ptr) T(exact_t<const launch_retainer&>(launch_retainer{}));
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
				auto* obj = std::launder(static_cast<T*>(ptr));
				obj->~T();
				std::memset(ptr, 0xCC, sizeof(T)); // Set to trap values
			}

			template <typename T>
			static void call_thread_op(void* ptr, thread_state state) noexcept
			{
				// Abort and/or join (expected thread_state::aborting or thread_state::finished)
				*std::launder(static_cast<T*>(ptr)) = state;
			}

			template <typename T>
				requires requires(T& a, utils::serial& ar) { a.save(stx::exact_t<utils::serial&>(ar)); }
			static void call_save(void* ptr, utils::serial& ar) noexcept
			{
				std::launder(static_cast<T*>(ptr))->save(stx::exact_t<utils::serial&>(ar));
			}

			template <typename T> requires requires (const T&) { T::saveable(true); }
			static bool call_saveable(bool is_writing) noexcept
			{
				return T::saveable(is_writing);
			}

			template <typename T>
			static typeinfo make_typeinfo()
			{
				static_assert(!std::is_copy_assignable_v<T> && !std::is_copy_constructible_v<T>, "Please make sure the object cannot be accidentally copied.");

				typeinfo r{};
				r.create = &call_ctor<T>;
				r.destroy = &call_dtor<T>;

				if constexpr (std::is_assignable_v<T&, thread_state>)
				{
					r.thread_op = &call_thread_op<T>;
				}

				if constexpr (!!(requires(T& a, utils::serial& ar) { a.save(stx::exact_t<utils::serial&>(ar)); }))
				{
					r.save = &call_save<T>;
				}

				if constexpr (!!(requires (const T&) { T::saveable(true); }))
				{
					r.saveable = &call_saveable<T>;
				}

				r.is_trivial_and_nonsavable = std::is_trivially_default_constructible_v<T> && !r.save;

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

		// Indicates whether object is created at given index
		std::array<bool, (Size ? Size / Align : 65536)> m_init{};

		// Creation order for each object (used to reverse destruction order)
		void** m_order = nullptr;

		// Helper for destroying in reverse order
		const typeinfo** m_info = nullptr;

	public:
		manual_typemap() noexcept = default;

		manual_typemap(const manual_typemap&) = delete;

		manual_typemap& operator=(const manual_typemap&) = delete;

		~manual_typemap()
		{
			ensure(!m_info);
		}

		void reset()
		{
			if (is_init())
			{
				clear();
			}

			m_order = new void*[stx::typelist<typeinfo>().count() + 1];
			m_info = new const typeinfo*[stx::typelist<typeinfo>().count() + 1];
			ensure(m_init.size() > stx::typelist<typeinfo>().count());

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

			*m_order++ = nullptr;
			*m_info++ = nullptr;
		}

		void init(bool reset = true, utils::serial* ar = nullptr, std::function<void()> func = {})
		{
			if (reset)
			{
				this->reset();
			}

			// Use unique_ptr to reduce header dependencies in this commonly used header
			const usz type_count = stx::typelist<typeinfo>().count();
			const auto order = std::make_unique<std::pair<double, const type_info<typeinfo>*>[]>(type_count);

			usz pos = 0;
			for (const auto& type : stx::typelist<typeinfo>())
			{
				order[pos++] = {type.init_pos(), std::addressof(type)};
			}

			std::stable_sort(order.get(), order.get() + type_count, [](const auto& a, const auto& b)
			{
				if (a.second->is_trivial_and_nonsavable && !b.second->is_trivial_and_nonsavable)
				{
					return true;
				}

				return a.first < b.first;
			});

			const auto info_before = m_info;

			for (pos = 0; pos < type_count; pos++)
			{
				const auto& type = *order[pos].second;

				const u32 id = type.index();
				uchar* data = (Size ? +m_data : m_list) + type.pos();

				// Allocate initialization order id
				if (m_init[id])
				{
					continue;
				}

				const bool saveable = !type.saveable || type.saveable(false);

				if (type.create(data, *this, saveable ? ar : nullptr, type.name))
				{
					*m_order++ = data;
					*m_info++ = &type;
					m_init[id] = true;

					if (ar && saveable && type.save)
					{
						serial_breathe_and_tag(*ar, type.name, false);
					}
				}
			}

			if (func)
			{
				func();
			}

			// Launch threads
			for (auto it = m_info; it != info_before; it--)
			{
				if (auto op = (*std::prev(it))->thread_op)
				{
					op(*std::prev(m_order, m_info - it + 1), thread_state{});
				}
			}

			g_tls_serialize_name = {};
		}

		void clear()
		{
			if (!is_init())
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

			// Order semi-destructors before the actual destructors
			// This allows to safely access data that may be deallocated or destroyed from other members of FXO regardless of their intialization time
			for (u32 i = 0; i < _max; i++)
			{
				const auto info = (*std::prev(m_info, i + 1));

				if (auto op = info->thread_op)
				{
					constexpr thread_state destroying_context{7};
					op(*std::prev(m_order, i + 1), destroying_context);
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
			m_info--;
			m_order--;
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

			m_init.fill(false);
			m_info = nullptr;
			m_order = nullptr;

			if constexpr (Size == 0)
			{
				m_list = nullptr;
			}
		}

		template <typename T> requires (std::is_same_v<T&, utils::serial&>)
		void save(T& ar)
		{
			if (!is_init())
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

			// Save data in forward order
			for (u32 i = _max; i; i--)
			{
				const auto info = (*std::prev(m_info, i));

				if (auto save = info->save)
				{
					save(*std::prev(m_order, i), ar);

					serial_breathe_and_tag(ar, info->name, false);
				}
			}
		}

		// Check if object is not initialized but shall be initialized first (to use in initializing other objects)
		template <typename T> requires (!std::is_constructible_v<T, exact_t<utils::serial&>> && (std::is_constructible_v<T, exact_t<manual_typemap&>> || std::is_default_constructible_v<T>))
		void need() noexcept
		{
			if (!m_init[stx::typeindex<typeinfo, std::decay_t<T>>()])
			{
				if constexpr (std::is_constructible_v<T, exact_t<manual_typemap&>>)
				{
					init<T>(exact_t<manual_typemap&>(*this));
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
		template <typename T, typename As = T, typename... Args> requires (std::is_constructible_v<std::decay_t<As>, Args&&...>)
		As* init(Args&&... args) noexcept
		{
			if (m_init[stx::typeindex<typeinfo, std::decay_t<T>, std::decay_t<As>>()])
			{
				// Already exists, recreation is not supported (may be added later)
				return nullptr;
			}

			m_init[stx::typeindex<typeinfo, std::decay_t<T>, std::decay_t<As>>()] = true;

			As* obj = nullptr;

			const auto type_info = &stx::typedata<typeinfo, std::decay_t<T>, std::decay_t<As>>();

			g_tls_serialize_name = get_name<T, As>();

			if constexpr (Size != 0)
			{
				obj = new (m_data + stx::typeoffset<typeinfo, std::decay_t<T>>()) std::decay_t<As>(std::forward<Args>(args)...);
			}
			else
			{
				obj = new (m_list + stx::typeoffset<typeinfo, std::decay_t<T>>()) std::decay_t<As>(std::forward<Args>(args)...);
			}

			if constexpr ((std::is_same_v<std::remove_cvref_t<Args>, utils::serial> || ...))
			{
				ensure(type_info->save);

				serial_breathe_and_tag(std::get<0>(std::tie(args...)), get_name<T, As>(), false);
			}

			if constexpr ((std::is_same_v<std::remove_cvref_t<Args>, utils::serial*> || ...))
			{
				ensure(type_info->save);

				utils::serial* ar = std::get<0>(std::tie(args...));

				if (ar)
				{
					serial_breathe_and_tag(*ar, get_name<T, As>(), false);
				}
			}

			g_tls_serialize_name = {};

			*m_order++ = obj;
			*m_info++ = type_info;
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

		bool is_init() const noexcept
		{
			return m_info != nullptr;
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

		template <typename T, typename As = T>
		static std::string_view get_name() noexcept
		{
			return stx::typedata<typeinfo, std::decay_t<T>, std::decay_t<As>>().name;
		}

		// Obtain object pointer if initialized
		template <typename T>
		T* try_get() const noexcept
		{
			if (is_init<T>())
			{
				[[likely]] return std::addressof(get<T>());
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
