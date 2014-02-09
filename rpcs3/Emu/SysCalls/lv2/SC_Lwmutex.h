#pragma once
#include <Utilities/SMutex.h>

// attr_protocol (waiting scheduling policy)
enum
{
	// First In, First Out
	SYS_SYNC_FIFO = 1,
	// Priority Order
	SYS_SYNC_PRIORITY = 2,
	// Basic Priority Inheritance Protocol
	SYS_SYNC_PRIORITY_INHERIT = 3,
	// Not selected while unlocking
	SYS_SYNC_RETRY = 4,
	//
	SYS_SYNC_ATTR_PROTOCOL_MASK = 0xF, 
};

// attr_recursive (recursive locks policy)
enum
{
	// Recursive locks are allowed
	SYS_SYNC_RECURSIVE = 0x10,
	// Recursive locks are NOT allowed
	SYS_SYNC_NOT_RECURSIVE = 0x20,
	//
	SYS_SYNC_ATTR_RECURSIVE_MASK = 0xF0, //???
};

struct sys_lwmutex_attribute_t
{
	be_t<u32> attr_protocol;
	be_t<u32> attr_recursive;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

class SleepQueue
{
	/* struct q_rec
	{
		u32 tid;
		u64 prio;
		q_rec(u32 tid, u64 prio): tid(tid), prio(prio) {}
	}; */

	SMutex m_mutex;
	Array<u32> list;
	u64 m_name;

public:
	SleepQueue(u64 name)
		: m_name(name)
	{
	}

	void push(u32 tid)
	{
		SMutexLocker lock(m_mutex);
		list.AddCpy(tid);
	}

	u32 pop() // SYS_SYNC_FIFO
	{
		SMutexLocker lock(m_mutex);
		
		while (true)
		{
			if (list.GetCount())
			{
				u32 res = list[0];
				list.RemoveAt(0);
				if (Emu.GetIdManager().CheckID(res))
				// check thread
				{
					return res;
				}
			}
			return 0;
		};
	}

	u32 pop_prio() // SYS_SYNC_PRIORITY
	{
		SMutexLocker lock(m_mutex);

		while (true)
		{
			if (list.GetCount())
			{
				u64 max_prio = 0;
				u32 sel = 0;
				for (u32 i = 0; i < list.GetCount(); i++)
				{
					CPUThread* t = Emu.GetCPU().GetThread(list[i]);
					if (!t)
					{
						list[i] = 0;
						sel = i;
						break;
					}

					u64 prio = t->GetPrio();
					if (prio > max_prio)
					{
						max_prio = prio;
						sel = i;
					}
				}
				u32 res = list[sel];
				list.RemoveAt(sel);
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

	u32 pop_prio_inherit() // (TODO)
	{
		ConLog.Error("TODO: SleepQueue::pop_prio_inherit()");
		Emu.Pause();
	}
};

struct sys_lwmutex_t
{
	union // sys_lwmutex_variable_t
	{
		struct // sys_lwmutex_lock_info_t
		{
			/* volatile */ SMutexBE owner;
			/* volatile */ be_t<u32> waiter; // not used
		};
		struct
		{ 
			/* volatile */ be_t<u64> all_info;
		};
	};
	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue;
	be_t<u32> pad;

	int trylock(be_t<u32> tid)
	{
		if (!attribute.ToBE()) return CELL_EINVAL;

		if (tid == owner.GetOwner()) 
		{
			if (attribute.ToBE() & se32(SYS_SYNC_RECURSIVE))
			{
				recursive_count += 1;
				if (!recursive_count.ToBE()) return CELL_EKRESOURCE;
				return CELL_OK;
			}
			else
			{
				return CELL_EDEADLK;
			}
		}

		switch (owner.trylock(tid))
		{
		case SMR_OK: recursive_count = 1; return CELL_OK;
		case SMR_FAILED: return CELL_EBUSY;
		default: return CELL_EINVAL;
		}
	}

	int unlock(be_t<u32> tid)
	{
		if (tid != owner.GetOwner())
		{
			return CELL_EPERM;
		}
		else
		{
			recursive_count -= 1;
			if (!recursive_count.ToBE())
			{
				be_t<u32> target = 0;
				switch (attribute.ToBE() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK))
				{
				case se32(SYS_SYNC_FIFO):
				case se32(SYS_SYNC_PRIORITY):
					SleepQueue* sq;
					if (!Emu.GetIdManager().GetIDData(sleep_queue, sq)) return CELL_ESRCH;
					target = attribute.ToBE() & se32(SYS_SYNC_FIFO) ? sq->pop() : sq->pop_prio();
				case se32(SYS_SYNC_RETRY): default: owner.unlock(tid, target); break;
				}
			}
			return CELL_OK;
		}
	}

	int lock(be_t<u32> tid, u64 timeout)
	{
		switch (int res = trylock(tid))
		{
		case CELL_EBUSY: break;
		default: return res;
		}

		switch (attribute.ToBE() & se32(SYS_SYNC_ATTR_PROTOCOL_MASK))
		{
		case se32(SYS_SYNC_PRIORITY):
		case se32(SYS_SYNC_FIFO):
			SleepQueue* sq;
			if (!Emu.GetIdManager().GetIDData(sleep_queue, sq)) return CELL_ESRCH;
			sq->push(tid);
		default: break;
		}

		switch (owner.lock(tid, timeout))
		{
		case SMR_OK: case SMR_SIGNAL: recursive_count = 1; return CELL_OK;
		case SMR_TIMEOUT: return CELL_ETIMEDOUT;
		default: return CELL_EINVAL;
		}
	}
};

class lwmutex_locker
{
	mem_ptr_t<sys_lwmutex_t> m_mutex;
	be_t<u32> m_id;

	lwmutex_locker(mem_ptr_t<sys_lwmutex_t> lwmutex, be_t<u32> tid, u64 timeout = 0)
		: m_id(tid)
		, m_mutex(lwmutex)
	{
		if (int res = m_mutex->lock(m_id, timeout))
		{
			ConLog.Error("lwmutex_locker: m_mutex->lock failed(res=0x%x)", res);
			Emu.Pause();
		}
	}

	~lwmutex_locker()
	{
		m_mutex->unlock(m_id);
	}
};