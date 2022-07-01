#pragma once

#include "util/types.hpp"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>

#include "util/fixed_typemap.hpp"

extern stx::manual_typemap<void, 0x20'00000, 128> g_fixed_typemap;

constexpr auto* g_fxo = &g_fixed_typemap;

enum class thread_state : u32;

// Helper namespace
namespace id_manager
{
	// Common global mutex
	extern shared_mutex g_mutex;

	template <typename T>
	constexpr std::pair<u32, u32> get_invl_range()
	{
		return {0, 0};
	}

	template <typename T> requires requires () { T::id_invl_range; }
	constexpr std::pair<u32, u32> get_invl_range()
	{
		return T::id_invl_range;
	}

	template <typename T>
	concept IdmCompatible = requires () { T::id_base, T::id_step, T::id_count; };

	// ID traits
	template <typename T>
	struct id_traits
	{
		static_assert(IdmCompatible<T>, "ID object must specify: id_base, id_step, id_count");

		enum : u32
		{
			base = T::id_base, // First ID (N = 0)
			step = T::id_step, // Any ID: N * id_setp + id_base
			count = T::id_count, // Limit: N < id_count
			invalid = -+!base, // Invalid ID sample
		};

		static constexpr std::pair<u32, u32> invl_range = get_invl_range<T>();

		static_assert(count && step && u64{step} * (count - 1) + base < u32{umax} + u64{base != 0 ? 1 : 0}, "ID traits: invalid object range");

		// TODO: Add more conditions
		static_assert(!invl_range.second || (u64{invl_range.second} + invl_range.first <= 32 /*....*/ ));
	};

	class typeinfo
	{
		// Global variable for each registered type
		template <typename T>
		struct registered
		{
			static const u32 index;
		};

		// Increment type counter
		static u32 add_type(u32 i)
		{
			static atomic_t<u32> g_next{0};

			return g_next.fetch_add(i);
		}

	public:
		// Get type index
		template <typename T>
		static inline u32 get_index()
		{
			return registered<T>::index;
		}

		// Get type count
		static inline u32 get_count()
		{
			return add_type(0);
		}
	};

	template <typename T>
	const u32 typeinfo::registered<T>::index = typeinfo::add_type(1);

	// ID value with additional type stored
	class id_key
	{
		u32 m_value;           // ID value
		u32 m_type;            // True object type

	public:
		id_key() = default;

		id_key(u32 value, u32 type)
			: m_value(value)
			, m_type(type)
		{
		}

		u32 value() const
		{
			return m_value;
		}

		u32 type() const
		{
			return m_type;
		}

		operator u32() const
		{
			return m_value;
		}
	};

	template <typename T>
	struct id_map
	{
		std::vector<std::pair<id_key, std::shared_ptr<void>>> vec{}, private_copy{};
		shared_mutex mutex{}; // TODO: Use this instead of global mutex

		id_map()
		{
			// Preallocate memory
			vec.reserve(T::id_count);
		}

		template <bool dummy = false> requires (std::is_assignable_v<T&, thread_state>)
		id_map& operator=(thread_state state)
		{
			if (private_copy.empty())
			{
				reader_lock lock(g_mutex);

				// Save all entries
				private_copy = vec;
			}

			// Signal or join threads
			for (const auto& [key, ptr] : private_copy)
			{
				if (ptr)
				{
					*static_cast<T*>(ptr.get()) = state;
				}
			}

			return *this;
		}
	};
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Last allocated ID for constructors
	static thread_local u32 g_id;

	template <typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

	template <typename T>
	static constexpr u32 get_index(u32 id)
	{
		using traits = id_manager::id_traits<T>;

		constexpr u32 mask_out = ((1u << traits::invl_range.second) - 1) << traits::invl_range.first;

		// Note: if id is lower than base, diff / step will be higher than count
		u32 diff = (id & ~mask_out) - traits::base;

		if (diff % traits::step)
		{
			// id is invalid, return invalid index
			return traits::count;
		}

		// Get actual index
		return diff / traits::step;
	}

	// Helper
	template <typename F>
	struct function_traits;

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using result_type = R;
	};

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using result_type = R;
	};

	// Helper type: pointer + return value propagated
	template <typename T, typename RT>
	struct return_pair
	{
		std::shared_ptr<T> ptr;
		RT ret;

		explicit operator bool() const
		{
			return ptr.operator bool();
		}

		T& operator*() const
		{
			return *ptr;
		}

		T* operator->() const
		{
			return ptr.get();
		}
	};

	// Unsafe specialization (not refcounted)
	template <typename T, typename RT>
	struct return_pair<T*, RT>
	{
		T* ptr;
		RT ret;

		explicit operator bool() const
		{
			return ptr != nullptr;
		}

		T& operator*() const
		{
			return *ptr;
		}

		T* operator->() const
		{
			return ptr;
		}
	};

	using map_data = std::pair<id_manager::id_key, std::shared_ptr<void>>;

	// Prepare new ID (returns nullptr if out of resources)
	static map_data* allocate_id(std::vector<map_data>& vec, u32 type_id, u32 base, u32 step, u32 count, std::pair<u32, u32> invl_range);

	// Find ID (additionally check type if types are not equal)
	template <typename T, typename Type>
	static map_data* find_id(u32 id)
	{
		static_assert(PtrSame<T, Type>, "Invalid ID type combination");

		const u32 index = get_index<Type>(id);

		if (index >= id_manager::id_traits<Type>::count)
		{
			return nullptr;
		}

		auto& vec = g_fxo->get<id_manager::id_map<T>>().vec;

		if (index >= vec.size())
		{
			return nullptr;
		}

		auto& data = vec[index];

		if (data.second)
		{
			if (std::is_same<T, Type>::value || data.first.type() == get_type<Type>())
			{
				if (!id_manager::id_traits<Type>::invl_range.second || data.first.value() == id)
				{
					return &data;
				}
			}
		}

		return nullptr;
	}

	// Allocate new ID and assign the object from the provider()
	template <typename T, typename Type, typename F>
	static map_data* create_id(F&& provider)
	{
		static_assert(PtrSame<T, Type>, "Invalid ID type combination");

		// ID traits
		using traits = id_manager::id_traits<Type>;

		// Allocate new id
		std::lock_guard lock(id_manager::g_mutex);

		auto& map = g_fxo->get<id_manager::id_map<T>>();

		if (auto* place = allocate_id(map.vec, get_type<Type>(), traits::base, traits::step, traits::count, traits::invl_range))
		{
			// Get object, store it
			place->second = provider();

			if (place->second)
			{
				return place;
			}
		}

		return nullptr;
	}

public:

	// Remove all objects of a type
	template <typename T>
	static inline void clear()
	{
		std::lock_guard lock(id_manager::g_mutex);
		g_fxo->get<id_manager::id_map<T>>().vec.clear();
	}

	// Get last ID (updated in create_id/allocate_id)
	static inline u32 last_id()
	{
		return g_id;
	}

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template <typename T, typename Make = T, typename... Args> requires (std::is_constructible_v<Make, Args&&...>)
	static inline std::shared_ptr<Make> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return {pair->second, static_cast<Make*>(pair->second.get())};
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template <typename T, typename Make = T, typename... Args> requires (std::is_constructible_v<Make, Args&&...>)
	static inline u32 make(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return pair->first;
		}

		return id_manager::id_traits<Make>::invalid;
	}

	// Add a new ID for an object returned by provider()
	template <typename T, typename Made = T, typename F> requires (std::is_invocable_v<F&&>)
	static inline u32 import(F&& provider)
	{
		if (auto pair = create_id<T, Made>(std::forward<F>(provider)))
		{
			return pair->first;
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Add a new ID for an existing object provided (returns new id)
	template <typename T, typename Made = T>
	static inline u32 import_existing(std::shared_ptr<T> ptr)
	{
		return import<T, Made>([&] { return std::move(ptr); });
	}

	// Access the ID record without locking (unsafe)
	template <typename T, typename Get = T>
	static inline map_data* find_unlocked(u32 id)
	{
		return find_id<T, Get>(id);
	}

	// Check the ID without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline Get* check_unlocked(u32 id)
	{
		if (const auto found = find_id<T, Get>(id))
		{
			return static_cast<Get*>(found->second.get());
		}

		return nullptr;
	}

	// Check the ID
	template <typename T, typename Get = T>
	static inline Get* check(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		return check_unlocked<T, Get>(id);
	}

	// Check the ID, access object under shared lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline auto check(u32 id, F&& func)
	{
		reader_lock lock(id_manager::g_mutex);

		if (const auto ptr = check_unlocked<T, Get>(id))
		{
			if constexpr (!std::is_void_v<FRT>)
			{
				return return_pair<Get*, FRT>{ptr, func(*ptr)};
			}
			else
			{
				func(*ptr);
				return ptr;
			}
		}

		if constexpr (!std::is_void_v<FRT>)
		{
			return return_pair<Get*, FRT>{nullptr};
		}
		else
		{
			return static_cast<Get*>(nullptr);
		}
	}

	// Get the object without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> get_unlocked(u32 id)
	{
		const auto found = find_id<T, Get>(id);

		if (found == nullptr) [[unlikely]]
		{
			return nullptr;
		}

		return std::static_pointer_cast<Get>(found->second);
	}

	// Get the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> get(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		return get_unlocked<T, Get>(id);
	}

	// Get the object, access object under reader lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline std::conditional_t<std::is_void_v<FRT>, std::shared_ptr<Get>, return_pair<Get, FRT>> get(u32 id, F&& func)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id<T, Get>(id);

		if (found == nullptr) [[unlikely]]
		{
			return {nullptr};
		}

		const auto ptr = static_cast<Get*>(found->second.get());

		if constexpr (std::is_void_v<FRT>)
		{
			func(*ptr);
			return {found->second, ptr};
		}
		else
		{
			return {{found->second, ptr}, func(*ptr)};
		}
	}

	static constexpr std::false_type unlocked{};

	// Access all objects of specified type. Returns the number of objects processed.
	// If function result evaluates to true, stop and return the object and the value.
	template <typename T, typename... Get, typename F, typename Lock = std::true_type>
	static inline auto select(F&& func, Lock = {})
	{
		static_assert((PtrSame<T, Get> && ...), "Invalid ID type combination");

		std::conditional_t<static_cast<bool>(Lock()), reader_lock, const shared_mutex&> lock(id_manager::g_mutex);

		using func_traits = function_traits<decltype(&decltype(std::function(std::declval<F>()))::operator())>;
		using object_type = typename func_traits::object_type;
		using result_type = typename func_traits::result_type;

		static_assert(PtrSame<object_type, T>, "Invalid function argument type combination");

		std::conditional_t<std::is_void_v<result_type>, u32, return_pair<object_type, result_type>> result{};

		for (auto& id : g_fxo->get<id_manager::id_map<T>>().vec)
		{
			if (auto ptr = static_cast<object_type*>(id.second.get()))
			{
				if (sizeof...(Get) == 0 || ((id.first.type() == get_type<Get>()) || ...))
				{
					if constexpr (std::is_void_v<result_type>)
					{
						func(id.first, *ptr);
						result++;
					}
					else if ((result.ret = func(id.first, *ptr)))
					{
						result.ptr = {id.second, ptr};
						break;
					}
				}
			}
		}

		return result;
	}

	// Remove the ID
	template <typename T, typename Get = T>
	static inline bool remove(u32 id)
	{
		std::shared_ptr<void> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				ptr = std::move(found->second);
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	// Remove the ID if matches the weak/shared ptr
	template <typename T, typename Get = T, typename Ptr>
	static inline bool remove_verify(u32 id, Ptr sptr)
	{
		std::shared_ptr<void> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id); found &&
				(!found->second.owner_before(sptr) && !sptr.owner_before(found->second)))
			{
				ptr = std::move(found->second);
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	// Remove the ID and return the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> withdraw(u32 id)
	{
		std::shared_ptr<Get> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			if (const auto found = find_id<T, Get>(id))
			{
				ptr = std::static_pointer_cast<Get>(::as_rvalue(std::move(found->second)));
			}
		}

		return ptr;
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline std::conditional_t<std::is_void_v<FRT>, std::shared_ptr<Get>, return_pair<Get, FRT>> withdraw(u32 id, F&& func)
	{
		std::unique_lock lock(id_manager::g_mutex);

		if (const auto found = find_id<T, Get>(id))
		{
			const auto _ptr = static_cast<Get*>(found->second.get());

			if constexpr (std::is_void_v<FRT>)
			{
				func(*_ptr);
				return std::static_pointer_cast<Get>(::as_rvalue(std::move(found->second)));
			}
			else
			{
				FRT ret = func(*_ptr);

				if (ret)
				{
					// If return value evaluates to true, don't delete the object (error code)
					return {{found->second, _ptr}, std::move(ret)};
				}

				return {std::static_pointer_cast<Get>(::as_rvalue(std::move(found->second))), std::move(ret)};
			}
		}

		return {nullptr};
	}
};
