#pragma once

struct sys_lwcond_attribute_t
{
	char name[8];
};

struct sys_lwcond_t
{
	be_t<u32> lwmutex_addr;
	be_t<u32> lwcond_queue;
};

#pragma pack()

struct LWCond
{
	u64 m_name;

	LWCond(u64 name)
		: m_name(name)
	{
	}
};