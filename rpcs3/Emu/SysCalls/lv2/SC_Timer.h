#pragma once

enum
{
	SYS_TIMER_STATE_STOP      = 0x00U,
	SYS_TIMER_STATE_RUN       = 0x01U,
};

struct sys_timer_information_t
{
	s64 next_expiration_time; //system_time_t
	u64 period; //usecond_t
	u32 timer_state;
	u32 pad;
};

struct timer
{
	wxTimer tmr;
	sys_timer_information_t timer_information_t;
};

#pragma pack()