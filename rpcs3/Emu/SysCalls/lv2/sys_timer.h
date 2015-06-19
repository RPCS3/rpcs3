#pragma once

#include "Utilities/Thread.h"

namespace vm { using namespace ps3; }

// Timer State
enum : u32
{
	SYS_TIMER_STATE_STOP = 0,
	SYS_TIMER_STATE_RUN  = 1,
};

struct sys_timer_information_t
{
	be_t<s64> next_expiration_time;
	be_t<u64> period;
	be_t<u32> timer_state;
	be_t<u32> pad;
};

// "timer_t" conflicts with some definition
struct lv2_timer_t
{
	std::weak_ptr<lv2_event_queue_t> port; // event queue
	u64 source; // event source
	u64 data1; // event arg 1
	u64 data2; // event arg 2
	
	u64 start; // next expiration time
	u64 period; // period (oneshot if 0)

	std::atomic<u32> state; // timer state
	std::condition_variable cv;

	thread_t thread; // timer thread

	lv2_timer_t();
	~lv2_timer_t();
};

REG_ID_TYPE(lv2_timer_t, 0x11); // SYS_TIMER_OBJECT

s32 sys_timer_create(vm::ref<u32> timer_id);
s32 sys_timer_destroy(u32 timer_id);
s32 sys_timer_get_information(u32 timer_id, vm::ptr<sys_timer_information_t> info);
s32 _sys_timer_start(u32 timer_id, u64 basetime, u64 period); // basetime type changed from s64
s32 sys_timer_stop(u32 timer_id);
s32 sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2);
s32 sys_timer_disconnect_event_queue(u32 timer_id);
s32 sys_timer_sleep(u32 sleep_time);
s32 sys_timer_usleep(u64 sleep_time);
