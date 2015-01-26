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
	std::array<std::shared_ptr<T>, 0x8000> m_data;
	std::mutex m_mutex; // TODO: remove it when shared_ptr atomic ops are fully available

public:
	static const u32 uid_class = type;

	// check if UID is potentially valid (will return true if the object doesn't exist)
	bool check(s32 uid)
	{
		const psv_uid_t id = psv_uid_t::make(uid);

		// check sign bit, uid class and ensure that value is odd
		return !id.sign && id.type == uid_class && id.oddness == 1;
	}

	// share object with UID specified (will return empty pointer if the object doesn't exist or the UID is invalid)
	std::shared_ptr<T> find(s32 uid)
	{
		if (!check(uid))
		{
			return nullptr;
		}

		return m_data[psv_uid_t::make(uid).number];
	}

	std::shared_ptr<T> operator [](s32 uid)
	{
		return find(uid);
	}

	// generate UID for newly created object (will return zero if the limit exceeded)
	s32 add(std::shared_ptr<T>& data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& value : m_data)
		{
			// find an empty position and move the pointer
			//std::shared_ptr<T> old_ptr = nullptr;
			//if (std::atomic_compare_exchange_strong(&value, &old_ptr, data))
			if (!value)
			{
				value = data;
				psv_uid_t id = psv_uid_t::make(1); // odd number
				id.type = uid_class; // set type
				id.number = &value - m_data.data(); // set position
				return id.uid;
			}
		}

		return 0;
	}

	// remove object with UID specified and share it for the last time (will return empty pointer if the object doesn't exists or the UID is invalid)
	std::shared_ptr<T> remove(s32 uid)
	{
		if (!check(uid))
		{
			return nullptr;
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		std::shared_ptr<T> old_ptr = nullptr;
		m_data[psv_uid_t::make(uid).number].swap(old_ptr);
		return old_ptr;
		//return std::atomic_exchange<std::shared_ptr<T>>(&m_data[psv_uid_t::make(uid).number], nullptr);
	}

	// remove all objects
	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& value : m_data)
		{
			value = nullptr;
		}
	}
};

void clear_all_psv_objects();
