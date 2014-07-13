#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Event.h"
#include "sys_timer.h"

SysCallBase sys_timer("sys_timer");

s32 sys_timer_create(mem32_t timer_id)
{
	sys_timer.Warning("sys_timer_create(timer_id_addr=0x%x)", timer_id.GetAddr());

	if(!Memory.IsGoodAddr(timer_id.GetAddr())) return CELL_EFAULT;

	timer_id = sys_timer.GetNewId(new timer);
	return CELL_OK;
}

s32 sys_timer_destroy(u32 timer_id)
{
	sys_timer.Warning("TODO: sys_timer_destroy(timer_id=%d)", timer_id);

	if(!sys_timer.CheckId(timer_id)) return CELL_ESRCH;

	Emu.GetIdManager().RemoveID(timer_id);
	return CELL_OK;
}

s32 sys_timer_get_information(u32 timer_id, mem_ptr_t<sys_timer_information_t> info)
{
	sys_timer.Warning("sys_timer_get_information(timer_id=%d, info_addr=0x%x)", timer_id, info.GetAddr());
	
	timer* timer_data = nullptr;
	if(!sys_timer.CheckId(timer_id, timer_data)) return CELL_ESRCH;

	*info = timer_data->timer_information_t;
	return CELL_OK;
}

s32 sys_timer_start(u32 timer_id, s64 base_time, u64 period)
{
	sys_timer.Warning("sys_timer_start_periodic_absolute(timer_id=%d, basetime=%lld, period=%llu)", timer_id, base_time, period);

	timer* timer_data = nullptr;
	if(!sys_timer.CheckId(timer_id, timer_data)) return CELL_ESRCH;

	if(timer_data->timer_information_t.timer_state != SYS_TIMER_STATE_STOP) return CELL_EBUSY;
	if(period < 100) return CELL_EINVAL;
	//TODO: if (timer is not connected to an event queue) return CELL_ENOTCONN;
	
	timer_data->timer_information_t.next_expiration_time = base_time;
	timer_data->timer_information_t.period = period;
	timer_data->timer_information_t.timer_state = SYS_TIMER_STATE_RUN;
	//TODO: ?
	std::function<s32()> task(std::bind(sys_timer_stop, timer_id));
	std::thread([period, task]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(period));
		task();
	}).detach();

	return CELL_OK;
}

s32 sys_timer_stop(u32 timer_id)
{
	sys_timer.Warning("TODO: sys_timer_stop()");

	timer* timer_data = nullptr;
	if(!sys_timer.CheckId(timer_id, timer_data)) return CELL_ESRCH;

	timer_data->timer_information_t.timer_state = SYS_TIMER_STATE_STOP;
	return CELL_OK;
}

s32 sys_timer_connect_event_queue(u32 timer_id, u32 queue_id, u64 name, u64 data1, u64 data2)
{
	sys_timer.Warning("sys_timer_connect_event_queue(timer_id=%d, queue_id=%d, name=%llu, data1=%llu, data2=%llu)",
		timer_id, queue_id, name, data1, data2);

	timer* timer_data = nullptr;
	EventQueue* equeue = nullptr;
	if(!sys_timer.CheckId(timer_id, timer_data)) return CELL_ESRCH;
	if(!sys_timer.CheckId(queue_id, equeue)) return CELL_ESRCH;

	//TODO: ?

	return CELL_OK;
}

s32 sys_timer_disconnect_event_queue(u32 timer_id)
{
	sys_timer.Warning("TODO: sys_timer_disconnect_event_queue(timer_id=%d)", timer_id);

	timer* timer_data = nullptr;
	if(!sys_timer.CheckId(timer_id, timer_data)) return CELL_ESRCH;

	//TODO: ?

	return CELL_OK;
}

s32 sys_timer_sleep(u32 sleep_time)
{
	sys_timer.Warning("sys_timer_sleep(sleep_time=%d)", sleep_time);
	for (u32 i = 0; i < sleep_time; i++)
	{
		if (Emu.IsStopped())
		{
			sys_timer.Warning("sys_timer_sleep(sleep_time=%d)", sleep_time);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return CELL_OK;
}

s32 sys_timer_usleep(u64 sleep_time)
{
	sys_timer.Warning("sys_timer_usleep(sleep_time=%lld)", sleep_time);
	if (sleep_time > 0xFFFFFFFFFFFF) sleep_time = 0xFFFFFFFFFFFF; //2^48-1
	for (u64 i = 0; i < sleep_time; i += 1000000)
	{
		if (Emu.IsStopped())
		{
			sys_timer.Warning("sys_timer_usleep(sleep_time=%lld)", sleep_time);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::this_thread::sleep_for(std::chrono::microseconds(sleep_time % 1000000));
	return CELL_OK;
}
