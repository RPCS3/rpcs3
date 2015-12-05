#pragma once

#include "Emu/IdManager.h"

struct lv2_event_queue_t;

class EventManager
{
	std::mutex m_mutex;
	std::unordered_map<u64, std::shared_ptr<lv2_event_queue_t>> m_map;

public:
	void Init();
	void Clear();
	bool UnregisterKey(u64 key);

	template<typename... Args, typename = std::enable_if_t<std::is_constructible<lv2_event_queue_t, Args...>::value>> std::shared_ptr<lv2_event_queue_t> MakeEventQueue(u64 key, Args&&... args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (key && m_map.find(key) != m_map.end())
		{
			return nullptr;
		}

		auto queue = idm::make_ptr<lv2_event_queue_t>(std::forward<Args>(args)...);

		if (key)
		{
			m_map[key] = queue;
		}

		return queue;
	}

	std::shared_ptr<lv2_event_queue_t> GetEventQueue(u64 key);
};
