#pragma once

struct sys_lwcond_attribute_t
{
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct sys_lwcond_t
{
	be_t<u32> lwmutex;
	be_t<u32> lwcond_queue;
};

#pragma pack()

struct LWCond
{
	std::mutex m_lock;
	Array<u32> waiters; // list of waiting threads
	Array<u32> signaled; // list of signaled threads
	u64 m_name; // not used

	LWCond(u64 name)
		: m_name(name)
	{
	}

	void signal(u32 _protocol)
	{
		std::lock_guard<std::mutex> lock(m_lock);

		if (waiters.GetCount())
		{
			if ((_protocol & SYS_SYNC_ATTR_PROTOCOL_MASK) == SYS_SYNC_PRIORITY)
			{
				u64 max_prio = 0;
				u32 sel = 0;
				for (u32 i = 0; i < waiters.GetCount(); i++)
				{
					CPUThread* t = Emu.GetCPU().GetThread(waiters[i]);
					if (!t) continue;

					u64 prio = t->GetPrio();
					if (prio > max_prio)
					{
						max_prio = prio;
						sel = i;
					}
				}
				signaled.AddCpy(waiters[sel]);
				waiters.RemoveAt(sel);
			}
			else // SYS_SYNC_FIFO
			{
				signaled.AddCpy(waiters[0]);
				waiters.RemoveAt(0);
			}
		}
	}

	void signal_all()
	{
		std::lock_guard<std::mutex> lock(m_lock);
		
		signaled.AppendFrom(waiters); // "nobody cares" protocol (!)
		waiters.Clear();
	}

	bool signal_to(u32 id) // returns false if not found
	{
		std::lock_guard<std::mutex> lock(m_lock);
		for (u32 i = waiters.GetCount() - 1; ~i; i--)
		{
			if (waiters[i] == id)
			{
				waiters.RemoveAt(i);
				signaled.AddCpy(id);
				return true;
			}
		}
		return false;
	}

	void begin_waiting(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		waiters.AddCpy(id);
	}

	void stop_waiting(u32 id)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		for (u32 i = waiters.GetCount() - 1; ~i; i--)
		{
			if (waiters[i] == id)
			{
				waiters.RemoveAt(i);
				break;
			}
		}
	}

	bool check(u32 id) // returns true if signaled
	{
		std::lock_guard<std::mutex> lock(m_lock);
		for (u32 i = signaled.GetCount() - 1; ~i; i--)
		{
			if (signaled[i] == id)
			{
				signaled.RemoveAt(i);
				return true;
			}
		}
		return false;
	}
};