#pragma once

#include <map>
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Utilities/Log.h"

struct SemaphoreAttribites
{
	std::string name;
	u32 count;
	u32 max_count;

	SemaphoreAttribites() {}
	SemaphoreAttribites(const std::string& _name, u32 _count, u32 _max_count) : name(_name), count(_count), max_count(_max_count) {}
};

struct LwMutexAttributes
{
	std::string name;
	u32 owner_id;
	std::string status; // TODO: check status?

	LwMutexAttributes() {}
	LwMutexAttributes(const std::string& _name, u32 _owner_id, std::string _status = "INITIALIZED")
		: name(_name), owner_id(_owner_id), status(_status) {}
};

class SyncPrimManager
{
private:
	std::map<u32, std::string> m_cond_name;
	std::map<u32, std::string> m_mutex_name;
	std::map<u32, std::string> m_lw_cond_name;
	std::map<u32, LwMutexAttributes> m_lw_mutex_attr;
	std::map<u32, SemaphoreAttribites> m_semaph_attr;

public:

	// semaphores
	void AddSemaphoreData(const u32 id, const std::string& name, const u32 count, const u32 max_count)
	{
		m_semaph_attr[id] = *(new SemaphoreAttribites(name, count, max_count));
	}

	void EraseSemaphoreData(const u32 id)
	{
		m_semaph_attr.erase(id);
	}

	SemaphoreAttribites& GetSemaphoreData(const u32 id)
	{
		return m_semaph_attr[id];
	}

	// lw_mutexes
	void AddLwMutexData(const u32 id, const std::string& name, const u32 owner_id)
	{
		m_lw_mutex_attr[id] = *(new LwMutexAttributes(name, owner_id));
	}

	void EraseLwMutexData(const u32 id)
	{
		m_lw_mutex_attr.erase(id);
	}

	LwMutexAttributes& GetLwMutexData(const u32 id)
	{
		return m_lw_mutex_attr[id];
	}

	// lw_conditions, mutexes, conditions
	void AddSyncPrimData(const IDType type, const u32 id, const std::string& name)
	{
		switch (type)
		{
		case TYPE_LWCOND: m_lw_cond_name[id] = name; break;
		case TYPE_MUTEX: m_mutex_name[id] = name; break;
		case TYPE_COND: m_cond_name[id] = name; break;

		}
	}

	void EraseSyncPrimData(const IDType type, const u32 id)
	{
		switch (type)
		{
		case TYPE_LWCOND:  m_lw_cond_name.erase(id); break;
		case TYPE_MUTEX:  m_mutex_name.erase(id); break;
		case TYPE_COND:  m_cond_name.erase(id); break;

		default: LOG_ERROR(GENERAL, "Unknown IDType = %d", type);
		}
	}

	std::string& GetSyncPrimName(const IDType type, const u32 id)
	{
		switch (type)
		{
		case TYPE_LWCOND:  return m_lw_cond_name[id];
		case TYPE_MUTEX:  return m_mutex_name[id];
		case TYPE_COND:  return m_cond_name[id];
		}
	}

	void Close()
	{
		m_cond_name.clear();
		m_mutex_name.clear();
		m_lw_cond_name.clear();
		m_lw_mutex_attr.clear();
		m_semaph_attr.clear();
	}
};