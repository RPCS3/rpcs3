#include "stdafx.h"
#include "event.h"

void EventManager::Init()
{
	
}

void EventManager::Clear()
{
	key_map.clear();
}

bool EventManager::CheckKey(u64 key)
{
	if (!key) return true;
	SMutexLocker lock(m_lock);

	return key_map.find(key) != key_map.end();
}

bool EventManager::RegisterKey(EventQueue* data, u64 key)
{
	if (!key) return true;
	SMutexLocker lock(m_lock);

	if (key_map.find(key) != key_map.end()) return false;

	for (auto& v : key_map)
	{
		if (v.second == data) return false;
	}

	key_map[key] = data;

	return true;
}

bool EventManager::GetEventQueue(u64 key, EventQueue*& data)
{
	data = nullptr;
	if (!key) return false;
	SMutexLocker lock(m_lock);

	auto f = key_map.find(key);
	if (f != key_map.end())
	{
		data = f->second;
		return true;
	}
	return false;
}

bool EventManager::UnregisterKey(u64 key)
{
	if (!key) return false;
	SMutexLocker lock(m_lock);

	auto f = key_map.find(key);
	if (f != key_map.end())
	{
		key_map.erase(f);
		return true;
	}
	return false;
}

bool EventManager::SendEvent(u64 key, u64 source, u64 d1, u64 d2, u64 d3)
{
	if (!key) return false;
	SMutexLocker lock(m_lock);

	auto f = key_map.find(key);
	if (f == key_map.end())
	{
		return false;
	}
	EventQueue* eq = f->second;

	eq->events.push(source, d1, d2, d3);
	return true;
}
