#pragma once

struct mutex_attr
{
	u32 protocol;
	u32 recursive;
	u32 pshared;
	u32 adaptive;
	u64 ipc_key;
	int flags;
	u32 pad;
	char name[8];
};

struct mutex
{
	wxMutex mtx;
	mutex_attr attr;

	mutex(const mutex_attr& attr)
		: mtx()
		, attr(attr)
	{
	}
};