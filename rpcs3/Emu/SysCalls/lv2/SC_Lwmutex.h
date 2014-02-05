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
	// ????
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
	char name[8];
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

	int trylock(u32 tid)
	{
		if (tid == (u32)owner.GetOwner()) // recursive or deadlock
		{
			if (attribute & se32(SYS_SYNC_RECURSIVE))
			{
				recursive_count += 1;
				if (!recursive_count) return CELL_EKRESOURCE;
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
		default: return CELL_EBUSY;
		}
	}

	bool unlock(u32 tid)
	{
		if (tid != (u32)owner.GetOwner())
		{
			return false;
		}
		else
		{
			recursive_count -= 1;
			if (!recursive_count)
			{
				owner.unlock(tid);
			}
			return true;
		}
	}

	int lock(u32 tid, u64 timeout)
	{
		switch (int res = trylock(tid))
		{
		case CELL_OK: return CELL_OK;
		case CELL_EBUSY: break;
		default: return res;
		}
		switch (owner.lock(tid, timeout))
		{
		case SMR_OK: recursive_count = 1; return CELL_OK;
		case SMR_TIMEOUT: return CELL_ETIMEDOUT;
		default: return CELL_EINVAL;
		}
	}
};

class lwmutex_locker
{
	mem_ptr_t<sys_lwmutex_t> m_mutex;
	u32 m_id;

	lwmutex_locker(u32 lwmutex_addr, u32 tid, u64 timeout = 0)
		: m_id(tid)
		, m_mutex(lwmutex_addr)
	{
		if (int res = m_mutex->lock(m_id, timeout)) throw "lwmutex_locker: m_mutex->lock failed";
	}

	~lwmutex_locker()
	{
		m_mutex->unlock(m_id);
	}
};