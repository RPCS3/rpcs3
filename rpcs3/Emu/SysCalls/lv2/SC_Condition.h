#pragma once
#include "SC_Mutex.h"

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

struct Cond
{
	Mutex* mutex; // associated with mutex
	SMutex signal;
	SleepQueue m_queue;
	u64 signal_stamp;

	Cond(Mutex* mutex, u64 name)
		: mutex(mutex)
		, m_queue(name)
	{
	}
};