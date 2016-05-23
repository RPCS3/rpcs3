#pragma once

#include "Utilities/SleepQueue.h"
#include <mutex>
#include <condition_variable>

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

extern std::condition_variable& get_current_thread_cv();

// Simple class for global mutex to pass unique_lock and check it
struct lv2_lock_t
{
	using type = std::unique_lock<std::mutex>;

	type& ref;

	lv2_lock_t(type& lv2_lock)
		: ref(lv2_lock)
	{
		EXPECTS(ref.owns_lock());
		EXPECTS(ref.mutex() == &mutex);
	}

	operator type&() const
	{
		return ref;
	}

	static type::mutex_type mutex;
};

#define LV2_LOCK lv2_lock_t::type lv2_lock(lv2_lock_t::mutex)
