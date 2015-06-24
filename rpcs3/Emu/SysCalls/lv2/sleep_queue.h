#pragma once

namespace vm { using namespace ps3; }

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

// attr_pshared
enum
{
	SYS_SYNC_NOT_PROCESS_SHARED = 0x200,
};

// attr_adaptive
enum
{
	SYS_SYNC_ADAPTIVE     = 0x1000,
	SYS_SYNC_NOT_ADAPTIVE = 0x2000,
};

class sleep_queue_t
{
	std::vector<u32> m_waiting;
	std::vector<u32> m_signaled;
	std::mutex m_mutex;
	std::string m_name;

public:
	const u64 name;

	sleep_queue_t(u64 name = 0)
		: name(name)
	{
	}

	~sleep_queue_t();

	void set_full_name(const std::string& name) { m_name = name; }
	const std::string& get_full_name() { return m_name; }

	void push(u32 tid, u32 protocol);
	bool pop(u32 tid, u32 protocol);
	u32 signal(u32 protocol);
	bool signal_selected(u32 tid);
	bool invalidate(u32 tid, u32 protocol);
	u32 count();
};
