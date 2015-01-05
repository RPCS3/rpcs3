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
	for (auto& tid : m_waiting)
	{
		LOG_NOTICE(HLE, "~sleep_queue_t['%s']: m_waiting[%lld]=%d", m_name.c_str(), &tid - m_waiting.data(), tid);
	}
	for (auto& tid : m_signaled)
	{
		LOG_NOTICE(HLE, "~sleep_queue_t['%s']: m_signaled[%lld]=%d", m_name.c_str(), &tid - m_signaled.data(), tid);
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

		for (auto& v : m_waiting)
		{
			if (v == tid)
			{
				LOG_ERROR(HLE, "sleep_queue_t['%s']::push() failed: thread already waiting (%d)", m_name.c_str(), tid);
				Emu.Pause();
				return;
			}
		}

		for (auto& v : m_signaled)
		{
			if (v == tid)
			{
				LOG_ERROR(HLE, "sleep_queue_t['%s']::push() failed: thread already signaled (%d)", m_name.c_str(), tid);
				Emu.Pause();
				return;
			}
		}

		m_waiting.push_back(tid);
		return;
	}
	case SYS_SYNC_RETRY:
	{
		return;
	}
	}

	LOG_ERROR(HLE, "sleep_queue_t['%s']::push() failed: unsupported protocol (0x%x)", m_name.c_str(), protocol);
	Emu.Pause();
}

bool sleep_queue_t::pop(u32 tid, u32 protocol)
{
	assert(tid);

	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_signaled.size() && m_signaled[0] == tid)
		{
			m_signaled.erase(m_signaled.begin());
			return true;
		}

		for (auto& v : m_signaled)
		{
			if (v == tid)
			{
				return false;
			}
		}

		for (auto& v : m_waiting)
		{
			if (v == tid)
			{
				return false;
			}
		}

		LOG_ERROR(HLE, "sleep_queue_t['%s']::pop() failed: thread not found (%d)", m_name.c_str(), tid);
		Emu.Pause();
		return true; // ???
	}
	//case SYS_SYNC_RETRY: // ???
	//{
	//	return true; // ???
	//}
	}

	LOG_ERROR(HLE, "sleep_queue_t['%s']::pop() failed: unsupported protocol (0x%x)", m_name.c_str(), protocol);
	Emu.Pause();
	return false; // ???
}

u32 sleep_queue_t::signal(u32 protocol)
{
	u32 res = ~0;

	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_waiting.size())
		{
			res = m_waiting[0];
			if (!Emu.GetIdManager().CheckID(res))
			{
				LOG_ERROR(HLE, "sleep_queue_t['%s']::signal(SYS_SYNC_FIFO) failed: invalid thread (%d)", m_name.c_str(), res);
				Emu.Pause();
			}

			m_waiting.erase(m_waiting.begin());
			m_signaled.push_back(res);
		}
		else
		{
			res = 0;
		}
		
		return res;
	}
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		u64 highest_prio = ~0ull;
		u64 sel = ~0ull;
		for (auto& v : m_waiting)
		{
			if (std::shared_ptr<CPUThread> t = Emu.GetCPU().GetThread(v))
			{
				const u64 prio = t->GetPrio();
				if (prio < highest_prio)
				{
					highest_prio = prio;
					sel = &v - m_waiting.data();
				}
			}
			else
			{
				LOG_ERROR(HLE, "sleep_queue_t['%s']::signal(SYS_SYNC_PRIORITY) failed: invalid thread (%d)", m_name.c_str(), v);
				Emu.Pause();
			}
		}

		if (~sel)
		{
			res = m_waiting[sel];
			m_waiting.erase(m_waiting.begin() + sel);
			m_signaled.push_back(res);
			return res;
		}
		// fallthrough
	}
	case SYS_SYNC_RETRY:
	{
		return 0;
	}
	}

	LOG_ERROR(HLE, "sleep_queue_t['%s']::signal(): unsupported protocol (0x%x)", m_name.c_str(), protocol);
	Emu.Pause();
	return 0;
}

bool sleep_queue_t::invalidate(u32 tid, u32 protocol)
{
	assert(tid);

	switch (protocol & SYS_SYNC_ATTR_PROTOCOL_MASK)
	{
	case SYS_SYNC_FIFO:
	case SYS_SYNC_PRIORITY:
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& v : m_waiting)
		{
			if (v == tid)
			{
				m_waiting.erase(m_waiting.begin() + (&v - m_waiting.data()));
				return true;
			}
		}

		for (auto& v : m_signaled)
		{
			if (v == tid)
			{
				if (&v == m_signaled.data())
				{
					return false; // if the thread is signaled, pop() should be used
				}
				m_signaled.erase(m_signaled.begin() + (&v - m_signaled.data()));
				return true;
			}
		}

		return false;
	}
	case SYS_SYNC_RETRY:
	{
		return true;
	}
	}

	LOG_ERROR(HLE, "sleep_queue_t['%s']::invalidate(): unsupported protocol (0x%x)", m_name.c_str(), protocol);
	Emu.Pause();
	return 0;
}

bool sleep_queue_t::signal_selected(u32 tid)
{
	assert(tid);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& v : m_waiting)
	{
		if (v == tid)
		{
			m_waiting.erase(m_waiting.begin() + (&v - m_waiting.data()));
			m_signaled.push_back(tid);
			return true;
		}
	}

	return false;
}

u32 sleep_queue_t::count()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return (u32)m_waiting.size() + (u32)m_signaled.size();
}
