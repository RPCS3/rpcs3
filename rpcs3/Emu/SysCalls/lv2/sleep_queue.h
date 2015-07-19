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

using sleep_queue_t = std::deque<std::shared_ptr<CPUThread>>;

static struct defer_sleep_t{} const defer_sleep;

// automatic object handling a thread entry in the sleep queue
class sleep_queue_entry_t final
{
	CPUThread& m_thread;
	sleep_queue_t& m_queue;

	void add_entry();
	void remove_entry();
	bool find() const;

public:
	// add specified thread to the sleep queue
	sleep_queue_entry_t(CPUThread& cpu, sleep_queue_t& queue);

	// don't add specified thread to the sleep queue
	sleep_queue_entry_t(CPUThread& cpu, sleep_queue_t& queue, const defer_sleep_t&);

	// removes specified thread from the sleep queue if added
	~sleep_queue_entry_t() noexcept(false);

	// add thread to the sleep queue
	inline void enter()
	{
		add_entry();
	}

	// remove thread from the sleep queue
	inline void leave()
	{
		remove_entry();
	}

	// check whether the thread exists in the sleep queue
	inline explicit operator bool() const
	{
		return find();
	}
};
