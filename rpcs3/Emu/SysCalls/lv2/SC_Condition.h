#pragma once

struct sys_cond_attribute
{
	be_t<u32> pshared;
	be_t<u64> ipc_key;
	be_t<int> flags;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct condition
{
	wxCondition cond;
	u64 name_u64;

	condition(wxMutex& mtx, u64 name)
		: cond(mtx)
		, name_u64(name)
	{
	}
};