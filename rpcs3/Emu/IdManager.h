#pragma once

#define ID_MANAGER_INCLUDED

class ID_data_t final
{
public:
	const std::shared_ptr<void> data;
	const std::type_info& info;
	const std::size_t hash;

	template<typename T> force_inline ID_data_t(std::shared_ptr<T> data)
		: data(std::move(data))
		, info(typeid(T))
		, hash(typeid(T).hash_code())
	{
	}

	ID_data_t(ID_data_t&& right)
		: data(std::move(const_cast<std::shared_ptr<void>&>(right.data)))
		, info(right.info)
		, hash(right.hash)
	{
	}
};

// ID Manager
// 0 is invalid ID
// 1..0x7fffffff : general purpose IDs
// 0x80000000+ : reserved
namespace idm
{
	// reinitialize ID manager
	void clear();

	// check if ID exists
	template<typename T> bool check(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto f = g_id_map.find(id);

		return f != g_id_map.end() && f->second.info == typeid(T);
	}

	// must be called from the constructor called through make() or make_ptr() to get further ID of current object
	inline u32 get_current_id()
	{
		// contains the next ID or 0x80000000 | current_ID
		extern u32 g_cur_id;

		if ((g_cur_id & 0x80000000) == 0)
		{
			throw EXCEPTION("Current ID is not available");
		}

		return g_cur_id & 0x7fffffff;
	}

	// add new ID of specified type with specified constructor arguments (returns object)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> make_ptr(Args&&... args)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;
		extern u32 g_cur_id;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		g_cur_id |= 0x80000000;

		if (const u32 id = g_cur_id & 0x7fffffff)
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			g_id_map.emplace(id, ID_data_t(ptr));

			g_cur_id = id + 1;

			return std::move(ptr);
		}

		throw EXCEPTION("Out of IDs");
	}

	// add new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;
		extern u32 g_cur_id;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		g_cur_id |= 0x80000000;

		if (const u32 id = g_cur_id & 0x7fffffff)
		{
			g_id_map.emplace(id, ID_data_t(std::make_shared<T>(std::forward<Args>(args)...)));

			g_cur_id = id + 1;

			return id;
		}

		throw EXCEPTION("Out of IDs");
	}

	// get ID of specified type
	template<typename T> std::shared_ptr<T> get(u32 id)
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id);

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
		extern std::unordered_map<u32, ID_data_t> g_id_map;

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
		extern std::unordered_map<u32, ID_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		const auto found = g_id_map.find(id);

		if (found == g_id_map.end() || found->second.info != typeid(T))
		{
			return false;
		}

		g_id_map.erase(found);

		return true;
	}

	template<typename T> u32 get_count()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;

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
		extern std::unordered_map<u32, ID_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		std::set<u32> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result.insert(v.first);
			}
		}

		return result;
	}

	// get sorted ID map (ID value -> ID data) of specified type
	template<typename T> std::map<u32, std::shared_ptr<T>> get_map()
	{
		extern std::mutex g_id_mutex;
		extern std::unordered_map<u32, ID_data_t> g_id_map;

		std::lock_guard<std::mutex> lock(g_id_mutex);

		std::map<u32, std::shared_ptr<T>> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : g_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result[v.first] = std::static_pointer_cast<T>(v.second.data);
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

	// check whether the object exists
	template<typename T> bool check()
	{
		extern std::mutex g_fx_mutex;
		extern std::unordered_map<std::type_index, std::shared_ptr<void>> g_fx_map;

		std::lock_guard<std::mutex> lock(g_fx_mutex);

		return g_fx_map.find(typeid(T)) != g_fx_map.end();
	}

	// get fixed object of specified type
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
