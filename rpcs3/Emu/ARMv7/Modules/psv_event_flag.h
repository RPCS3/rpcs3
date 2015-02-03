#pragma once

struct psv_event_flag_t
{
	char name[32];
	u32 attr;
	u32 pattern;

private:
	psv_event_flag_t() = delete;
	psv_event_flag_t(const psv_event_flag_t&) = delete;
	psv_event_flag_t(psv_event_flag_t&&) = delete;

	psv_event_flag_t& operator =(const psv_event_flag_t&) = delete;
	psv_event_flag_t& operator =(psv_event_flag_t&&) = delete;

public:
	psv_event_flag_t(const char* name, u32 attr, u32 pattern);
};

extern psv_object_list_t<psv_event_flag_t, SCE_KERNEL_THREADMGR_UID_CLASS_EVENT_FLAG> g_psv_ef_list;
