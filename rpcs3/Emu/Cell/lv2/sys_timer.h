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

class lv2_timer_t final : public named_thread
{
	void on_task() override;

public:
	std::string get_name() const override;

	void on_stop() override;

	const id_value<> id{};

	atomic_t<u32> state{ SYS_TIMER_STATE_RUN }; // Timer state

	std::weak_ptr<lv2_event_queue_t> port; // Event queue
	u64 source; // Event source
	u64 data1; // Event arg 1
	u64 data2; // Event arg 2
	
	u64 expire = 0; // Next expiration time
	u64 period = 0; // Period (oneshot if 0)
};

s32 sys_timer_create(vm::ptr<u32> timer_id);
s32 sys_timer_destroy(u32 timer_id);
s32 sys_timer_get_information(u32 timer_id, vm::ptr<sys_timer_information_t> info);
s32 _sys_timer_start(u32 timer_id, u64 basetime, u64 period); // basetime type changed from s64
s32 sys_timer_stop(u32 timer_id);
s32 sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2);
s32 sys_timer_disconnect_event_queue(u32 timer_id);
s32 sys_timer_sleep(u32 sleep_time);
s32 sys_timer_usleep(u64 sleep_time);
