#pragma once

struct sys_semaphore_attribute
{
	be_t<u32> protocol;
	be_t<u32> pshared; // undefined
	be_t<u64> ipc_key; // undefined
	be_t<int> flags; // undefined
	be_t<u32> pad; // not used
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct Semaphore
{
	std::mutex m_mutex;
	SleepQueue m_queue;
	int m_value;
	u32 signal;

	const int max;
	const u32 protocol;
	const u64 name;

	Semaphore(int initial_count, int max_count, u32 protocol, u64 name)
		: m_value(initial_count)
		, signal(0)
		, max(max_count)
		, protocol(protocol)
		, name(name)
	{
	}
};