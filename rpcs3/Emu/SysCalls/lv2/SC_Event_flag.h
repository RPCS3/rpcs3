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

struct EventFlagWaiter
{
	u32 tid;
	u32 mode;
	u64 bitptn;
};

struct EventFlag
{
	SMutex m_mutex;
	u64 flags;
	std::vector<EventFlagWaiter> waiters;
	SMutex signal;
	const u32 m_protocol;
	const int m_type;

	EventFlag(u64 pattern, u32 protocol, int type)
		: flags(pattern)
		, m_protocol(protocol)
		, m_type(type)
	{
	}

	u32 check()
	{
		SleepQueue sq; // TODO: implement without SleepQueue

		u32 target = 0;

		for (u32 i = 0; i < waiters.size(); i++)
		{
			if (((waiters[i].mode & SYS_EVENT_FLAG_WAIT_AND) && (flags & waiters[i].bitptn) == waiters[i].bitptn) ||
				((waiters[i].mode & SYS_EVENT_FLAG_WAIT_OR) && (flags & waiters[i].bitptn)))
			{
				if (m_protocol == SYS_SYNC_FIFO)
				{
					target = waiters[i].tid;
					break;
				}
				sq.list.push_back(waiters[i].tid);
			}
		}

		if (m_protocol == SYS_SYNC_PRIORITY)
		{
			target = sq.pop_prio();
		}

		return target;
	}
};
