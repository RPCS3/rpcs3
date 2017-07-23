#pragma once

#include "Utilities/Thread.h"

// Timer State
enum : u32
{
	SYS_TIMER_STATE_STOP = 0,
	SYS_TIMER_STATE_RUN  = 1,
};

struct sys_timer_information_t
{
	be_t<s64> next_expire;
	be_t<u64> period;
	be_t<u32> timer_state;
	be_t<u32> pad;
};

struct lv2_timer final : public lv2_obj, public named_thread
{
	static const u32 id_base = 0x11000000;

	void on_task() override;
	void on_stop() override;

	semaphore<> mutex;
	atomic_t<u32> state{SYS_TIMER_STATE_STOP};

	std::weak_ptr<lv2_event_queue> port;
	u64 source;
	u64 data1;
	u64 data2;
	
	atomic_t<u64> expire{0}; // Next expiration time
	atomic_t<u64> period{0}; // Period (oneshot if 0)
};

class ppu_thread;

// Syscalls

error_code sys_timer_create(vm::ps3::ptr<u32> timer_id);
error_code sys_timer_destroy(u32 timer_id);
error_code sys_timer_get_information(u32 timer_id, vm::ps3::ptr<sys_timer_information_t> info);
error_code _sys_timer_start(u32 timer_id, u64 basetime, u64 period); // basetime type changed from s64
error_code sys_timer_stop(u32 timer_id);
error_code sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2);
error_code sys_timer_disconnect_event_queue(u32 timer_id);
error_code sys_timer_sleep(ppu_thread&, u32 sleep_time);
error_code sys_timer_usleep(ppu_thread&, u64 sleep_time);
