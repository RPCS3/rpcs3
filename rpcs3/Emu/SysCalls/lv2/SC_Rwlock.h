#pragma once

struct sys_rwlock_attribute_t
{
	be_t<u32> attr_protocol;
	be_t<u32> attr_pshared; // == 0x200 (NOT SHARED)
	be_t<u64> key; // process-shared key (not used)
	be_t<s32> flags; // process-shared flags (not used)
	be_t<u32> pad; // not used
	union
	{
		char name[8];
		u64 name_u64;
	};
};

#pragma pack()

struct RWLock
{
	std::mutex m_lock; // internal lock
	u32 wlock_thread; // write lock owner
	std::vector<u32> wlock_queue; // write lock queue
	std::vector<u32> rlock_list; // read lock list
	u32 m_protocol; // TODO

	union
	{
		u64 m_name_u64;
		char m_name[8];
	};

	RWLock(u32 protocol, u64 name)
		: m_protocol(protocol)
		, m_name_u64(name)
		, wlock_thread(0)
	{
	}

	bool rlock_trylock(u32 tid)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (!wlock_thread && !wlock_queue.size())
		{
			rlock_list.push_back(tid);
			return true;
		}
		return false;
	}

	bool rlock_unlock(u32 tid)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		for (u32 i = rlock_list.size() - 1; ~i; i--)
		{
			if (rlock_list[i] == tid)
			{
				rlock_list.erase(rlock_list.begin() + i);
				return true;
			}
		}
		return false;
	}

	bool wlock_check(u32 tid)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread == tid)
		{
			return false; // deadlock
		}
		for (u32 i = rlock_list.size() - 1; ~i; i--)
		{
			if (rlock_list[i] == tid)
			{
				return false; // deadlock
			}
		}
		return true;
	}

	bool wlock_trylock(u32 tid, bool enqueue)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread || rlock_list.size()) // already locked
		{
			if (!enqueue)
			{
				return false; // do not enqueue
			}
			for (u32 i = wlock_queue.size() - 1; ~i; i--)
			{
				if (wlock_queue[i] == tid)
				{
					return false; // already enqueued
				}
			}
			wlock_queue.push_back(tid); // enqueue new thread
			return false;
		}
		else
		{
			if (wlock_queue.size())
			{
				// SYNC_FIFO only yet
				if (wlock_queue[0] == tid)
				{
					wlock_thread = tid;
					wlock_queue.erase(wlock_queue.begin());
					return true;
				}
				else
				{
					if (!enqueue)
					{
						return false; // do not enqueue
					}
					for (u32 i = wlock_queue.size() - 1; ~i; i--)
					{
						if (wlock_queue[i] == tid)
						{
							return false; // already enqueued
						}
					}
					wlock_queue.push_back(tid); // enqueue new thread
					return false;
				}
			}
			else
			{
				wlock_thread = tid; // easy way
				return true;
			}
		}
	}

	bool wlock_unlock(u32 tid)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (wlock_thread == tid)
		{
			wlock_thread = 0;
			return true;
		}
		return false;
	}
};
