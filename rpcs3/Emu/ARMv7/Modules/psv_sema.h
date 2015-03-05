#pragma once

struct psv_sema_t
{
	char name[32];
	u32 attr;
	s32 value;
	s32 max;

public:
	psv_sema_t(const char* name, u32 attr, s32 init_value, s32 max_value);
};

typedef psv_object_list_t<psv_sema_t, SCE_KERNEL_THREADMGR_UID_CLASS_SEMA> psv_sema_list_t;

extern psv_sema_list_t g_psv_sema_list;
