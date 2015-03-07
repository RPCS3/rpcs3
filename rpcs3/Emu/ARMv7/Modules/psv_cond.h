#pragma once

struct psv_cond_t
{
	char name[32];
	u32 attr;
	s32 mutexId;

public:
	psv_cond_t(const char* name, u32 attr, s32 mutexId);
};

typedef psv_object_list_t<psv_cond_t, SCE_KERNEL_THREADMGR_UID_CLASS_COND> psv_cond_list_t;

extern psv_cond_list_t g_psv_cond_list;
