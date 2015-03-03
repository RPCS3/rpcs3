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

template<typename T, u32 type>
class psv_object_list_t // Class for managing object data
{
public:
	typedef refcounter_t<T> rc_type;
	typedef ref_t<T> ref_type;

	static const u32 max = 0x8000;

private:
	std::array<rc_type, max> m_data;
	std::atomic<u32> m_hint; // guessing next free position

	void error(s32 uid)
	{
		throw fmt::format("Invalid UID requested (type=0x%x, uid=0x%x)", type, uid);
	}

public:
	psv_object_list_t()
		: m_hint(0)
	{
	}

	psv_object_list_t(const psv_object_list_t&) = delete;
	psv_object_list_t(psv_object_list_t&&) = delete;

	psv_object_list_t& operator =(const psv_object_list_t&) = delete;
	psv_object_list_t& operator =(psv_object_list_t&&) = delete;

public:
	static const u32 uid_class = type;

	// check if UID is potentially valid (will return true even if the object doesn't exist)
	bool check(s32 uid)
	{
		const psv_uid_t id = psv_uid_t::make(uid);

		// check sign bit, uid class and ensure that value is odd
		return !id.sign && id.type == uid_class && id.oddness == 1;
	}

	// share object with UID specified
	ref_type get(s32 uid)
	{
		if (!check(uid))
		{
			return ref_type();
		}

		return &m_data[psv_uid_t::make(uid).number];
	}

	ref_type operator [](s32 uid)
	{
		return find(uid);
	}

	// generate UID for newly created object (will return zero if the limit exceeded)
	s32 add(T* data, s32 error_code)
	{
		for (u32 i = 0, j = m_hint; i < m_data.size(); i++, j = (j + 1) % m_data.size())
		{
			// find an empty position and copy the pointer
			if (m_data[j].try_set(data))
			{
				m_hint = (j + 1) % m_data.size(); // guess next position

				psv_uid_t id = psv_uid_t::make(1); // make UID
				id.type = uid_class;
				id.number = j;

				return id.uid; // return UID
			}
		}

		delete data;
		return error_code;
	}

	// remove object with specified UID
	bool remove(s32 uid)
	{
		if (!check(uid))
		{
			return false;
		}

		const u32 pos = psv_uid_t::make(uid).number;

		m_hint = std::min<u32>(pos, m_hint);

		return m_data[pos].try_remove();
	}

	// remove all objects
	void clear()
	{
		for (auto& v : m_data)
		{
			v.try_remove();
		}

		m_hint = 0;
	}
};

void clear_all_psv_objects();
