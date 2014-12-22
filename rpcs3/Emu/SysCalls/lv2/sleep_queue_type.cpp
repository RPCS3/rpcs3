#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue_type.h"

void sleep_queue_t::push(u32 tid, u32 protocol)
{
	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		list.push_back(tid);
		return;
	}
	case SYS_SYNC_RETRY:
	{
		return;
	}
	}

	LOG_ERROR(HLE, "sleep_queue_t::push(): unsupported protocol (0x%x)", protocol);
	Emu.Pause();
}

u32 sleep_queue_t::pop(u32 protocol)
{
	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		while (true)
		{
			if (list.size())
			{
				u32 res = list[0];
				list.erase(list.begin());
				if (res && Emu.GetIdManager().CheckID(res))
					// check thread
				{
					return res;
				}
			}
			return 0;
		}
	}
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		while (true)
		{
			if (list.size())
			{
				u64 highest_prio = ~0ull;
				u32 sel = 0;
				for (u32 i = 0; i < list.size(); i++)
				{
					CPUThread* t = Emu.GetCPU().GetThread(list[i]);
					if (!t)
					{
						list[i] = 0;
						sel = i;
						break;
					}
					u64 prio = t->GetPrio();
					if (prio < highest_prio)
					{
						highest_prio = prio;
						sel = i;
					}
				}
				u32 res = list[sel];
				list.erase(list.begin() + sel);
				/* if (Emu.GetIdManager().CheckID(res)) */
				if (res)
					// check thread
				{
					return res;
				}
			}

			return 0;
		}
	}
	case SYS_SYNC_RETRY:
	{
		return 0;
	}
	}

	LOG_ERROR(HLE, "sleep_queue_t::pop(): unsupported protocol (0x%x)", protocol);
	Emu.Pause();
	return 0;
}

bool sleep_queue_t::invalidate(u32 tid)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (tid) for (u32 i = 0; i < list.size(); i++)
	{
		if (list[i] == tid)
		{
			list.erase(list.begin() + i);
			return true;
		}
	}

	return false;
}

u32 sleep_queue_t::count()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	u32 result = 0;
	for (u32 i = 0; i < list.size(); i++)
	{
		if (list[i]) result++;
	}
	return result;
}

bool sleep_queue_t::finalize()
{
	if (!m_mutex.try_lock()) return false;

	for (u32 i = 0; i < list.size(); i++)
	{
		if (list[i])
		{
			m_mutex.unlock();
			return false;
		}
	}

	m_mutex.unlock();
	return true;
}
