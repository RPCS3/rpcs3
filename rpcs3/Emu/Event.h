#pragma once
#include <unordered_map>

struct event_queue_t;

class EventManager
{
	std::mutex m_lock;
	std::unordered_map<u64, std::shared_ptr<event_queue_t>> eq_map;

public:
	void Init();
	void Clear();
	bool CheckKey(u64 key);
	bool RegisterKey(std::shared_ptr<event_queue_t>& data, u64 key);
	bool UnregisterKey(u64 key);

	std::shared_ptr<event_queue_t> GetEventQueue(u64 key);
};
