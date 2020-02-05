#pragma once

#include "Utilities/types.h"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>

// Helper namespace
namespace id_manager
{
	// Common global mutex
	extern shared_mutex g_mutex;

	// ID traits
	template <typename T, typename = void>
	struct id_traits
	{
		static_assert(sizeof(T) == 0, "ID object must specify: id_base, id_step, id_count");

		static const u32 base    = 1;     // First ID (N = 0)
		static const u32 step    = 1;     // Any ID: N * id_step + id_base
		static const u32 count   = 65535; // Limit: N < id_count
		static const u32 invalid = 0;
	};

	template <typename T>
	struct id_traits<T, std::void_t<decltype(&T::id_base), decltype(&T::id_step), decltype(&T::id_count)>>
	{
		static const u32 base    = T::id_base;
		static const u32 step    = T::id_step;
		static const u32 count   = T::id_count;
		static const u32 invalid = -+!base;

		// Note: full 32 bits range cannot be used at current implementation
		static_assert(count > 0 && step > 0 && u64{step} * count + base < u64{UINT32_MAX} + (base != 0 ? 1 : 0), "ID traits: invalid object range");
	};

	// Correct usage testing
	template <typename T, typename T2, typename = void>
	struct id_verify : std::integral_constant<bool, std::is_base_of<T, T2>::value>
	{
		// If common case, T2 shall be derived from or equal to T
	};

	template <typename T, typename T2>
	struct id_verify<T, T2, std::void_t<typename T2::id_type>> : std::integral_constant<bool, std::is_same<T, typename T2::id_type>::value>
	{
		// If T2 contains id_type type, T must be equal to it
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

	using id_map = std::vector<std::pair<id_key, std::shared_ptr<void>>>;
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	// Last allocated ID for constructors
	static thread_local u32 g_id;

	// Type Index -> ID -> Object. Use global since only one process is supported atm.
	static std::vector<id_manager::id_map> g_map;

	template <typename T>
	static inline u32 get_type()
	{
		return id_manager::typeinfo::get_index<T>();
	}

	template <typename T>
	static constexpr u32 get_index(u32 id)
	{
		using traits = id_manager::id_traits<T>;

		// Note: if id is lower than base, diff / step will be higher than count
		u32 diff = id - traits::base;

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

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using void_type   = void;
	};

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using void_type   = void;
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

		T* operator->() const
		{
			return ptr;
		}
	};

	// Prepare new ID (returns nullptr if out of resources)
	static id_manager::id_map::pointer allocate_id(const id_manager::id_key& info, u32 base, u32 step, u32 count);

	// Find ID (additionally check type if types are not equal)
	template <typename T, typename Type>
	static id_manager::id_map::pointer find_id(u32 id)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		const u32 index = get_index<Type>(id);

		if (index >= id_manager::id_traits<Type>::count)
		{
			return nullptr;
		}

		auto& vec = g_map[get_type<T>()];

		if (index >= vec.size())
		{
			return nullptr;
		}

		auto& data = vec[index];

		if (data.second)
		{
			if (std::is_same<T, Type>::value || data.first.type() == get_type<Type>())
			{
				return &data;
			}
		}

		return nullptr;
	}

	// Allocate new ID and assign the object from the provider()
	template <typename T, typename Type, typename F>
	static id_manager::id_map::pointer create_id(F&& provider)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		// ID info
		const id_manager::id_key info{get_type<T>(), get_type<Type>()};

		// ID traits
		using traits = id_manager::id_traits<Type>;

		// Allocate new id
		std::lock_guard lock(id_manager::g_mutex);

		if (auto* place = allocate_id(info, traits::base, traits::step, traits::count))
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
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Get last ID (updated in create_id/allocate_id)
	static inline u32 last_id()
	{
		return g_id;
	}

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<Make>> make_ptr(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return {pair->second, static_cast<Make*>(pair->second.get())};
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, u32> make(Args&&... args)
	{
		if (auto pair = create_id<T, Make>([&] { return std::make_shared<Make>(std::forward<Args>(args)...); }))
		{
			return pair->first;
		}

		return id_manager::id_traits<Make>::invalid;
	}

	// Add a new ID for an existing object provided (returns new id)
	template <typename T, typename Made = T>
	static inline u32 import_existing(const std::shared_ptr<T>& ptr)
	{
		if (auto pair = create_id<T, Made>([&] { return ptr; }))
		{
			return pair->first;
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Add a new ID for an object returned by provider()
	template <typename T, typename Made = T, typename F, typename = std::invoke_result_t<F>>
	static inline u32 import(F&& provider)
	{
		if (auto pair = create_id<T, Made>(std::forward<F>(provider)))
		{
			return pair->first;
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Access the ID record without locking (unsafe)
	template <typename T, typename Get = T>
	static inline id_manager::id_map::pointer find_unlocked(u32 id)
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

		return {found->second, static_cast<Get*>(found->second.get())};
	}

	// Get the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> get(u32 id)
	{
		reader_lock lock(id_manager::g_mutex);

		const auto found = find_id<T, Get>(id);

		if (found == nullptr) [[unlikely]]
		{
			return nullptr;
		}

		return {found->second, static_cast<Get*>(found->second.get())};
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

	// Access all objects of specified type. Returns the number of objects processed.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::void_type>
	static inline u32 select(F&& func, int = 0)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		reader_lock lock(id_manager::g_mutex);

		u32 result = 0;

		for (auto& id : g_map[get_type<T>()])
		{
			if (id.second)
			{
				if (std::is_same<T, Get>::value || id.first.type() == get_type<Get>())
				{
					func(id.first, *static_cast<typename function_traits<FT>::object_type*>(id.second.get()));
					result++;
				}
			}
		}

		return result;
	}

	// Access all objects of specified type. If function result evaluates to true, stop and return the object and the value.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::result_type>
	static inline auto select(F&& func)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		using object_type = typename function_traits<FT>::object_type;
		using result_type = return_pair<object_type, FRT>;

		reader_lock lock(id_manager::g_mutex);

		for (auto& id : g_map[get_type<T>()])
		{
			if (auto ptr = static_cast<object_type*>(id.second.get()))
			{
				if (std::is_same<T, Get>::value || id.first.type() == get_type<Get>())
				{
					if (FRT result = func(id.first, *ptr))
					{
						return result_type{{id.second, ptr}, std::move(result)};
					}
				}
			}
		}

		return result_type{nullptr};
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

	// Remove the ID and return the object
	template <typename T, typename Get = T>
	static inline std::shared_ptr<Get> withdraw(u32 id)
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
				return nullptr;
			}
		}

		return {ptr, static_cast<Get*>(ptr.get())};
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
				std::shared_ptr<void> ptr = std::move(found->second);
				return {ptr, static_cast<Get*>(ptr.get())};
			}
			else
			{
				FRT ret = func(*_ptr);

				if (ret)
				{
					// If return value evaluates to true, don't delete the object (error code)
					return {{found->second, _ptr}, std::move(ret)};
				}

				std::shared_ptr<void> ptr = std::move(found->second);
				return {{ptr, static_cast<Get*>(ptr.get())}, std::move(ret)};
			}
		}

		return {nullptr};
	}
};

#include "util/fixed_typemap.hpp"

extern stx::manual_fixed_typemap<void> g_fixed_typemap;

constexpr stx::manual_fixed_typemap<void>* g_fxo = &g_fixed_typemap;
