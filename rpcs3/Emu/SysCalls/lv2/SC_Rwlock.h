#pragma once

struct sys_rwlock_attribute_t
{
	be_t<u32> attr_protocol;
	be_t<u32> attr_pshared; // == 0x200 (NOT SHARED)
	be_t<u64> key; // process-shared key (not used)
	be_t<s32> flags; // process-shared flags (not used)
	be_t<u32> pad;
	char name[8];
};

#pragma pack()

struct RWLock
{
	std::mutex m_lock; // internal lock
	u32 wlock_thread; // write lock owner
	Array<u32> wlock_queue; // write lock queue
	Array<u32> rlock_list; // read lock list
	u32 m_protocol; // TODO

	u32 m_pshared; // not used
	u64 m_key; // not used
	s32 m_flags; // not used
	union
	{
		u64 m_name_data; // not used
		char m_name[8];
	};

	RWLock(u32 protocol, u32 pshared, u64 key, s32 flags, u64 name)
		: m_protocol(protocol)
		, m_pshared(pshared)
		, m_key(key)
		, m_flags(flags)
		, m_name_data(name)
		, wlock_thread(0)
	{
	}

	bool rlock_trylock(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (!wlock_thread && !wlock_queue.GetCount())
		{
			rlock_list.AddCpy(id);
			return true;
		}
		return false;
	}

	bool rlock_unlock(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		for (u32 i = rlock_list.GetCount() - 1; ~i; i--)
		{
			if (rlock_list[i] == id)
			{
				rlock_list.RemoveAt(i);
				return true;
			}
		}
		return false;
	}

	bool wlock_check(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread == id)
		{
			return false; // deadlock
		}
		for (u32 i = rlock_list.GetCount() - 1; ~i; i--)
		{
			if (rlock_list[i] == id)
			{
				return false; // deadlock
			}
		}
		return true;
	}

	bool wlock_trylock(u32 id, bool enqueue)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread || rlock_list.GetCount()) // already locked
		{
			if (!enqueue)
			{
				return false; // do not enqueue
			}
			for (u32 i = wlock_queue.GetCount() - 1; ~i; i--)
			{
				if (wlock_queue[i] == id)
				{
					return false; // already enqueued
				}
			}
			wlock_queue.AddCpy(id); // enqueue new thread
			return false;
		}
		else
		{
			if (wlock_queue.GetCount())
			{
				// SYNC_FIFO only yet
				if (wlock_queue[0] == id)
				{
					wlock_thread = id;
					wlock_queue.RemoveAt(0);
					return true;
				}
				else
				{
					if (!enqueue)
					{
						return false; // do not enqueue
					}
					for (u32 i = wlock_queue.GetCount() - 1; ~i; i--)
					{
						if (wlock_queue[i] == id)
						{
							return false; // already enqueued
						}
					}
					wlock_queue.AddCpy(id); // enqueue new thread
					return false;
				}
			}
			else
			{
				wlock_thread = id; // easy way
				return true;
			}
		}
	}

	bool wlock_unlock(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread == id)
		{
			wlock_thread = 0;
			return true;
		}
		return false;
	}
};