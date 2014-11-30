#pragma once
#include <unordered_map>

#define rID_ANY -1 // was wxID_ANY

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

	template<typename T> T* get()
	{
		return (T*)m_ptr;
	}

	template<typename T> const T* get() const
	{
		return (const T*)m_ptr;
	}
};

class ID
{
	std::string m_name;
	IDData* m_data;
	IDType m_type;

public:
	template<typename T>
	ID(const std::string& name, T* data, const IDType type)
		: m_name(name)
		, m_type(type)
	{
		m_data = new IDData(data, [](void *ptr) -> void { delete (T*)ptr; });
	}

	ID() : m_data(nullptr)
	{
	}

	ID(ID&& other)
	{
		m_name = other.m_name;
		m_type = other.m_type;
		m_data = other.m_data;
		other.m_data = nullptr;
	}
	ID& operator=(ID&& other)
	{
		std::swap(m_name,other.m_name);
		std::swap(m_type,other.m_type);
		std::swap(m_data,other.m_data);
		return *this;
	}

	void Kill()
	{
		delete m_data;
	}

	const std::string& GetName() const
	{
		return m_name;
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

	bool CheckID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		return m_id_map.find(id) != m_id_map.end();
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		for(auto& i : m_id_map) {
			i.second.Kill();
		}

		m_id_map.clear();
		m_cur_id = s_first_id;
	}
	
	template<typename T 
#ifdef __GNUG__ 
		= char
#endif
	>
	u32 GetNewID(const std::string& name = "", T* data = nullptr, const IDType type = TYPE_OTHER)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		m_id_map[m_cur_id] = ID(name, data, type);
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

	template<typename T>
	bool GetIDData(const u32 id, T*& result)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto f = m_id_map.find(id);
		if (f == m_id_map.end()) {
			return false;
		}

		result = f->second.GetData()->get<T>();

		return true;
	}

	bool HasID(const u32 id)
	{
		{
			std::lock_guard<std::mutex> lock(m_mtx_main);

			if(id == rID_ANY) {
				return m_id_map.begin() != m_id_map.end();
			}
		}
		return CheckID(id);
	}

	bool RemoveID(const u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto item = m_id_map.find(id);

		if (item == m_id_map.end()) {
			return false;
		}
		if (item->second.GetType() < TYPE_OTHER) {
			m_types[item->second.GetType()].erase(id);
		}

		item->second.Kill();
		m_id_map.erase(item);

		return true;
	}

	u32 GetTypeCount(IDType type)
	{
		if (type < TYPE_OTHER) {
			return (u32)m_types[type].size();
		}
		return 1;
	}

	const std::set<u32>& GetTypeIDs(IDType type)
	{
		assert(type < TYPE_OTHER);
		return m_types[type];
	}
};
