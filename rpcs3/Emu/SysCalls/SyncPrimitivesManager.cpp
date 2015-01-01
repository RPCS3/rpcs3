#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Utilities/Thread.h"

#include "lv2/sleep_queue_type.h"
#include "lv2/sys_lwmutex.h"
#include "lv2/sys_lwcond.h"
#include "lv2/sys_mutex.h"
#include "lv2/sys_cond.h"
#include "lv2/sys_semaphore.h"
#include "SyncPrimitivesManager.h"

SemaphoreAttributes SyncPrimManager::GetSemaphoreData(u32 id)
{
	std::shared_ptr<Semaphore> sem;
	if (!Emu.GetIdManager().GetIDData(id, sem))
	{
		return{};
	}
	
	return{ std::string((const char*)&sem->name, 8), sem->value.read_sync(), sem->max };
}

LwMutexAttributes SyncPrimManager::GetLwMutexData(u32 id)
{
	std::shared_ptr<sleep_queue_t> sq;
	if (!Emu.GetIdManager().GetIDData(id, sq))
	{
		return{};
	}

	return{ std::string((const char*)&sq->name, 8) };
}

std::string SyncPrimManager::GetSyncPrimName(u32 id, IDType type)
{
	switch (type)
	{
	case TYPE_LWCOND:
	{
		std::shared_ptr<Lwcond> lw;
		if (Emu.GetIdManager().GetIDData(id, lw))
		{
			return std::string((const char*)&lw->queue.name, 8);
		}
		break;
	}

	case TYPE_MUTEX:
	{
		std::shared_ptr<Mutex> mutex;
		if (Emu.GetIdManager().GetIDData(id, mutex))
		{
			return std::string((const char*)&mutex->queue.name, 8);
		}
		break;
	}

	case TYPE_COND:
	{
		std::shared_ptr<Cond> cond;
		if (Emu.GetIdManager().GetIDData(id, cond))
		{
			return std::string((const char*)&cond->queue.name, 8);
		}
		break;
	}
	}

	LOG_ERROR(GENERAL, "SyncPrimManager::GetSyncPrimName(id=%d, type=%d) failed", id, type);
	return "NOT_FOUND";
}
