#pragma once

// attr_protocol (waiting scheduling policy)
enum
{
	// First In, First Out
	SYS_SYNC_FIFO = 1,
	// Priority Order (doesn't care?)
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
};

struct sys_lwmutex_attribute_t
{
	be_t<u32> attr_protocol;
	be_t<u32> attr_recursive;
	char name[8];
};