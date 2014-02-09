#pragma once

enum EventQueueType
{
	SYS_PPU_QUEUE = 1,
	SYS_SPU_QUEUE = 2,
};

struct sys_event_queue_attr
{
	be_t<u32> protocol; // SYS_SYNC_PRIORITY or SYS_SYNC_FIFO
	be_t<int> type;
	union
	{
		char name[8];
		u64 name_u64;
	};
};

struct sys_event_data
{
	be_t<u64> source;
	be_t<u64> data1;
	be_t<u64> data2;
	be_t<u64> data3;
};

struct EventQueue;

struct EventPort
{
	u64 name;
	u64 data1;
	u64 data2;
	u64 data3;
	bool has_data;
	CPUThread* thread;
	EventQueue* queue[127];
	int pos;
};

struct EventQueue
{
	EventPort* ports[127];
	int size;
	int pos;

	u32 m_protocol;	
	int m_type;
	u64 m_name;
	u64 m_key;

	EventQueue(u32 protocol, int type, u64 name, u64 key, int size)
		: m_type(type)
		, m_protocol(protocol)
		, m_name(name)
		, m_key(key)
		, size(size)
		, pos(0)
	{
	}
};