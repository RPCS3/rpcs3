#pragma once
#include <unordered_map>

typedef u32 ID_TYPE;

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

struct ID
{
	std::string m_name;
	u32 m_attr;
	IDData* m_data;

	template<typename T>
	ID(const std::string& name, T* data, const u32 attr)
		: m_name(name)
		, m_attr(attr)
	{
		m_data = new IDData(data, [](void *ptr) -> void { delete (T*)ptr; });
	}

	ID() : m_data(nullptr)
	{
	}

	ID(ID&& other)
	{
		m_name = other.m_name;
		m_attr = other.m_attr;
		m_data = other.m_data;
		other.m_data = nullptr;
	}

	void Kill()
	{
		delete m_data;
	}
};

class IdManager
{
	static const ID_TYPE s_first_id = 1;
	static const ID_TYPE s_max_id = -1;

	std::unordered_map<ID_TYPE, ID> m_id_map;
	std::mutex m_mtx_main;

	ID_TYPE m_cur_id;

public:
	IdManager() : m_cur_id(s_first_id)
	{
	}
	
	~IdManager()
	{
		Clear();
	}

	bool CheckID(const ID_TYPE id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		return m_id_map.find(id) != m_id_map.end();
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		for(auto& i : m_id_map)
		{
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
	ID_TYPE GetNewID(const std::string& name = "", T* data = nullptr, const u32 attr = 0)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		m_id_map[m_cur_id] = std::move(ID(name, data, attr));

		return m_cur_id++;
	}
	
	ID& GetID(const ID_TYPE id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		return m_id_map[id];
	}

	template<typename T>
	bool GetIDData(const ID_TYPE id, T*& result)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto f = m_id_map.find(id);

		if(f == m_id_map.end())
		{
			return false;
		}

		result = f->second.m_data->get<T>();

		return true;
	}

	bool HasID(const s64 id)
	{
		{
			std::lock_guard<std::mutex> lock(m_mtx_main);

			if(id == wxID_ANY)
				return m_id_map.begin() != m_id_map.end();
		}

		return CheckID(id);
	}

	bool RemoveID(const ID_TYPE id)
	{
		std::lock_guard<std::mutex> lock(m_mtx_main);

		auto item = m_id_map.find(id);

		if(item == m_id_map.end()) return false;

		item->second.Kill();
		m_id_map.erase(item);

		return true;
	}
};
