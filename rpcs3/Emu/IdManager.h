#pragma once

#include "Utilities/SharedMutex.h"

#define ID_MANAGER_INCLUDED

// TODO: make id_aux_initialize and id_aux_finalize safer against a possible ODR violation

// Function called after the successfull creation of an ID (does nothing by default, provide an overload)
inline void id_aux_initialize(void*)
{
	;
}

// Function called after the ID removal (does nothing by default, provide an overload)
inline void id_aux_finalize(void*)
{
	;
}

// Type-erased id_aux_* function type
using id_aux_func_t = void(*)(void*);

template<typename T>
struct id_type_info_t
{
	static const auto size = sizeof(T); // forbid forward declarations

	static const id_aux_func_t on_remove;
};

// Type-erased finalization function
template<typename T>
const id_aux_func_t id_type_info_t<T>::on_remove = [](void* ptr)
{
	return id_aux_finalize(static_cast<T*>(ptr));
};

using id_type_index_t = const id_aux_func_t*;

// Get a unique pointer to the on_remove value (will be unique for each type)
template<typename T>
inline constexpr id_type_index_t get_id_type_index()
{
	return &id_type_info_t<T>::on_remove;
}

// Default ID traits for any arbitrary type
template<typename T>
struct id_traits
{
	static const auto size = sizeof(T); // forbid forward declarations

	// Get next mapped id (may return 0 if out of IDs)
	static u32 next_id(u32 raw_id) { return raw_id < 0x80000000 ? (raw_id + 1) & 0x7fffffff : 0; }

	// Convert "public" id to mapped id (may return 0 if invalid)
	static u32 in_id(u32 id) { return id; }

	// Convert mapped id to "public" id
	static u32 out_id(u32 raw_id) { return raw_id; }
};

// ID Manager
// 0 is invalid ID
// 1..0x7fffffff : general purpose IDs
// 0x80000000+ : reserved (may be used through id_traits specializations)
namespace idm
{
	struct id_data_t final
	{
		std::shared_ptr<void> data;
		const std::type_info* info;
		id_type_index_t type_index;

		template<typename T> id_data_t(const std::shared_ptr<T>& data)
			: data(data)
			, info(&typeid(T))
			, type_index(get_id_type_index<T>())
		{
		}
	};

	// Custom hasher for ID values (map to itself)
	struct id_hash_t final
	{
		std::size_t operator ()(u32 value) const
		{
			return value;
		}
	};

	using map_t = std::unordered_map<u32, id_data_t, id_hash_t>;

	// Can be called from the constructor called through make() or make_ptr() to get the ID of the object being created
	inline u32 get_last_id()
	{
		extern thread_local u32 g_tls_last_id;

		return g_tls_last_id;
	}

	// Remove all objects
	void clear();

	// Internal
	bool check(u32 in_id, id_type_index_t type);

	// Check if an ID of specified type exists
	template<typename T>
	bool check(u32 id)
	{
		return check(id_traits<T>::in_id(id), get_id_type_index<T>());
	}

	// Check if an ID exists and return its type or nullptr
	const std::type_info* get_type(u32 raw_id);

	// Internal
	template<typename T, typename Ptr>
	std::shared_ptr<T> add(Ptr&& get_ptr)
	{
		extern shared_mutex g_mutex;
		extern idm::map_t g_map;
		extern u32 g_last_raw_id;
		extern thread_local u32 g_tls_last_id;

		std::lock_guard<shared_mutex> lock(g_mutex);

		for (u32 raw_id = g_last_raw_id; (raw_id = id_traits<T>::next_id(raw_id)); /**/)
		{
			if (g_map.find(raw_id) != g_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			std::shared_ptr<T> ptr = get_ptr();

			g_map.emplace(raw_id, id_data_t(ptr));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return ptr;
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template<typename T, typename... Args>
	std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_ptr(Args&&... args)
	{
		if (auto ptr = add<T>(WRAP_EXPR(std::make_shared<T>(std::forward<Args>(args)...))))
		{
			id_aux_initialize(ptr.get());
			return ptr;
		}

		return nullptr;
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args>
	std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		if (auto ptr = add<T>(WRAP_EXPR(std::make_shared<T>(std::forward<Args>(args)...))))
		{
			id_aux_initialize(ptr.get());
			return get_last_id();
		}

		throw EXCEPTION("Out of IDs ('%s')", typeid(T).name());
	}

	// Add a new ID for an existing object provided (returns new id)
	template<typename T>
	u32 import(const std::shared_ptr<T>& ptr)
	{
		static const auto size = sizeof(T); // forbid forward declarations

		if (add<T>(WRAP_EXPR(ptr)))
		{
			id_aux_initialize(ptr.get());
			return get_last_id();
		}

		throw EXCEPTION("Out of IDs ('%s')", typeid(T).name());
	}

	// Internal
	std::shared_ptr<void> get(u32 in_id, id_type_index_t type);

	// Get ID of specified type
	template<typename T>
	std::shared_ptr<T> get(u32 id)
	{
		return std::static_pointer_cast<T>(get(id_traits<T>::in_id(id), get_id_type_index<T>()));
	}

	// Internal
	idm::map_t get_all(id_type_index_t type);

	// Get all IDs of specified type T (unsorted)
	template<typename T>
	std::vector<std::shared_ptr<T>> get_all()
	{
		std::vector<std::shared_ptr<T>> result;

		for (auto& id : get_all(get_id_type_index<T>()))
		{
			result.emplace_back(std::static_pointer_cast<T>(id.second.data));
		}

		return result;
	}

	std::shared_ptr<void> withdraw(u32 in_id, id_type_index_t type);

	// Remove the ID created with type T
	template<typename T>
	bool remove(u32 id)
	{
		if (auto ptr = withdraw(id_traits<T>::in_id(id), get_id_type_index<T>()))
		{
			id_aux_finalize(static_cast<T*>(ptr.get()));

			return true;
		}

		return false;
	}

	// Remove the ID created with type T and return it
	template<typename T>
	std::shared_ptr<T> withdraw(u32 id)
	{
		if (auto ptr = std::static_pointer_cast<T>(withdraw(id_traits<T>::in_id(id), get_id_type_index<T>())))
		{
			id_aux_finalize(ptr.get());

			return ptr;
		}

		return nullptr;
	}

	u32 get_count(id_type_index_t type);

	template<typename T>
	u32 get_count()
	{
		return get_count(get_id_type_index<T>());
	}

	// Get sorted list of all IDs of specified type
	template<typename T>
	std::set<u32> get_set()
	{
		std::set<u32> result;

		for (auto& id : get_all(get_id_type_index<T>()))
		{
			result.emplace(id_traits<T>::out_id(id.first));
		}

		return result;
	}

	// Get sorted map (ID value -> ID data) of all IDs of specified type
	template<typename T>
	std::map<u32, std::shared_ptr<T>> get_map()
	{
		std::map<u32, std::shared_ptr<T>> result;

		for (auto& id : get_all(get_id_type_index<T>()))
		{
			result[id_traits<T>::out_id(id.first)] = std::static_pointer_cast<T>(id.second.data);
		}

		return result;
	}
}

// Fixed Object Manager
// allows to manage shared objects of any specified type, but only one object per type;
// object are deleted when the emulation is stopped
namespace fxm
{
	// Custom hasher for aligned pointer values
	struct hash_t final
	{
		std::size_t operator()(id_type_index_t value) const
		{
			return reinterpret_cast<std::size_t>(value) >> 3;
		}
	};

	using map_t = std::unordered_map<id_type_index_t, std::shared_ptr<void>, hash_t>;

	// Remove all objects
	void clear();

	// Internal (returns old and new pointers)
	template<typename T, bool Always, typename Ptr>
	std::pair<std::shared_ptr<T>, std::shared_ptr<T>> add(Ptr&& get_ptr)
	{
		extern shared_mutex g_mutex;
		extern fxm::map_t g_map;

		std::lock_guard<shared_mutex> lock(g_mutex);

		auto& item = g_map[get_id_type_index<T>()];

		if (Always || !item)
		{
			std::shared_ptr<T> old = std::static_pointer_cast<T>(std::move(item));
			std::shared_ptr<T> ptr = get_ptr();

			// Set new object
			item = ptr;

			return{ std::move(old), std::move(ptr) };
		}
		else
		{
			return{ std::static_pointer_cast<T>(item), nullptr };
		}
	}

	// Create the object (returns nullptr if it already exists)
	template<typename T, typename... Args>
	std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		auto pair = add<T, false>(WRAP_EXPR(std::make_shared<T>(std::forward<Args>(args)...)));

		if (pair.second)
		{
			id_aux_initialize(pair.second.get());
			return std::move(pair.second);
		}

		return nullptr;
	}

	// Create the object unconditionally (old object will be removed if it exists)
	template<typename T, typename... Args>
	std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		auto pair = add<T, true>(WRAP_EXPR(std::make_shared<T>(std::forward<Args>(args)...)));

		if (pair.first)
		{
			id_aux_finalize(pair.first.get());
		}

		id_aux_initialize(pair.second.get());
		return std::move(pair.second);
	}

	// Emplace the object
	template<typename T>
	bool import(const std::shared_ptr<T>& ptr)
	{
		static const auto size = sizeof(T); // forbid forward declarations

		auto pair = add<T, false>(WRAP_EXPR(ptr));

		if (pair.second)
		{
			id_aux_initialize(pair.second.get());
			return true;
		}

		return false;
	}

	// Emplace the object unconditionally (old object will be removed if it exists)
	template<typename T>
	void import_always(const std::shared_ptr<T>& ptr)
	{
		static const auto size = sizeof(T); // forbid forward declarations

		auto pair = add<T, true>(WRAP_EXPR(ptr));

		if (pair.first)
		{
			id_aux_finalize(pair.first.get());
		}

		id_aux_initialize(pair.second.get());
	}

	// Get the object unconditionally (create an object if it doesn't exist)
	template<typename T, typename... Args>
	std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		auto pair = add<T, false>(WRAP_EXPR(std::make_shared<T>(std::forward<Args>(args)...)));

		if (pair.second)
		{
			id_aux_initialize(pair.second.get());
			return std::move(pair.second);
		}

		return std::move(pair.first);
	}

	// Internal
	bool check(id_type_index_t type);

	// Check whether the object exists
	template<typename T>
	bool check()
	{
		return check(get_id_type_index<T>());
	}

	// Internal
	std::shared_ptr<void> get(id_type_index_t type);

	// Get the object (returns nullptr if it doesn't exist)
	template<typename T>
	std::shared_ptr<T> get()
	{
		return std::static_pointer_cast<T>(get(get_id_type_index<T>()));
	}

	// Internal
	std::shared_ptr<void> withdraw(id_type_index_t type);

	// Delete the object
	template<typename T>
	bool remove()
	{
		if (auto ptr = withdraw(get_id_type_index<T>()))
		{
			id_aux_finalize(static_cast<T*>(ptr.get()));
			return true;
		}

		return false;
	}

	// Delete the object and return it
	template<typename T>
	std::shared_ptr<T> withdraw()
	{
		if (auto ptr = std::static_pointer_cast<T>(withdraw(get_id_type_index<T>())))
		{
			id_aux_finalize(ptr.get());
			return ptr;
		}
		
		return nullptr;
	}
}
