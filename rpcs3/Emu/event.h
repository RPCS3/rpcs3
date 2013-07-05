#pragma once

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

struct EventPort
{
	u64 name;
	u64 data1;
	u64 data2;
	u64 data3;
	bool has_data;
	PPCThread* thread;
};

struct EventQueue
{
	EventPort* ports[127];
	int size;
	int pos;
	int type;
	char name[8];
};