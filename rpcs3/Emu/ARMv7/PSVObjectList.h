#pragma once

union psv_uid_t
{
	// true UID format is partially unknown
	s32 uid;

	struct
	{
		u32 oddness : 1; // always 1 for UIDs (to not mess it up with addresses)
		u32 number : 15; // ID from 0 to 2^15-1
		u32 type : 15; // UID class (psv_object_class_t)
		u32 sign : 1; // UIDs are positive, error codes are negative
	};

	static psv_uid_t make(s32 uid)
	{
		psv_uid_t result;
		result.uid = uid;
		return result;
	}
};

template<typename T, u32 uid_class>
class psv_object_list_t // Class for managing object data
{
	std::array<std::shared_ptr<T>, 0x8000> m_data;
	std::atomic<u32> m_hint; // guessing next free position
	std::mutex m_mutex;

	void error(s32 uid)
	{
		throw fmt::format("Invalid UID requested (type=0x%x, uid=0x%x)", type, uid);
	}

public:
	psv_object_list_t()
		: m_hint(0)
	{
	}

	// check if UID is potentially valid (will return true even if the object doesn't exist)
	inline static bool check(s32 uid)
	{
		const psv_uid_t id = psv_uid_t::make(uid);

		// check sign bit, uid class and ensure that value is odd
		return !id.sign && id.type == uid_class && id.oddness == 1;
	}

	// share object with UID specified
	__forceinline std::shared_ptr<T> get(s32 uid)
	{
		if (!check(uid))
		{
			return nullptr;
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		return m_data[psv_uid_t::make(uid).number];
	}

	__forceinline std::shared_ptr<T> operator [](s32 uid)
	{
		return this->get(uid);
	}

	// create new object and generate UID for it, or do nothing and return zero (if limit reached)
	template<typename... Args> s32 create(Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (u32 i = 0, j = m_hint; i < m_data.size(); i++, j = (j + 1) % m_data.size())
		{
			// find an empty position and copy the pointer
			if (!m_data[j])
			{
				m_data[j] = std::make_shared<T>(args...); // construct object with specified arguments

				m_hint = (j + 1) % m_data.size(); // guess next position

				psv_uid_t id = psv_uid_t::make(1); // make UID
				id.type = uid_class;
				id.number = j;

				return id.uid; // return UID
			}
		}

		return 0;
	}

	// remove object with specified UID
	bool remove(s32 uid)
	{
		if (!check(uid))
		{
			return false;
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		const u32 pos = psv_uid_t::make(uid).number;

		m_hint = std::min<u32>(pos, m_hint);

		if (!m_data[pos])
		{
			return false;
		}

		m_data[pos].reset();
		return true;
	}

	// remove all objects
	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& v : m_data)
		{
			v.reset();
		}

		m_hint = 0;
	}
};

void clear_all_psv_objects();
