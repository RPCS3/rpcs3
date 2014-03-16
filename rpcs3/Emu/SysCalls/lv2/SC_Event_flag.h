#pragma once

enum
{
	SYS_SYNC_WAITER_SINGLE = 0x10000,
	SYS_SYNC_WAITER_MULTIPLE = 0x20000,
	
	SYS_EVENT_FLAG_WAIT_AND = 0x01,
	SYS_EVENT_FLAG_WAIT_OR = 0x02,

	SYS_EVENT_FLAG_WAIT_CLEAR = 0x10,
	SYS_EVENT_FLAG_WAIT_CLEAR_ALL = 0x20,
};

struct sys_event_flag_attr
{
	be_t<u32> protocol;
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<int> flags;
	be_t<int> type;
	char name[8];
};

struct event_flag
{
	SMutex m_mutex;
	u64 flags;
	SleepQueue waiters;
	const u32 m_protocol;
	const int m_type;

	event_flag(u64 pattern, u32 protocol, int type)
		: flags(pattern)
		, m_protocol(protocol)
		, m_type(type)
	{
	}
};