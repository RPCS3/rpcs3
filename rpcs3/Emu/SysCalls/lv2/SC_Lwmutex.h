#pragma once

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

extern std::mutex g_lwmutex;

struct sys_lwmutex_t
{
	union // sys_lwmutex_variable_t
	{
		struct // sys_lwmutex_lock_info_t
		{
			/* volatile */ be_t<u32> owner;
			/* volatile */ be_t<u32> waiter;
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
		std::lock_guard<std::mutex> lock(g_lwmutex); // global lock

		if ((u32)attribute & SYS_SYNC_RECURSIVE)
		{
			if (tid == (u32)owner)
			{
				recursive_count = (u32)recursive_count + 1;
				if ((u32)recursive_count == 0xffffffff) return CELL_EKRESOURCE;
				return CELL_OK;
			}
		}
		else // recursive not allowed
		{
			if (tid == (u32)owner)
			{
				return CELL_EDEADLK;
			}
		}

		if (!(u32)owner) // try lock
		{
			owner = tid; 
			recursive_count = 1;
			return CELL_OK;
		}
		else
		{
			return CELL_EBUSY;
		}
	}

	bool unlock(u32 tid)
	{
		std::lock_guard<std::mutex> lock(g_lwmutex); // global lock

		if (tid != (u32)owner)
		{
			return false;
		}
		else
		{
			recursive_count = (u32)recursive_count - 1;
			if (!(u32)recursive_count)
			{
				waiter = 0; // not used yet
				owner = 0; // release
			}
			return true;
		}
	}
};

struct lwmutex_locker
{
private:
	mem_ptr_t<sys_lwmutex_t> m_mutex;
	u32 m_id;
public:
	const int res;	

	lwmutex_locker(u32 lwmutex_addr, u32 tid)
		: m_id(tid)
		, m_mutex(lwmutex_addr)
		, res(m_mutex->trylock(m_id))
	{
	}

	~lwmutex_locker()
	{
		if (res == CELL_OK) m_mutex->unlock(m_id);
	}
};