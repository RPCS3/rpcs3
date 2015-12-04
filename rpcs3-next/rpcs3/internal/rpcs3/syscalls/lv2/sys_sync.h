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
