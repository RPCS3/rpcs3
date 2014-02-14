#pragma once

struct sys_lwcond_attribute_t
{
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct sys_lwcond_t
{
	be_t<u32> lwmutex;
	be_t<u32> lwcond_queue;
};