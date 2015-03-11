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

class IDData
{
protected:
	void* m_ptr;
	std::function<void(void*)> m_destr;

public:
	IDData(void* ptr, std::function<void(void*)> destr)
		: m_ptr(ptr)
		, m_destr(destr)
	{
	}

	~IDData()
	{
		m_destr(m_ptr);
	}

	template<typename T> std::shared_ptr<T> get() const
	{
		return *(std::shared_ptr<T>*)m_ptr;
	}
};

class ID
{
	const std::type_info& m_info;
	IDData* m_data;
	IDType m_type;

public:
	template<typename T>
	ID(std::shared_ptr<T>& data, const IDType type)
		: m_info(typeid(T))
		, m_type(type)
	{
		m_data = new IDData(new std::shared_ptr<T>(data), [](void *ptr) -> void { delete (std::shared_ptr<T>*)ptr; });
	}

	ID()
		: m_info(typeid(nullptr_t))
		, m_data(nullptr)
	{
	}

	ID(const ID& right) = delete;

	ID(ID&& right)
		: m_info(right.m_info)
		, m_data(right.m_data)
		, m_type(right.m_type)
	{
		right.m_data = nullptr;
	}

	ID& operator=(ID&& other) = delete;

	~ID()
	{
		if (m_data)
		{
			delete m_data;
		}
	}

	const std::type_info& GetInfo() const
	{
		return m_info;
	}

	IDData* GetData() const
	{
		return m_data;
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
	std::mutex m_mtx_main;

	u32 m_cur_id;

public:
	IdManager() : m_cur_id(s_first_id)
	{
	}
	
	~IdManager()
	{
		Clear();
	}

	template<typename T> bool CheckID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.GetInfo() == typeid(T);
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		m_id_map.clear();
		m_cur_id = s_first_id;
	}
	
	template<typename T> u32 GetNewID(std::shared_ptr<T>& data, const IDType type = TYPE_OTHER)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		m_id_map.emplace(m_cur_id, ID(data, type));
		if (type < TYPE_OTHER) {
			m_types[type].insert(m_cur_id);
		}

		return m_cur_id++;
	}
	
	ID& GetID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		return m_id_map[id];
	}

	template<typename T> bool GetIDData(const u32 id, std::shared_ptr<T>& result)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto f = m_id_map.find(id);

		if (f == m_id_map.end() || f->second.GetInfo() != typeid(T)) {
			return false;
		}

		result = f->second.GetData()->get<T>();

		return true;
	}

	bool HasID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		return m_id_map.find(id) != m_id_map.end();
	}

	template<typename T> bool RemoveID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto item = m_id_map.find(id);

		if (item == m_id_map.end() || item->second.GetInfo() != typeid(T)) {
			return false;
		}
		if (item->second.GetType() < TYPE_OTHER) {
			m_types[item->second.GetType()].erase(id);
		}

		m_id_map.erase(item);

		return true;
	}

	u32 GetTypeCount(IDType type)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

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
		std::lock_guard<std::mutex> lock(m_mtx_main);

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
