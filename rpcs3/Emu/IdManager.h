#pragma once

#define ID_MANAGER_INCLUDED

template<typename T> struct type_info_t { static char value; };

template<typename T> char type_info_t<T>::value = 42;

template<typename T> constexpr inline const void* get_type_index()
{
	return &type_info_t<T>::value;
}

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

struct id_data_t final
{
	std::shared_ptr<void> data;
	const std::type_info* info;
	const void* type_index;

	template<typename T> inline id_data_t(std::shared_ptr<T> data)
		: data(std::move(data))
		, info(&typeid(T))
		, type_index(get_type_index<T>())
	{
	}
};

// ID Manager
// 0 is invalid ID
// 1..0x7fffffff : general purpose IDs
// 0x80000000+ : reserved (may be used through id_traits specializations)
namespace idm
{
	extern std::mutex g_mutex;

	extern std::unordered_map<u32, id_data_t> g_map;

	extern u32 g_last_raw_id;

	thread_local extern u32 g_tls_last_id;

	// can be called from the constructor called through make() or make_ptr() to get the ID of the object being created
	inline static u32 get_last_id()
	{
		return g_tls_last_id;
	}

	// reinitialize ID manager
	static void clear()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		g_map.clear();
		g_last_raw_id = 0;
	}

	// check if ID of specified type exists
	template<typename T> static bool check(u32 id)
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		
		const auto found = g_map.find(id_traits<T>::in_id(id));

		return found != g_map.end() && found->second.type_index == get_type_index<T>();
	}

	// check if ID exists and return its type or nullptr
	inline static const std::type_info* get_type(u32 raw_id)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(raw_id);

		return found == g_map.end() ? nullptr : found->second.info;
	}

	// add new ID of specified type with specified constructor arguments (returns object or nullptr)
	template<typename T, typename... Args> static std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_ptr(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		for (u32 raw_id = g_last_raw_id; (raw_id = id_traits<T>::next_id(raw_id)); /**/)
		{
			if (g_map.find(raw_id) != g_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_map.emplace(raw_id, id_data_t(ptr));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return ptr;
		}

		return nullptr;
	}

	// add new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args> static std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		for (u32 raw_id = g_last_raw_id; (raw_id = id_traits<T>::next_id(raw_id)); /**/)
		{
			if (g_map.find(raw_id) != g_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			g_map.emplace(raw_id, id_data_t(std::make_shared<T>(std::forward<Args>(args)...)));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return id_traits<T>::out_id(raw_id);
		}

		throw EXCEPTION("Out of IDs");
	}

	// add new ID for an existing object provided (don't use for initial object creation)
	template<typename T> static u32 import(const std::shared_ptr<T>& ptr)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		for (u32 raw_id = g_last_raw_id; (raw_id = id_traits<T>::next_id(raw_id)); /**/)
		{
			if (g_map.find(raw_id) != g_map.end()) continue;

			g_tls_last_id = id_traits<T>::out_id(raw_id);

			g_map.emplace(raw_id, id_data_t(ptr));

			if (raw_id < 0x80000000) g_last_raw_id = raw_id;

			return id_traits<T>::out_id(raw_id);
		}

		throw EXCEPTION("Out of IDs");
	}

	// get ID of specified type
	template<typename T> static std::shared_ptr<T> get(u32 id)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(id_traits<T>::in_id(id));

		if (found == g_map.end() || found->second.type_index != get_type_index<T>())
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second.data);
	}

	// get all IDs of specified type T (unsorted)
	template<typename T> static std::vector<std::shared_ptr<T>> get_all()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		std::vector<std::shared_ptr<T>> result;

		const auto type = get_type_index<T>();

		for (auto& v : g_map)
		{
			if (v.second.type_index == type)
			{
				result.emplace_back(std::static_pointer_cast<T>(v.second.data));
			}
		}

		return result;
	}

	// remove ID created with type T
	template<typename T> static bool remove(u32 id)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(id_traits<T>::in_id(id));

		if (found == g_map.end() || found->second.type_index != get_type_index<T>())
		{
			return false;
		}

		g_map.erase(found);

		return true;
	}

	// remove ID created with type T and return the object
	template<typename T> static std::shared_ptr<T> withdraw(u32 id)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(id_traits<T>::in_id(id));

		if (found == g_map.end() || found->second.type_index != get_type_index<T>())
		{
			return nullptr;
		}

		auto ptr = std::static_pointer_cast<T>(found->second.data);

		g_map.erase(found);

		return ptr;
	}

	template<typename T> static u32 get_count()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		u32 result = 0;

		const auto type = get_type_index<T>();

		for (auto& v : g_map)
		{
			if (v.second.type_index == type)
			{
				result++;
			}
		}

		return result;
	}

	// get sorted ID list of specified type
	template<typename T> static std::set<u32> get_set()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		std::set<u32> result;

		const auto type = get_type_index<T>();

		for (auto& v : g_map)
		{
			if (v.second.type_index == type)
			{
				result.insert(id_traits<T>::out_id(v.first));
			}
		}

		return result;
	}

	// get sorted ID map (ID value -> ID data) of specified type
	template<typename T> static std::map<u32, std::shared_ptr<T>> get_map()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		std::map<u32, std::shared_ptr<T>> result;

		const auto type = get_type_index<T>();

		for (auto& v : g_map)
		{
			if (v.second.type_index == type)
			{
				result[id_traits<T>::out_id(v.first)] = std::static_pointer_cast<T>(v.second.data);
			}
		}

		return result;
	}
};

// Fixed Object Manager
// allows to manage shared objects of any specified type, but only one object per type;
// object are deleted when the emulation is stopped
namespace fxm
{
	extern std::mutex g_mutex;

	extern std::unordered_map<const void*, std::shared_ptr<void>> g_map;

	// reinitialize
	static void clear()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		g_map.clear();
	}

	// add fixed object of specified type only if it doesn't exist (one unique object per type may exist)
	template<typename T, typename... Args> static std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto index = get_type_index<T>();

		const auto found = g_map.find(index);

		// only if object of this type doesn't exist
		if (found == g_map.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_map.emplace(index, ptr);

			return ptr;
		}

		return nullptr;
	}

	// add fixed object of specified type, replacing previous one if it exists
	template<typename T, typename... Args> static std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		g_map[get_type_index<T>()] = ptr;

		return ptr;
	}

	// import existing fixed object of specified type only if it doesn't exist (don't use)
	template<typename T> static std::shared_ptr<T> import(std::shared_ptr<T>&& ptr)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto index = get_type_index<T>();

		const auto found = g_map.find(index);

		if (found == g_map.end())
		{
			g_map.emplace(index, ptr);

			return ptr;
		}

		return nullptr;
	}

	// import existing fixed object of specified type, replacing previous one if it exists (don't use)
	template<typename T> static std::shared_ptr<T> import_always(std::shared_ptr<T>&& ptr)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		g_map[get_type_index<T>()] = ptr;

		return ptr;
	}

	// get fixed object of specified type (always returns an object, it's created if it doesn't exist)
	template<typename T, typename... Args> static std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto index = get_type_index<T>();

		const auto found = g_map.find(index);

		if (found == g_map.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_map[index] = ptr;

			return ptr;
		}

		return std::static_pointer_cast<T>(found->second);
	}

	// check whether the object exists
	template<typename T> static bool check()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		return g_map.find(get_type_index<T>()) != g_map.end();
	}

	// get fixed object of specified type (returns nullptr if it doesn't exist)
	template<typename T> static std::shared_ptr<T> get()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(get_type_index<T>());

		if (found == g_map.end())
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second);
	}

	// remove fixed object created with type T
	template<typename T> static bool remove()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(get_type_index<T>());

		if (found == g_map.end())
		{
			return false;
		}

		return g_map.erase(found), true;
	}

	// remove fixed object created with type T and return it
	template<typename T> static std::shared_ptr<T> withdraw()
	{
		std::lock_guard<std::mutex> lock(g_mutex);

		const auto found = g_map.find(get_type_index<T>());

		if (found == g_map.end())
		{
			return nullptr;
		}

		auto ptr = std::static_pointer_cast<T>(std::move(found->second));

		return g_map.erase(found), ptr;
	}
};
