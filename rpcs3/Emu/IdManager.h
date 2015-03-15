#pragma once

#define ID_MANAGER_INCLUDED

enum IDType
{
	// Special objects
	TYPE_MEM,
	TYPE_MUTEX,
	TYPE_COND,
	TYPE_RWLOCK,
	TYPE_INTR_TAG,
	TYPE_INTR_SERVICE_HANDLE,
	TYPE_EVENT_QUEUE,
	TYPE_EVENT_PORT,
	TYPE_TRACE,
	TYPE_SPUIMAGE,
	TYPE_PRX,
	TYPE_SPUPORT,
	TYPE_LWMUTEX,
	TYPE_TIMER,
	TYPE_SEMAPHORE,
	TYPE_FS_FILE,
	TYPE_FS_DIR,
	TYPE_LWCOND,
	TYPE_EVENT_FLAG,

	// Any other objects
	TYPE_OTHER,
};

class ID final
{
	const std::type_info& m_info;
	std::shared_ptr<void> m_data;
	IDType m_type;

public:
	template<typename T> ID(std::shared_ptr<T>& data, const IDType type)
		: m_info(typeid(T))
		, m_data(data)
		, m_type(type)
	{
	}

	ID()
		: m_info(typeid(void))
		, m_data(nullptr)
		, m_type(TYPE_OTHER)
	{
	}

	ID(const ID& right) = delete;

	ID(ID&& right)
		: m_info(right.m_info)
		, m_data(right.m_data)
		, m_type(right.m_type)
	{
		right.m_data = nullptr;
		right.m_type = TYPE_OTHER;
	}

	ID& operator=(ID&& other) = delete;

	const std::type_info& GetInfo() const
	{
		return m_info;
	}

	template<typename T> std::shared_ptr<T> GetData() const
	{
		return std::static_pointer_cast<T>(m_data);
	}

	IDType GetType() const
	{
		return m_type;
	}
};

class IdManager
{
	static const u32 s_first_id = 1;
	static const u32 s_max_id = -1;

	std::unordered_map<u32, ID> m_id_map;
	std::set<u32> m_types[TYPE_OTHER];
	std::mutex m_mutex;

	u32 m_cur_id = s_first_id;

public:
	template<typename T> bool CheckID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.GetInfo() == typeid(T);
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.clear();
		m_cur_id = s_first_id;
	}
	
	template<typename T> u32 GetNewID(std::shared_ptr<T>& data, const IDType type = TYPE_OTHER)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.emplace(m_cur_id, ID(data, type));

		if (type < TYPE_OTHER)
		{
			m_types[type].insert(m_cur_id);
		}

		return m_cur_id++;
	}

	template<typename T> bool GetIDData(const u32 id, std::shared_ptr<T>& result)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		if (f == m_id_map.end() || f->second.GetInfo() != typeid(T))
		{
			return false;
		}

		result = f->second.GetData<T>();

		return true;
	}

	template<typename T> std::shared_ptr<T> GetIDData(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto f = m_id_map.find(id);

		if (f == m_id_map.end() || f->second.GetInfo() != typeid(T))
		{
			return nullptr;
		}

		return f->second.GetData<T>();
	}

	bool HasID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return m_id_map.find(id) != m_id_map.end();
	}

	IDType GetIDType(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto item = m_id_map.find(id);

		if (item == m_id_map.end())
		{
			return TYPE_OTHER;
		}

		return item->second.GetType();
	}

	template<typename T> bool RemoveID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto item = m_id_map.find(id);

		if (item == m_id_map.end() || item->second.GetInfo() != typeid(T))
		{
			return false;
		}

		if (item->second.GetType() < TYPE_OTHER)
		{
			m_types[item->second.GetType()].erase(id);
		}

		m_id_map.erase(item);

		return true;
	}

	u32 GetTypeCount(IDType type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (type < TYPE_OTHER)
		{
			return (u32)m_types[type].size();
		}
		else
		{
			assert(!"Invalid ID type");
			return 0;
		}
	}

	std::set<u32> GetTypeIDs(IDType type)
	{
		// you cannot simply return reference to existing set
		std::lock_guard<std::mutex> lock(m_mutex);

		if (type < TYPE_OTHER)
		{
			return m_types[type];
		}
		else
		{
			assert(!"Invalid ID type");
			return std::set<u32>{};
		}
	}
};
