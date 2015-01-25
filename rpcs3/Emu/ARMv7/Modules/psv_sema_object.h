#pragma once

struct psv_sema_t
{
	char name[32];
	u32 attr;
	s32 initCount;
	s32 maxCount;
};

extern psv_object_list_t<psv_sema_t, SCE_KERNEL_THREADMGR_UID_CLASS_SEMA> g_psv_sema_list;
