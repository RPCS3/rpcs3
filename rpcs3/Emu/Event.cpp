#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/SysCalls/lv2/sleep_queue.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Event.h"

void EventManager::Init()
{
}

void EventManager::Clear()
{
	eq_map.clear();
}

bool EventManager::CheckKey(u64 key)
{
	if (!key)
	{
		// never exists
		return false;
	}

	std::lock_guard<std::mutex> lock(m_lock);

	return eq_map.find(key) != eq_map.end();
}

bool EventManager::RegisterKey(const std::shared_ptr<lv2_event_queue_t>& data)
{
	if (!data->key)
	{
		// always ok
		return true;
	}

	std::lock_guard<std::mutex> lock(m_lock);

	if (eq_map.find(data->key) != eq_map.end())
	{
		return false;
	}

	eq_map[data->key] = data;

	return true;
}

bool EventManager::UnregisterKey(u64 key)
{
	if (!key)
	{
		// always ok
		return true;
	}

	std::lock_guard<std::mutex> lock(m_lock);

	auto f = eq_map.find(key);
	if (f != eq_map.end())
	{
		eq_map.erase(f);
		return true;
	}

	return false;
}

std::shared_ptr<lv2_event_queue_t> EventManager::GetEventQueue(u64 key)
{
	if (!key)
	{
		// never exists
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(m_lock);

	auto f = eq_map.find(key);
	if (f != eq_map.end())
	{
		return f->second;
	}

	return nullptr;
}
