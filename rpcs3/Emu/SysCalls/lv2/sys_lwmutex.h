#pragma once

// attr_protocol (waiting scheduling policy)
enum
{
	// First In, First Out
	SYS_SYNC_FIFO = 1,
	// Priority Order
	SYS_SYNC_PRIORITY = 2,
	// Basic Priority Inheritance Protocol (probably not implemented)
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
	be_t<u32> protocol;
	be_t<u32> recursive;
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
	std::vector<u32> list;
	std::mutex m_mutex;
	u64 m_name;

	SleepQueue(u64 name = 0)
		: m_name(name)
	{
	}

	void push(u32 tid);
	u32 pop(); // SYS_SYNC_FIFO
	u32 pop_prio(); // SYS_SYNC_PRIORITY
	u32 pop_prio_inherit(); // (TODO)
	bool invalidate(u32 tid);
	u32 count();
	bool finalize();
};

struct sys_lwmutex_t
{
	atomic_t<u32> mutex;
	atomic_t<u32> waiter; // currently not used
	be_t<u32> attribute;
	be_t<u32> recursive_count;
	be_t<u32> sleep_queue;
	be_t<u32> pad;

	u64& all_info()
	{
		return *(reinterpret_cast<u64*>(this));
	}

	s32 trylock(be_t<u32> tid);
	s32 unlock(be_t<u32> tid);
	s32 lock(be_t<u32> tid, u64 timeout);
};

// Aux
s32 lwmutex_create(sys_lwmutex_t& lwmutex, u32 protocol, u32 recursive, u64 name_u64);

// SysCalls
s32 sys_lwmutex_create(vm::ptr<sys_lwmutex_t> lwmutex, vm::ptr<sys_lwmutex_attribute_t> attr);
s32 sys_lwmutex_destroy(vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_lock(vm::ptr<sys_lwmutex_t> lwmutex, u64 timeout);
s32 sys_lwmutex_trylock(vm::ptr<sys_lwmutex_t> lwmutex);
s32 sys_lwmutex_unlock(vm::ptr<sys_lwmutex_t> lwmutex);
