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

	// 0 is invalid ID
	// 1..0x7fffffff : general purpose IDs
	// 0x80000000+ : occupied by fixed IDs
	std::unordered_map<u32, ID_data_t> m_id_map;

	// contains the next ID or 0x80000000 | current_ID
	u32 m_cur_id = 1;

	static u32 get_type_fixed_id(const std::type_info& type)
	{
		// TODO: more reliable way of fixed ID generation
		return 0x80000000 | static_cast<u32>(type.hash_code());
	}

public:
	// check if ID exists and has specified type
	template<typename T> bool check_id(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.info == typeid(T);
	}

	// check if ID exists and has specified type
	bool check_id(u32 id, u32 type)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto f = m_id_map.find(id);

		return f != m_id_map.end() && f->second.type == type;
	}

	// must be called from the constructor called through make() or make_ptr() to get further ID of current object
	u32 get_current_id()
	{
		if ((m_cur_id & 0x80000000) == 0)
		{
			throw EXCEPTION("Current ID is not available");
		}

		return m_cur_id & 0x7fffffff;
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_id_map.clear();
		m_cur_id = 1; // first ID
	}

	// add fixed ID of specified type only if it doesn't exist (each type has unique id)
	template<typename T, typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>> std::shared_ptr<T> make_fixed(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		static const u32 id = get_type_fixed_id(typeid(T));

		const auto found = m_id_map.find(id);

		// ensure that this ID doesn't exist
		if (found == m_id_map.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			m_id_map.emplace(id, ID_data_t(ptr, type, id));

			return std::move(ptr);
		}

		// ensure that this ID is not occupied by the object of another type
		if (found->second.info == typeid(T))
		{
			return nullptr;
		}

		throw EXCEPTION("Collision occured ('%s' and '%s', id=0x%x)", found->second.info.name(), typeid(T).name(), id);
	}

	// add new ID of specified type with specified constructor arguments (returns object)
	template<typename T, typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>> std::shared_ptr<T> make_ptr(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		m_cur_id |= 0x80000000;

		if (const u32 id = m_cur_id & 0x7fffffff)
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

			m_id_map.emplace(id, ID_data_t(ptr, type, id));

			m_cur_id = id + 1;

			return std::move(ptr);
		}

		throw EXCEPTION("Out of IDs");
	}

	// add new ID of specified type with specified constructor arguments (returns id)
	template<typename T, typename... Args> std::enable_if_t<std::is_constructible<T, Args...>::value, u32> make(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 type = ID_type<T>::type;

		m_cur_id |= 0x80000000;

		if (const u32 id = m_cur_id & 0x7fffffff)
		{
			m_id_map.emplace(id, ID_data_t(std::make_shared<T>(std::forward<Args>(args)...), type, id));

			m_cur_id = id + 1;

			return id;
		}

		throw EXCEPTION("Out of IDs");
	}

	// load fixed ID of specified type Orig, optionally static_cast to T
	template<typename T, typename Orig = T> auto get() -> decltype(std::shared_ptr<T>(static_cast<T*>(std::declval<Orig*>())))
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		static const u32 id = get_type_fixed_id(typeid(T));

		const auto found = m_id_map.find(id);

		if (found == m_id_map.end() || found->second.info != typeid(Orig))
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second.data);
	}

	// load ID created with type Orig, optionally static_cast to T
	template<typename T, typename Orig = T> auto get(u32 id) -> decltype(std::shared_ptr<T>(static_cast<T*>(std::declval<Orig*>())))
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto found = m_id_map.find(id);

		if (found == m_id_map.end() || found->second.info != typeid(Orig))
		{
			return nullptr;
		}

		return std::static_pointer_cast<T>(found->second.data);
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

	// remove fixed ID created with type T
	template<typename T> bool remove()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		static const u32 id = get_type_fixed_id(typeid(T));

		const auto found = m_id_map.find(id);

		if (found == m_id_map.end() || found->second.info != typeid(T))
		{
			return false;
		}

		m_id_map.erase(found);

		return true;
	}

	// remove ID created with type T
	template<typename T> bool remove(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto found = m_id_map.find(id);

		if (found == m_id_map.end() || found->second.info != typeid(T))
		{
			return false;
		}

		m_id_map.erase(found);

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
	template<typename T> std::set<u32> get_set()
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
	std::set<u32> get_set(u32 type)
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
