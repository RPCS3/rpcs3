#pragma once

struct psv_mutex_t
{
	char name[32];
	u32 attr;
	s32 count;

public:
	psv_mutex_t(const char* name, u32 attr, s32 count);
};

typedef psv_object_list_t<psv_mutex_t, SCE_KERNEL_THREADMGR_UID_CLASS_MUTEX> psv_mutex_list_t;

extern psv_mutex_list_t g_psv_mutex_list;
