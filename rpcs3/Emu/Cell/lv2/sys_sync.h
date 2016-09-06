#pragma once

#include "Utilities/SleepQueue.h"
#include "Utilities/Thread.h"
#include "Utilities/mutex.h"
#include "Utilities/sema.h"
#include "Utilities/cond.h"

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

// Temporary implementation for LV2_UNLOCK (TODO: remove it)
struct lv2_lock_guard
{
	static semaphore<> g_sema;

	lv2_lock_guard(const lv2_lock_guard&) = delete;

	lv2_lock_guard()
	{
		g_sema.post();
	}

	~lv2_lock_guard()
	{
		g_sema.wait();
	}
};

using lv2_lock_t = semaphore_lock&;

#define LV2_LOCK semaphore_lock lv2_lock(lv2_lock_guard::g_sema)

#define LV2_UNLOCK lv2_lock_guard{}
