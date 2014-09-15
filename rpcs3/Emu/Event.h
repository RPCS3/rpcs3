#pragma once
#include "Emu/SysCalls/lv2/sys_event.h"
#include <unordered_map>

class EventManager
{
	std::mutex m_lock;
	std::unordered_map<u64, EventQueue*> key_map;

public:
	void Init();
	void Clear();
	bool CheckKey(u64 key);
	bool RegisterKey(EventQueue* data, u64 key);
	bool GetEventQueue(u64 key, EventQueue*& data);
	bool UnregisterKey(u64 key);
	bool SendEvent(u64 key, u64 source, u64 d1, u64 d2, u64 d3);
};
