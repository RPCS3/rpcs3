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
	m_map.clear();
}

bool EventManager::CheckKey(u64 key)
{
	if (!key)
	{
		// never exists
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	return m_map.find(key) != m_map.end();
}

bool EventManager::UnregisterKey(u64 key)
{
	if (!key)
	{
		// always ok
		return true;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	auto f = m_map.find(key);
	if (f != m_map.end())
	{
		m_map.erase(f);
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

	std::lock_guard<std::mutex> lock(m_mutex);

	auto f = m_map.find(key);
	if (f != m_map.end())
	{
		return f->second;
	}

	return nullptr;
}
