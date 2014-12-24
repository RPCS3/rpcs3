#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/atomic_type.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "sleep_queue_type.h"

sleep_queue_t::~sleep_queue_t()
{
	for (auto& tid : m_list)
	{
		LOG_NOTICE(HLE, "~sleep_queue_t(): thread %d", tid);
	}
}

void sleep_queue_t::push(u32 tid, u32 protocol)
{
	assert(tid);

	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& v : m_list)
		{
			if (v == tid)
			{
				assert(!"sleep_queue_t::push() failed (duplication)");
			}
		}

		m_list.push_back(tid);
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

		if (m_list.size())
		{
			const u32 res = m_list[0];
			if (!Emu.GetIdManager().CheckID(res))
			{
				LOG_ERROR(HLE, "sleep_queue_t::pop(SYS_SYNC_FIFO): invalid thread (%d)", res);
				Emu.Pause();
			}

			m_list.erase(m_list.begin());
			return res;
		}
		else
		{
			return 0;
		}
	}
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		u64 highest_prio = ~0ull;
		u64 sel = ~0ull;
		for (auto& v : m_list)
		{
			if (std::shared_ptr<CPUThread> t = Emu.GetCPU().GetThread(v))
			{
				const u64 prio = t->GetPrio();
				if (prio < highest_prio)
				{
					highest_prio = prio;
					sel = &v - m_list.data();
				}
			}
			else
			{
				LOG_ERROR(HLE, "sleep_queue_t::pop(SYS_SYNC_PRIORITY): invalid thread (%d)", v);
				Emu.Pause();
			}
		}

		if (~sel)
		{
			const u32 res = m_list[sel];
			m_list.erase(m_list.begin() + sel);
			return res;
		}
		// fallthrough
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
	assert(tid);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& v : m_list)
	{
		if (v == tid)
		{
			m_list.erase(m_list.begin() + (&v - m_list.data()));
			return true;
		}
	}

	return false;
}

u32 sleep_queue_t::count()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return (u32)m_list.size();
}
