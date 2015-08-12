#pragma once

#define ID_MANAGER_INCLUDED

// default traits for any arbitrary type
template<typename T> struct id_traits
{
	// get next mapped id (may return 0 if out of IDs)
	static u32 next_id(u32 raw_id) { return raw_id < 0x80000000 ? (raw_id + 1) & 0x7fffffff : 0; }

	// convert "public" id to mapped id (may return 0 if invalid)
	static u32 in_id(u32 id) { return id; }

	// convert mapped id to "public" id
	static u32 out_id(u32 raw_id) { return raw_id; }
};

class id_data_t final
{
public:
	const std::shared_ptr<void> data;
	const std::type_info& info;
	const std::size_t hash;

	template<typename T> force_inline id_data_t(std::shared_ptr<T> data)
		: data(std::move(data))
		, info(typeid(T))
		, hash(typeid(T).hash_code())
	{
	}

	id_data_t(id_data_t&& right)
		: data(std::move(const_cast<std::shared_ptr<void>&>(right.data)))
		, info(right.info)
		, hash(right.hash)
	{
	}
};

// ID Manager
// 0 is invalid ID
// 1..0x7fffffff : general purpose IDs
// 0x80000000+ : reserved (may be used through id_traits specializations)
namespace idm
{
	// can be called from the constructor called through make() or make_ptr() to get the ID of currently created object
	inline u32 get_last_id()
	{
		thread_local extern u32 g_tls_last_id;

		return g_tls_last_id;
	}

	// reinitialize ID manager
	void clear();

	// check if ID of specified type exists
	template<typename T> bool check(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id_traits<T>::in_id(id));

		return found != g_id_map.end() && found->second.info == typeid(T);
	}

	// check if ID exists and return its type or nullptr
	inline const std::type_info* get_type(u32 raw_id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(raw_id);

		return found == g_id_map.end() ? nullptr : &found->second.info;
	}

	// add new ID of specified type with specified constructor arguments (returns object or nullptr)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_ptr(Args&&... args)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;
		extern u32 g_last_raw_id;
		thread_local extern u32 g_tls_last_id;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		u32 raw_id = g_last_raw_id;

		while ((raw_id = id_traits<T>::next_id(raw_id)))
		{
			if (g_id_map.find(raw_id) != g_id_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_id_map.emplace(raw_id, id_data_t(ptr));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return ptr;
		}

		return nullptr;
	}

	// add new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;
		extern u32 g_last_raw_id;
		thread_local extern u32 g_tls_last_id;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		u32 raw_id = g_last_raw_id;

		while ((raw_id = id_traits<T>::next_id(raw_id)))
		{
			if (g_id_map.find(raw_id) != g_id_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			g_id_map.emplace(raw_id, id_data_t(std::make_shared<T>(std::forward<Args>(args)...)));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return id_traits<T>::out_id(raw_id);
		}

		throw EXCEPTION("Out of IDs");
	}

	// add new ID for an existing object provided (don't use for initial object creation)
	template<typename T> u32 import(const std::shared_ptr<T>& ptr)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;
		extern u32 g_last_raw_id;
		thread_local extern u32 g_tls_last_id;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		u32 raw_id = g_last_raw_id;

		while ((raw_id = id_traits<T>::next_id(raw_id)))
		{
			if (g_id_map.find(raw_id) != g_id_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			g_id_map.emplace(raw_id, id_data_t(ptr));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return id_traits<T>::out_id(raw_id);
		}

		throw EXCEPTION("Out of IDs");
	}

	// get ID of specified type
	template<typename T> std::shared_ptr<T> get(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id_traits<T>::in_id(id));

		if (found == g_id_map.end() || found->second.info != typeid(T))
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second.data);
	}

	// get all IDs of specified type T (unsorted)
	template<typename T> std::vector<std::shared_ptr<T>> get_all()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		std::vector<std::shared_ptr<T>> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result.emplace_back(std::static_pointer_cast<T>(v.second.data));
			}
		}

		return result;
	}

	// remove ID created with type T
	template<typename T> bool remove(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id_traits<T>::in_id(id));

		if (found == g_id_map.end() || found->second.info != typeid(T))
		{
			return false;
		}

		g_id_map.erase(found);

		return true;
	}

	// remove ID created with type T and return the object
	template<typename T> std::shared_ptr<T> withdraw(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id_traits<T>::in_id(id));

		if (found == g_id_map.end() || found->second.info != typeid(T))
		{
			return nullptr;
		}

		auto ptr = std::static_pointer_cast<T>(found->second.data);

		g_id_map.erase(found);

		return ptr;
	}

	template<typename T> u32 get_count()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		u32 result = 0;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result++;
			}
		}

		return result;
	}

	// get sorted ID list of specified type
	template<typename T> std::set<u32> get_set()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		std::set<u32> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result.insert(id_traits<T>::out_id(v.first));
			}
		}

		return result;
	}

	// get sorted ID map (ID value -> ID data) of specified type
	template<typename T> std::map<u32, std::shared_ptr<T>> get_map()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, id_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		std::map<u32, std::shared_ptr<T>> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result[id_traits<T>::out_id(v.first)] = std::static_pointer_cast<T>(v.second.data);
			}
		}

		return result;
	}
}

// Fixed Object Manager
// allows to manage shared objects of any specified type, but only one object per type;
// object are deleted when the emulation is stopped
namespace fxm
{
	// reinitialize
	void clear();

	// add fixed object of specified type only if it doesn't exist (one unique object per type may exist)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		const auto found = g_fx_map.find(typeid(T));

		// only if object of this type doesn't exist
		if (found == g_fx_map.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_fx_map.emplace(typeid(T), ptr);

			return std::move(ptr);
		}

		return nullptr;
	}

	// add fixed object of specified type, replacing previous one if it exists
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		g_fx_map[typeid(T)] = ptr;

		return ptr;
	}

	// get fixed object of specified type (always returns an object, it's created if it doesn't exist)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		const auto found = g_fx_map.find(typeid(T));

		if (found == g_fx_map.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_fx_map[typeid(T)] = ptr;

			return ptr;
		}

		return std::static_pointer_cast<T>(found->second);
	}

	// check whether the object exists
	template<typename T> bool check()
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		return g_fx_map.find(typeid(T)) != g_fx_map.end();
	}

	// get fixed object of specified type (returns nullptr if it doesn't exist)
	template<typename T> std::shared_ptr<T> get()
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		const auto found = g_fx_map.find(typeid(T));

		if (found == g_fx_map.end())
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second);
	}

	// remove fixed object created with type T
	template<typename T> bool remove()
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		const auto found = g_fx_map.find(typeid(T));

		if (found == g_fx_map.end())
		{
			return false;
		}

		return g_fx_map.erase(found), true;
	}

	// remove fixed object created with type T and return it
	template<typename T> std::shared_ptr<T> withdraw()
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		const auto found = g_fx_map.find(typeid(T));

		if (found == g_fx_map.end())
		{
			return nullptr;
		}

		auto ptr = std::static_pointer_cast<T>(std::move(found->second));

		return g_fx_map.erase(found), ptr;
	}
}
