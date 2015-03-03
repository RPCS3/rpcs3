#pragma once

struct psv_event_flag_t
{
	char name[32];
	u32 attr;
	u32 pattern;

public:
	psv_event_flag_t(const char* name, u32 attr, u32 pattern);
};

typedef psv_object_list_t<psv_event_flag_t, SCE_KERNEL_THREADMGR_UID_CLASS_EVENT_FLAG> psv_ef_list_t;

extern psv_ef_list_t g_psv_ef_list;
