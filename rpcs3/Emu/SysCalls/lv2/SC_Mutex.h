#pragma once

struct sys_mutex_attribute
{
	be_t<u32> protocol; // SYS_SYNC_FIFO, SYS_SYNC_PRIORITY or SYS_SYNC_PRIORITY_INHERIT
	be_t<u32> recursive; // SYS_SYNC_RECURSIVE or SYS_SYNC_NOT_RECURSIVE
	be_t<u32> pshared; // always 0x200 (not shared)
	be_t<u32> adaptive;
	be_t<u64> ipc_key;
	be_t<int> flags;
	be_t<u32> pad;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct Mutex
{
	u32 id;
	SMutex m_mutex;
	SleepQueue m_queue;
	u32 recursive; // recursive locks count
	std::atomic<u32> cond_count; // count of condition variables associated

	const u32 protocol;
	const bool is_recursive;

	Mutex(u32 protocol, bool is_recursive, u64 name)
		: protocol(protocol)
		, is_recursive(is_recursive)
		, m_queue(name)
		, cond_count(0)
	{
	}

	~Mutex()
	{
		if (u32 owner = m_mutex.GetOwner())
		{
			ConLog.Write("Mutex(%d) was owned by thread %d (recursive=%d)", id, owner, recursive);
		}

		if (!m_queue.m_mutex.try_lock()) return;

		for (u32 i = 0; i < m_queue.list.size(); i++)
		{
			if (u32 owner = m_queue.list[i]) ConLog.Write("Mutex(%d) was waited by thread %d", id, owner);
		}

		m_queue.m_mutex.unlock();
	}
};
