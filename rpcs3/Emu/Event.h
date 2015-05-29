#pragma once

struct lv2_event_queue_t;

class EventManager
{
	std::mutex m_lock;
	std::unordered_map<u64, std::shared_ptr<lv2_event_queue_t>> eq_map;

public:
	void Init();
	void Clear();
	bool CheckKey(u64 key);
	bool RegisterKey(const std::shared_ptr<lv2_event_queue_t>& data);
	bool UnregisterKey(u64 key);

	template<typename... Args> std::shared_ptr<lv2_event_queue_t> MakeEventQueue(Args&&... args)
	{
		const auto queue = std::make_shared<lv2_event_queue_t>(args...);

		return RegisterKey(queue) ? queue : nullptr;
	}

	std::shared_ptr<lv2_event_queue_t> GetEventQueue(u64 key);
};
