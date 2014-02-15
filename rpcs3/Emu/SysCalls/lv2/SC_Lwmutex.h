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

struct SleepQueue
{
	/* struct q_rec
	{
		u32 tid;
		u64 prio;
		q_rec(u32 tid, u64 prio): tid(tid), prio(prio) {}
	}; */
	Array<u32> list;
	SMutex m_mutex;
	u64 m_name;

	SleepQueue(u64 name = 0)
		: m_name(name)
	{
	}

	void push(u32 tid);
	u32 pop(); // SYS_SYNC_FIFO
	u32 pop_prio(); // SYS_SYNC_PRIORITY
	u32 pop_prio_inherit(); // (TODO)
	void invalidate(u32 tid);
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

	int trylock(be_t<u32> tid);
	int unlock(be_t<u32> tid);
	int lock(be_t<u32> tid, u64 timeout);
};
/*
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
};*/