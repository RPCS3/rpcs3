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
	const u32 type;

	template<typename T, typename... Args> ID_data_t(u32 type, Args&&... args) // doesn't work
		: data(std::make_shared<T>(args...))
		, info(typeid(T))
		, type(type)
	{
	}

	template<typename T> ID_data_t(const std::shared_ptr<T>& data, u32 type)
		: data(data)
		, info(typeid(T))
		, type(type)
	{
	}

	ID_data_t(const ID_data_t& right) = delete;

	ID_data_t& operator =(const ID_data_t& right) = delete;

	ID_data_t(ID_data_t&& right)
		: data(right.data)
		, info(right.info)
		, type(right.type)
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
	template<typename T> bool check_id(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.info == typeid(T);
	}

	// must be called from the constructor called through make() to get further ID of current object
	u32 get_cur_id()
	{
		// if called correctly from make(), the mutex is locked
		// if called illegally, the mutex is unlocked with high probability (wrong ID is returned otherwise)

		if (m_mutex.try_lock())
		{
			// schedule unlocking
			std::lock_guard<std::mutex> lock(m_mutex, std::adopt_lock);

			throw "Invalid get_cur_id() usage";
		}

		return m_cur_id;
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.clear();
		m_cur_id = 1; // first ID
	}
	
	// add new ID using existing std::shared_ptr (not recommended, use make() instead)
	template<typename T> u32 add(const std::shared_ptr<T>& data, u32 type = ID_type<T>::type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.emplace(m_cur_id, ID_data_t(data, type));

		return m_cur_id++;
	}

	// add new ID of specified type with specified constructor arguments (passed to std::make_shared<>)
	template<typename T, typename... Args> u32 make(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		m_id_map.emplace(m_cur_id, ID_data_t(std::make_shared<T>(args...), type));

		return m_cur_id++;
	}

	template<typename T> std::shared_ptr<T> get(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		if (f == m_id_map.end() || f->second.info != typeid(T))
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(f->second.data);
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

	u32 get_count_by_type(u32 type)
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

	std::set<u32> get_IDs_by_type(u32 type)
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
};
