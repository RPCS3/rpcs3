#pragma once

#define ID_MANAGER_INCLUDED

// ID type
enum : u32
{
	ID_TYPE_NONE = 0,
};

// Helper template to detect type
template<typename T> struct ID_type
{
	//static_assert(sizeof(T) == 0, "ID type not registered (use REG_ID_TYPE)");

	static const u32 type = ID_TYPE_NONE; // default type
};

class ID_data_t final
{
public:
	const std::shared_ptr<void> data;
	const std::type_info& info;
	const std::size_t hash;
	const u32 type;
	const u32 id;

	template<typename T> force_inline ID_data_t(std::shared_ptr<T> data, u32 type, u32 id)
		: data(std::move(data))
		, info(typeid(T))
		, hash(typeid(T).hash_code())
		, type(type)
		, id(id)
	{
	}

	ID_data_t(const ID_data_t& right)
		: data(right.data)
		, info(right.info)
		, hash(right.hash)
		, type(right.type)
		, id(right.id)
	{
	}

	ID_data_t& operator =(const ID_data_t& right) = delete;

	ID_data_t(ID_data_t&& right)
		: data(std::move(const_cast<std::shared_ptr<void>&>(right.data)))
		, info(right.info)
		, hash(right.hash)
		, type(right.type)
		, id(right.id)
	{
	}

	ID_data_t& operator =(ID_data_t&& other) = delete;
};

class ID_manager
{
	std::mutex m_mutex;

	std::unordered_map<u32, ID_data_t> m_id_map;
	u32 m_cur_id = 1; // first ID

public:
	// check if ID exists and has specified type
	template<typename T> bool check_id(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.info == typeid(T);
	}

	// check if ID exists and has specified type
	bool check_id(u32 id, u32 type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.type == type;
	}

	// must be called from the constructor called through make() to get further ID of current object
	u32 get_current_id()
	{
		// if called correctly from make(), the mutex is locked
		// if called illegally, the mutex is unlocked with high probability (wrong ID is returned otherwise)

		if (m_mutex.try_lock())
		{
			// schedule unlocking
			std::lock_guard<std::mutex> lock(m_mutex, std::adopt_lock);

			throw EXCEPTION("Current ID is not available");
		}

		return m_cur_id;
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.clear();
		m_cur_id = 1; // first ID
	}

	// add new ID of specified type with specified constructor arguments (returns object)
	template<typename T, typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>> std::shared_ptr<T> make_ptr(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		m_id_map.emplace(m_cur_id, ID_data_t(ptr, type, m_cur_id));

		return m_cur_id++, std::move(ptr);
	}

	// add new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		m_id_map.emplace(m_cur_id, ID_data_t(std::make_shared<T>(std::forward<Args>(args)...), type, m_cur_id));

		return m_cur_id++;
	}

	// load ID created with type Orig, optionally static_cast to T
	template<typename T, typename Orig = T> auto get(u32 id) -> decltype(std::shared_ptr<T>(static_cast<T*>(std::declval<Orig*>())))
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		if (f == m_id_map.end() || f->second.info != typeid(Orig))
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(f->second.data);
	}

	// load all IDs created with type Orig, optionally static_cast to T
	template<typename T, typename Orig = T> auto get_all() -> std::vector<decltype(std::shared_ptr<T>(static_cast<T*>(std::declval<Orig*>())))>
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::vector<std::shared_ptr<T>> result;

		const std::size_t hash = typeid(Orig).hash_code();

		for (auto& v : m_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(Orig))
			{
				result.emplace_back(std::static_pointer_cast<T>(v.second.data));
			}
		}

		return result;
	}

	template<typename T> bool remove(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto item = m_id_map.find(id);

		if (item == m_id_map.end() || item->second.info != typeid(T))
		{
			return false;
		}

		m_id_map.erase(item);

		return true;
	}

	template<typename T> u32 get_count()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		u32 result = 0;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : m_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result++;
			}
		}

		return result;
	}

	u32 get_count(u32 type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		u32 result = 0;

		for (auto& v : m_id_map)
		{
			if (v.second.type == type)
			{
				result++;
			}
		}

		return result;
	}

	// get sorted ID list
	template<typename T> std::set<u32> get_IDs()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::set<u32> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : m_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result.insert(v.first);
			}
		}

		return result;
	}

	// get sorted ID list
	std::set<u32> get_IDs(u32 type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::set<u32> result;

		for (auto& v : m_id_map)
		{
			if (v.second.type == type)
			{
				result.insert(v.first);
			}
		}

		return result;
	}

	template<typename T> std::vector<ID_data_t> get_data()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::vector<ID_data_t> result;

		const std::size_t hash = typeid(T).hash_code();

		for (auto& v : m_id_map)
		{
			if (v.second.hash == hash && v.second.info == typeid(T))
			{
				result.emplace_back(v.second);
			}
		}

		return result;
	}

	std::vector<ID_data_t> get_data(u32 type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::vector<ID_data_t> result;

		for (auto& v : m_id_map)
		{
			if (v.second.type == type)
			{
				result.emplace_back(v.second);
			}
		}

		return result;
	}
};
