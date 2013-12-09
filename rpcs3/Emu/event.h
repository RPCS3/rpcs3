#pragma once

struct sys_event_flag_attr
{
	u32 protocol;
	u32 pshared;
	u64 ipc_key;
	int flags;
	int type;
	char name[8];
};

struct event_flag
{
	sys_event_flag_attr attr;
	u64 pattern;

	event_flag(u64 pattern, sys_event_flag_attr attr)
		: pattern(pattern)
		, attr(attr)
	{
	}
};

struct sys_event_queue_attr
{
	u32 attr_protocol;
	int type;
	char name[8];
};

struct sys_event_data
{
	u64 source;
	u64 data1;
	u64 data2;
	u64 data3;
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
	int type;
	char name[8];
};